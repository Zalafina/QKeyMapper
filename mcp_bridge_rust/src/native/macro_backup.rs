use std::{
    fs::{self, File, OpenOptions},
    io::{self, Write},
    os::windows::fs::{MetadataExt, OpenOptionsExt},
    path::{Component, Path, PathBuf, Prefix},
};

use base64::{Engine, engine::general_purpose::STANDARD};
use serde_json::Value;
use windows_sys::Win32::{
    Foundation::{ERROR_ALREADY_EXISTS, ERROR_FILE_EXISTS},
    Storage::FileSystem::{
        FILE_ATTRIBUTE_REPARSE_POINT, FILE_FLAG_BACKUP_SEMANTICS, FILE_FLAG_OPEN_REPARSE_POINT,
        FILE_SHARE_READ, FILE_SHARE_WRITE, GetDriveTypeW, MOVEFILE_REPLACE_EXISTING,
        MOVEFILE_WRITE_THROUGH, MoveFileExW,
    },
};

use super::{Cancel, NativeError, Result, wide};
use crate::wire::lower_hex;

const MAX_BACKUP_BYTES: usize = 128 * 1024;
const INI_PREFIX: &[u8] = b"[General]\nMacroList_Export=\"@Variant(";
const INI_SUFFIX: &[u8] = b")\"\n";
// WinBase.h. Network-mapped drive letters must not pass the local-disk boundary.
const DRIVE_FIXED: u32 = 3;

pub struct MacroBackupDirectory {
    path: PathBuf,
    _pin: File,
}

fn local_directory(path: &Path) -> Result<()> {
    let invalid = || NativeError::new("backup_directory_unavailable");
    let Some(Component::Prefix(prefix)) = path.components().next() else {
        return Err(invalid());
    };
    let (Prefix::Disk(drive) | Prefix::VerbatimDisk(drive)) = prefix.kind() else {
        return Err(invalid());
    };
    let root = [u16::from(drive), b':' as u16, b'\\' as u16, 0];
    // The drive prefix is a NUL-terminated WCHAR root without a client-controlled tail.
    if !path.is_absolute()
        || unsafe { GetDriveTypeW(root.as_ptr()) } != DRIVE_FIXED
        || path
            .components()
            .any(|component| component == Component::ParentDir)
    {
        return Err(invalid());
    }
    let directories: Vec<_> = path
        .ancestors()
        .take_while(|entry| entry.has_root())
        .collect();
    for directory in directories.into_iter().rev() {
        let info = fs::symlink_metadata(directory).map_err(|_| invalid())?;
        if !info.is_dir() || info.file_attributes() & FILE_ATTRIBUTE_REPARSE_POINT != 0 {
            return Err(invalid());
        }
    }
    Ok(())
}

pub fn valid_macro_backup_name(name: &str) -> bool {
    if name.trim() != name
        || name.encode_utf16().count() > 255
        || name
            .chars()
            .any(|c| c.is_control() || r#"<>:"/\|?*"#.contains(c))
    {
        return false;
    }
    let Some((stem, ext)) = name.rsplit_once('.') else {
        return false;
    };
    let device = stem
        .split('.')
        .next()
        .unwrap_or("")
        .trim_end()
        .to_ascii_uppercase();
    ext.eq_ignore_ascii_case("ini")
        && !device.is_empty()
        && !matches!(
            device.as_str(),
            "CON" | "PRN" | "AUX" | "NUL" | "CONIN$" | "CONOUT$"
        )
        && !matches!(
            device
                .strip_prefix("COM")
                .or_else(|| device.strip_prefix("LPT")),
            Some("0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "¹" | "²" | "³")
        )
}

impl MacroBackupDirectory {
    pub fn open(path: PathBuf) -> Result<Self> {
        local_directory(&path)?;
        let invalid = || NativeError::new("backup_directory_unavailable");
        let path = fs::canonicalize(&path).map_err(|_| invalid())?;
        local_directory(&path)?;
        let pin = OpenOptions::new()
            .read(true)
            .share_mode(FILE_SHARE_READ | FILE_SHARE_WRITE)
            .custom_flags(FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT)
            .open(&path)
            .map_err(|_| invalid())?;
        let info = pin.metadata().map_err(|_| invalid())?;
        if !info.is_dir() || info.file_attributes() & FILE_ATTRIBUTE_REPARSE_POINT != 0 {
            return Err(invalid());
        }
        Ok(Self { path, _pin: pin })
    }

    pub(crate) fn write(
        &self,
        name: &str,
        overwrite: bool,
        bytes: &[u8],
        cancel: &Cancel,
    ) -> Result<String> {
        if !valid_macro_backup_name(name) {
            return Err(NativeError::new("invalid_arguments"));
        }
        cancel.wait(0)?;
        local_directory(&self.path)?;
        let path = self.path.join(name);
        match fs::symlink_metadata(&path) {
            Ok(info)
                if !info.is_file()
                    || info.file_attributes() & FILE_ATTRIBUTE_REPARSE_POINT != 0 =>
            {
                return Err(NativeError::new("unsafe_backup_destination"));
            }
            Ok(_) if !overwrite => return Err(NativeError::new("backup_destination_exists")),
            Ok(_) => (),
            Err(error) if error.kind() == io::ErrorKind::NotFound => (),
            Err(_) => return Err(NativeError::new("backup_write_failed")),
        }
        let mut nonce = [0; 16];
        getrandom::fill(&mut nonce).map_err(|_| NativeError::new("random_unavailable"))?;
        let temp = self
            .path
            .join(format!(".qkm-backup-{}.tmp", lower_hex(&nonce)));
        let failed = || NativeError::new("backup_write_failed");
        let source = wide(temp.as_os_str(), "backup_write_failed")?;
        let dest = wide(path.as_os_str(), "backup_write_failed")?;
        let mut file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&temp)
            .map_err(|_| failed())?;
        let written = file.write_all(bytes).and_then(|_| file.sync_all());
        drop(file);
        let result = (|| {
            written.map_err(|_| failed())?;
            cancel.wait(0)?;
            let flags = MOVEFILE_WRITE_THROUGH
                | if overwrite {
                    MOVEFILE_REPLACE_EXISTING
                } else {
                    0
                };
            // Both names are in the pinned local directory. No copy or in-place write is allowed.
            if unsafe { MoveFileExW(source.as_ptr(), dest.as_ptr(), flags) } == 0 {
                let error = io::Error::last_os_error().raw_os_error();
                return Err(
                    if matches!(error, Some(value) if value == ERROR_FILE_EXISTS as i32
                    || value == ERROR_ALREADY_EXISTS as i32)
                    {
                        NativeError::new("backup_destination_exists")
                    } else {
                        failed()
                    },
                );
            }
            Ok(path.to_string_lossy().into_owned())
        })();
        if result.is_err() && fs::remove_file(&temp).is_err() {
            return Err(NativeError::new("backup_cleanup_failed"));
        }
        result
    }
}

pub(crate) fn backup_bytes(
    result: &Value,
    instance: &str,
    revision: &Value,
    scope: &Value,
) -> Result<Vec<u8>> {
    let invalid = || NativeError::new("invalid_macro_backup");
    let text = result["document_base64"].as_str().ok_or_else(invalid)?;
    if result["instance"] != instance
        || result["format"] != "qkeymapper_macro_ini_v1"
        || &result["catalog_revision"] != revision
        || &result["scope"] != scope
        || !result["macro_count"]
            .as_u64()
            .is_some_and(|count| (1..=100).contains(&count))
        || text.len() > MAX_BACKUP_BYTES.div_ceil(3) * 4
    {
        return Err(invalid());
    }
    let bytes = STANDARD.decode(text).map_err(|_| invalid())?;
    if bytes.len() > MAX_BACKUP_BYTES
        || result["bytes"].as_u64() != Some(bytes.len() as u64)
        || !bytes.starts_with(INI_PREFIX)
        || !bytes.ends_with(INI_SUFFIX)
    {
        return Err(invalid());
    }
    Ok(bytes)
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn export_accepts_only_named_local_ini_files_and_bound_documents() {
        for name in ["备份 例.ini", "Settings.INI", "Macro.v2.ini"] {
            assert!(valid_macro_backup_name(name), "{name}");
        }
        for name in [
            "../escape.ini",
            "C:\\absolute.ini",
            "other.ini:stream",
            "COM¹.ini",
            "NUL.backup.ini",
            "two.ini ",
            "path/name.ini",
            "backup.txt",
            "",
        ] {
            assert!(!valid_macro_backup_name(name), "{name}");
        }
        let document = b"[General]\nMacroList_Export=\"@Variant(example)\"\n";
        let revision = json!("revision");
        let scope = json!("universal");
        let result = json!({
            "instance":"owned", "format":"qkeymapper_macro_ini_v1", "macro_count":1,
            "catalog_revision":revision, "scope":scope,
            "bytes":document.len(), "document_base64":STANDARD.encode(document)
        });
        assert_eq!(
            backup_bytes(&result, "owned", &revision, &scope).unwrap(),
            document
        );
        assert!(backup_bytes(&result, "other", &revision, &scope).is_err());
        for (key, value) in [
            ("bytes", json!(1)),
            ("macro_count", json!(101)),
            ("document_base64", json!("%")),
        ] {
            let mut bad = result.clone();
            bad[key] = value;
            assert!(backup_bytes(&bad, "owned", &revision, &scope).is_err());
        }
    }

    #[test]
    fn export_creates_or_replaces_only_after_a_complete_local_write() {
        let mut nonce = [0; 16];
        getrandom::fill(&mut nonce).unwrap();
        let root = std::env::temp_dir().join(format!("qkm-backup-test-{}", lower_hex(&nonce)));
        fs::create_dir(&root).unwrap();
        let directory = MacroBackupDirectory::open(root.clone()).unwrap();
        let cancel = Cancel::new().unwrap();
        let name = "备份 例.ini";
        let path = root.join(name);
        directory.write(name, false, b"first", &cancel).unwrap();
        assert_eq!(fs::read(&path).unwrap(), b"first");
        assert_eq!(
            directory
                .write(name, false, b"second", &cancel)
                .unwrap_err()
                .code,
            "backup_destination_exists"
        );

        let locked = OpenOptions::new()
            .read(true)
            .share_mode(FILE_SHARE_READ)
            .open(&path)
            .unwrap();
        assert_eq!(
            directory
                .write(name, true, b"second", &cancel)
                .unwrap_err()
                .code,
            "backup_write_failed"
        );
        drop(locked);
        assert_eq!(fs::read(&path).unwrap(), b"first");
        directory.write(name, true, b"second", &cancel).unwrap();
        assert_eq!(fs::read(&path).unwrap(), b"second");

        cancel.signal().unwrap();
        assert_eq!(
            directory
                .write("cancelled.ini", false, b"test", &cancel)
                .unwrap_err()
                .code,
            "cancelled"
        );
        assert_eq!(fs::read_dir(&root).unwrap().count(), 1);
        drop(directory);
        fs::remove_file(path).unwrap();
        fs::remove_dir(root).unwrap();
    }
}
