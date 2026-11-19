use std::{
    ffi::{OsStr, OsString},
    mem::size_of_val,
    num::NonZeroU64,
    os::windows::{ffi::OsStringExt, io::AsRawHandle},
    path::{Path, PathBuf},
    ptr::{NonNull, null, null_mut},
    slice,
};

use serde::Serialize;
use serde_json::Value;
use sha2::{Digest, Sha256};
use windows_sys::{
    Win32::{
        Foundation::ERROR_NOT_FOUND,
        Security::{
            Credentials::{
                CRED_PERSIST_LOCAL_MACHINE, CRED_TYPE_GENERIC, CREDENTIALW, CredDeleteW, CredFree,
                CredReadW, CredWriteW,
            },
            Cryptography::{
                CRYPT_INTEGER_BLOB, CRYPTPROTECT_UI_FORBIDDEN, CryptProtectData, CryptUnprotectData,
            },
        },
        Storage::FileSystem::{FILE_NAME_NORMALIZED, GetFinalPathNameByHandleW},
        System::{
            Com::CoTaskMemFree,
            Registry::{HKEY_LOCAL_MACHINE, RRF_RT_REG_SZ, RRF_SUBKEY_WOW6464KEY, RegGetValueW},
        },
        UI::Shell::{FOLDERID_LocalAppData, SHGetKnownFolderPath},
    },
    w,
};
use zeroize::{Zeroize, Zeroizing};

use super::{
    LocalAllocation, NativeError, Result,
    discovery::{normcase, open_executable},
    wide,
};
use crate::wire::{decimal, decode, hex_bytes, lower_hex, scope_bits};

const MAX_CRED_BLOB: usize = 2560;
const MAX_PLAINTEXT: usize = 1024;
const CONTEXT: &str = "credential_context_unavailable";
const REJECTED: &str = "credential_rejected";
const WRITE_FAILED: &str = "credential_write_failed";
const LABEL: &str = "QKeyMapper MCP bridge";

pub struct Credential {
    pub pair_id: [u8; 16],
    pub epoch: NonZeroU64,
    pub scopes: u16,
    pub secret: Zeroizing<[u8; 32]>,
}

impl Credential {
    fn from_plain(data: &[u8]) -> Result<Self> {
        let rejected = NativeError::new(REJECTED);
        let mut value = decode(data, MAX_PLAINTEXT).map_err(|_| rejected)?;
        let Some(Value::String(secret)) = value.remove("secret") else {
            return Err(rejected);
        };
        let secret = Zeroizing::new(secret);
        if value.len() != 3 {
            return Err(rejected);
        }
        let field = |name| value.get(name).and_then(Value::as_str).ok_or(rejected);
        let pair_id = hex_bytes(field("pair_id")?).map_err(|_| rejected)?;
        let epoch = decimal(field("epoch")?, false)
            .ok()
            .and_then(NonZeroU64::new)
            .ok_or(rejected)?;
        let scopes = scope_bits(value.get("scopes").ok_or(rejected)?).map_err(|_| rejected)?;
        let secret = Zeroizing::new(hex_bytes(&secret).map_err(|_| rejected)?);
        Ok(Self {
            pair_id,
            epoch,
            scopes,
            secret,
        })
    }
}

pub struct CredentialStore {
    target: Vec<u16>,
    entropy: [u8; 32],
}

impl CredentialStore {
    pub fn legacy(discovery: &Path, executable: &Path, session: u32) -> Result<Self> {
        if !discovery.is_absolute() || !executable.is_absolute() {
            return Err(NativeError::new(CONTEXT));
        }
        let machine = machine_guid()?;
        let discovery = normcase(discovery)?;
        let executable = normcase(executable)?;
        let context =
            format!("QKeyMapper-MCP-Bridge/v1\0{machine}\0{discovery}\0{executable}\0{session}");
        Self::for_context(context.as_bytes(), false)
    }

    pub fn persistent(executable: &Path, session: u32) -> Result<(Self, PathBuf)> {
        let canonical = canonical_target(executable)?;
        let mut digest = Sha256::new();
        digest.update(b"QKeyMapper-MCP-App/v2\0");
        digest.update(canonical.as_bytes());
        let binding = digest.finalize();
        let path = local_app_data()?
            .join("QKeyMapper/MCP/targets-v2")
            .join(lower_hex(&binding))
            .join("sessions")
            .join(session.to_string())
            .join("instance.json");
        let mut context = format!("QKeyMapper-MCP-Bridge/v2\0{}\0", machine_guid()?).into_bytes();
        context.extend_from_slice(&binding);
        Ok((Self::for_context(&context, true)?, path))
    }

    fn for_context(context: &[u8], persistent: bool) -> Result<Self> {
        let entropy: [u8; 32] = Sha256::digest(context).into();
        let prefix = if persistent {
            "QKeyMapper.MCP.Bridge/v2/"
        } else {
            "QKeyMapper.MCP.Bridge/"
        };
        let target = wide(
            OsStr::new(&format!("{prefix}{}", lower_hex(&entropy))),
            CONTEXT,
        )?;
        Ok(Self { target, entropy })
    }

    pub fn load(&self) -> Result<Option<Credential>> {
        let mut credential = null_mut();
        if unsafe { CredReadW(self.target.as_ptr(), CRED_TYPE_GENERIC, 0, &mut credential) } == 0 {
            let error = NativeError::last("credential_read_failed");
            if error.win32 == Some(ERROR_NOT_FOUND) {
                return Ok(None);
            }
            return Err(error);
        }
        let credential = NonNull::new(credential).ok_or_else(|| NativeError::new(REJECTED))?;
        struct ReadCredential(NonNull<CREDENTIALW>);
        impl Drop for ReadCredential {
            fn drop(&mut self) {
                unsafe { CredFree(self.0.as_ptr().cast()) };
            }
        }
        let credential = ReadCredential(credential);
        // CredReadW owns the blob in the CREDENTIALW allocation until CredFree.
        let value = unsafe { credential.0.as_ref() };
        let width = value.CredentialBlobSize as usize;
        let rejected = NativeError::new(REJECTED);
        if value.Persist != CRED_PERSIST_LOCAL_MACHINE
            || value.CredentialBlob.is_null()
            || !(1..=MAX_CRED_BLOB).contains(&width)
            || !width.is_multiple_of(4)
        {
            return Err(rejected);
        }
        let bytes = unsafe { slice::from_raw_parts(value.CredentialBlob, width) };
        let (pairs, _) = bytes.as_chunks::<2>();
        let text = pairs
            .iter()
            .copied()
            .map(u16::from_le_bytes)
            .collect::<Vec<_>>();
        let text = String::from_utf16(&text).map_err(|_| rejected)?;
        let (pairs, remainder) = text.as_bytes().as_chunks::<2>();
        if !remainder.is_empty() {
            return Err(rejected);
        }
        let cipher = pairs
            .iter()
            .map(|pair| {
                let hex = std::str::from_utf8(pair).map_err(|_| rejected)?;
                hex_bytes::<1>(hex)
                    .map(|value| value[0])
                    .map_err(|_| rejected)
            })
            .collect::<Result<Vec<u8>>>()?;
        let plaintext = self.crypt(&cipher, false)?;
        Credential::from_plain(&plaintext).map(Some)
    }

    pub fn save(&self, credential: &Credential) -> Result<()> {
        #[derive(Serialize)]
        struct WireCredential<'a> {
            pair_id: String,
            epoch: String,
            scopes: u16,
            secret: &'a str,
        }
        scope_bits(&Value::from(credential.scopes)).map_err(|_| NativeError::new(WRITE_FAILED))?;
        let secret = Zeroizing::new(lower_hex(&credential.secret[..]));
        let plaintext = Zeroizing::new(
            serde_json::to_vec(&WireCredential {
                pair_id: lower_hex(&credential.pair_id),
                epoch: credential.epoch.to_string(),
                scopes: credential.scopes,
                secret: &secret,
            })
            .map_err(|_| NativeError::new(WRITE_FAILED))?,
        );
        let cipher = self.crypt(&plaintext, true)?;
        let mut text: Vec<u16> = lower_hex(&cipher).encode_utf16().collect();
        let size = size_of_val(text.as_slice());
        if !(1..=MAX_CRED_BLOB).contains(&size) {
            return Err(NativeError::new(WRITE_FAILED));
        }
        let mut target = self.target.clone();
        let mut label = wide(OsStr::new(LABEL), WRITE_FAILED)?;
        let value = CREDENTIALW {
            Type: CRED_TYPE_GENERIC,
            TargetName: target.as_mut_ptr(),
            CredentialBlobSize: size as u32,
            CredentialBlob: text.as_mut_ptr().cast(),
            Persist: CRED_PERSIST_LOCAL_MACHINE,
            UserName: label.as_mut_ptr(),
            ..Default::default()
        };
        if unsafe { CredWriteW(&value, 0) } == 0 {
            return Err(NativeError::last(WRITE_FAILED));
        }
        Ok(())
    }

    pub fn forget(&self) -> Result<()> {
        if unsafe { CredDeleteW(self.target.as_ptr(), CRED_TYPE_GENERIC, 0) } == 0 {
            let error = NativeError::last("credential_delete_failed");
            if error.win32 != Some(ERROR_NOT_FOUND) {
                return Err(error);
            }
        }
        Ok(())
    }

    fn crypt(&self, data: &[u8], protect: bool) -> Result<Zeroizing<Vec<u8>>> {
        let code = if protect { WRITE_FAILED } else { REJECTED };
        let input = CRYPT_INTEGER_BLOB {
            cbData: data.len() as u32,
            pbData: data.as_ptr().cast_mut(),
        };
        let entropy = CRYPT_INTEGER_BLOB {
            cbData: self.entropy.len() as u32,
            pbData: self.entropy.as_ptr().cast_mut(),
        };
        let mut output = CRYPT_INTEGER_BLOB::default();
        let ok = if protect {
            unsafe {
                CryptProtectData(
                    &input,
                    w!("QKeyMapper MCP bridge"),
                    &entropy,
                    null(),
                    null(),
                    CRYPTPROTECT_UI_FORBIDDEN,
                    &mut output,
                )
            }
        } else {
            unsafe {
                CryptUnprotectData(
                    &input,
                    null_mut(),
                    &entropy,
                    null(),
                    null(),
                    CRYPTPROTECT_UI_FORBIDDEN,
                    &mut output,
                )
            }
        };
        if ok == 0 {
            return Err(NativeError::last(code));
        }
        let allocation = NonNull::new(output.pbData.cast())
            .map(LocalAllocation)
            .ok_or_else(|| NativeError::new(code))?;
        let size = output.cbData as usize;
        // DPAPI owns this allocation. Zero the returned bytes before LocalFree.
        let result = unsafe { slice::from_raw_parts_mut(allocation.0.as_ptr().cast::<u8>(), size) };
        let limit = if protect {
            MAX_CRED_BLOB / 4
        } else {
            MAX_PLAINTEXT
        };
        let copy = (1..=limit)
            .contains(&size)
            .then(|| Zeroizing::new(result.to_vec()));
        result.zeroize();
        copy.ok_or_else(|| NativeError::new(code))
    }
}

fn machine_guid() -> Result<String> {
    let mut value = [0u16; 512];
    let mut bytes = size_of_val(&value) as u32;
    let status = unsafe {
        RegGetValueW(
            HKEY_LOCAL_MACHINE,
            w!(r"SOFTWARE\Microsoft\Cryptography"),
            w!("MachineGuid"),
            RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
            null_mut(),
            value.as_mut_ptr().cast(),
            &mut bytes,
        )
    };
    if status != 0 {
        return Err(NativeError::from_win32(CONTEXT, status));
    }
    let value = value
        .get(..bytes as usize / 2)
        .and_then(|v| v.strip_suffix(&[0]))
        .filter(|v| !v.is_empty() && !v.contains(&0) && bytes.is_multiple_of(2))
        .ok_or_else(|| NativeError::new(CONTEXT))?;
    String::from_utf16(value)
        .map(|value| value.to_lowercase())
        .map_err(|_| NativeError::new(CONTEXT))
}

fn canonical_target(executable: &Path) -> Result<String> {
    let handle = open_executable(executable)?;
    let mut buffer = vec![0u16; 32768];
    let length = unsafe {
        GetFinalPathNameByHandleW(
            handle.as_raw_handle(),
            buffer.as_mut_ptr(),
            buffer.len() as u32,
            FILE_NAME_NORMALIZED,
        )
    };
    if length == 0 || length as usize >= buffer.len() {
        return Err(NativeError::new(CONTEXT));
    }
    let path =
        String::from_utf16(&buffer[..length as usize]).map_err(|_| NativeError::new(CONTEXT))?;
    let path = if let Some(suffix) = path.strip_prefix("\\\\?\\UNC\\") {
        format!("//{suffix}")
    } else {
        path.strip_prefix("\\\\?\\")
            .ok_or_else(|| NativeError::new(CONTEXT))?
            .to_owned()
    }
    .replace('\\', "/");
    if !Path::new(&path).is_absolute() {
        return Err(NativeError::new(CONTEXT));
    }
    Ok(path)
}

fn local_app_data() -> Result<PathBuf> {
    let mut path = null_mut();
    let code = unsafe { SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, null_mut(), &mut path) };
    struct ShellPath(*mut u16);
    impl Drop for ShellPath {
        fn drop(&mut self) {
            unsafe { CoTaskMemFree(self.0.cast()) };
        }
    }
    let path = ShellPath(path);
    if code < 0 || path.0.is_null() {
        return Err(NativeError::new(CONTEXT));
    }
    // SHGetKnownFolderPath returns a NUL-terminated UTF-16 string allocated by COM.
    let length = (0..32768)
        .find(|&index| unsafe { *path.0.add(index) } == 0)
        .ok_or_else(|| NativeError::new(CONTEXT))?;
    let path = PathBuf::from(OsString::from_wide(unsafe {
        slice::from_raw_parts(path.0, length)
    }));
    if !path.is_absolute() {
        return Err(NativeError::new(CONTEXT));
    }
    Ok(path)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn legacy_and_persistent_key_spaces_differ() {
        let exe = std::env::current_exe().unwrap();
        let hint = exe.with_file_name("instance.json");
        let old = CredentialStore::legacy(&hint, &exe, 1).unwrap();
        let other = CredentialStore::legacy(&hint, &exe, 2).unwrap();
        let (one, first) = CredentialStore::persistent(&exe, 1).unwrap();
        let (two, second) = CredentialStore::persistent(&exe, 2).unwrap();
        assert_ne!(old.target, other.target);
        assert_ne!(old.target, one.target);
        assert_eq!(one.target, two.target);
        assert_ne!(first, second);
        assert_eq!(
            first.parent().unwrap().parent(),
            second.parent().unwrap().parent()
        );
    }

    #[test]
    fn loads_existing_credential_record_format() {
        let data = br#"{"pair_id":"0123456789abcdef0123456789abcdef","epoch":"18446744073709551615","scopes":129,"secret":"2525252525252525252525252525252525252525252525252525252525252525"}"#;
        let result = Credential::from_plain(data).unwrap();
        assert_eq!(
            result.pair_id,
            [
                0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
                0xcd, 0xef
            ]
        );
        assert_eq!(result.epoch, NonZeroU64::MAX);
        assert_eq!(result.scopes, 129);
        assert!(*result.secret == [0x25; 32]);
    }

    #[test]
    #[ignore = "writes and removes a dedicated synthetic Windows credential"]
    fn dedicated_credential_roundtrip() {
        use std::fs;

        let mut nonce = [0u8; 16];
        getrandom::fill(&mut nonce).unwrap();
        let fresh = std::env::temp_dir().join(format!("QkmRustFixture-{}", lower_hex(&nonce)));
        fs::create_dir(&fresh).unwrap();
        let executable = fresh.join("演示 Target 日本語.exe");
        let hint = fresh.join("instance.json");
        struct Files(PathBuf, PathBuf);
        impl Drop for Files {
            fn drop(&mut self) {
                let _ = fs::remove_file(&self.1);
                let _ = fs::remove_dir(&self.0);
            }
        }
        let files = Files(fresh, executable);
        fs::write(&files.1, b"synthetic credential context; never launched").unwrap();
        let legacy = CredentialStore::legacy(&hint, &files.1, 1).unwrap();
        let (persistent, _) = CredentialStore::persistent(&files.1, 1).unwrap();
        assert!(legacy.load().unwrap().is_none() && persistent.load().unwrap().is_none());
        struct Cleanup<'a>([&'a CredentialStore; 2]);
        impl Drop for Cleanup<'_> {
            fn drop(&mut self) {
                for store in self.0 {
                    let _ = store.forget();
                }
            }
        }
        let cleanup = Cleanup([&legacy, &persistent]);
        let credential = Credential {
            pair_id: nonce,
            epoch: NonZeroU64::MAX,
            scopes: 129,
            secret: Zeroizing::new([0x25; 32]),
        };
        for store in cleanup.0 {
            store.save(&credential).unwrap();
            let read = store.load().unwrap().unwrap();
            assert_eq!(read.pair_id, credential.pair_id);
            assert_eq!(
                (read.epoch, read.scopes),
                (credential.epoch, credential.scopes)
            );
            assert!(*read.secret == *credential.secret);
        }
        let wrong_entropy = CredentialStore {
            target: legacy.target.clone(),
            entropy: persistent.entropy,
        };
        assert!(
            matches!(wrong_entropy.load(), Err(error) if error.code == REJECTED && error.win32.is_some())
        );
        for store in cleanup.0 {
            store.forget().unwrap();
        }
        drop(cleanup);
        assert!(legacy.load().unwrap().is_none() && persistent.load().unwrap().is_none());
    }
}
