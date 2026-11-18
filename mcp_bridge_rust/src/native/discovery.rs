use std::{
    ffi::OsString,
    fs::File,
    io::Read,
    mem::{offset_of, size_of},
    os::windows::{
        ffi::OsStringExt,
        io::{AsRawHandle, OwnedHandle},
    },
    path::{Component, Path, PathBuf, Prefix},
    ptr::{NonNull, null, null_mut},
};

use serde_json::Value;
use windows_sys::{
    Win32::{
        Foundation::{ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, GENERIC_READ},
        Globalization::{LCMAP_LOWERCASE, LCMapStringEx},
        Security::{
            ACCESS_ALLOWED_ACE, ACE_HEADER,
            Authorization::{GetSecurityInfo, SE_FILE_OBJECT},
            CONTAINER_INHERIT_ACE, DACL_SECURITY_INFORMATION, GetAce, GetSecurityDescriptorControl,
            OBJECT_INHERIT_ACE, OWNER_SECURITY_INFORMATION, SE_DACL_PROTECTED,
        },
        Storage::FileSystem::{
            BY_HANDLE_FILE_INFORMATION, FILE_ALL_ACCESS, FILE_ATTRIBUTE_DIRECTORY,
            FILE_ATTRIBUTE_REPARSE_POINT, FILE_FLAG_BACKUP_SEMANTICS, FILE_FLAG_OPEN_REPARSE_POINT,
            FILE_ID_INFO, FILE_READ_ATTRIBUTES, FILE_SHARE_DELETE, FILE_SHARE_READ,
            FILE_SHARE_WRITE, FILE_TYPE_DISK, FileIdInfo, GetFileInformationByHandle,
            GetFileInformationByHandleEx, GetFileType, GetFullPathNameW, READ_CONTROL,
        },
        System::SystemServices::ACCESS_ALLOWED_ACE_TYPE,
    },
    w,
};

use super::{
    Identity, LocalAllocation, NativeError, Result, identity, identity::read_sid, open_file, wide,
};
use crate::wire::{decimal, decode, hex_bytes};

pub(super) const PIPE_PREFIX: &str = r"\\.\pipe\QKeyMapper.Automation.";
const DISCOVERY_LIMIT: usize = 128 * 1024;

pub struct Discovery {
    pub instance: [u8; 16],
    pub(super) pipe: String,
    pub server: Identity,
}

impl Discovery {
    pub fn pipe_name(&self) -> &str {
        &self.pipe
    }
}

fn absolute_file_path(path: &Path) -> bool {
    path.is_absolute()
        && matches!(
            path.components().next(),
            Some(Component::Prefix(prefix))
                if matches!(prefix.kind(), Prefix::Disk(_) | Prefix::VerbatimDisk(_)
                    | Prefix::UNC(..) | Prefix::VerbatimUNC(..))
        )
}

fn info(handle: &OwnedHandle, code: &'static str) -> Result<BY_HANDLE_FILE_INFORMATION> {
    let mut value = BY_HANDLE_FILE_INFORMATION::default();
    if unsafe { GetFileInformationByHandle(handle.as_raw_handle(), &mut value) } == 0 {
        return Err(NativeError::last(code));
    }
    Ok(value)
}

pub(super) fn open_executable(path: &Path) -> Result<OwnedHandle> {
    let code = "executable_rejected";
    if !absolute_file_path(path) {
        return Err(NativeError::new(code));
    }
    let handle = open_file(
        path.as_os_str(),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        0,
        code,
    )?;
    if unsafe { GetFileType(handle.as_raw_handle()) } != FILE_TYPE_DISK
        || info(&handle, code)?.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY != 0
    {
        return Err(NativeError::new(code));
    }
    Ok(handle)
}

pub fn file_identity(path: &Path) -> Result<(u64, [u8; 16])> {
    let handle = open_executable(path)?;
    let mut value = FILE_ID_INFO::default();
    if unsafe {
        GetFileInformationByHandleEx(
            handle.as_raw_handle(),
            FileIdInfo,
            (&mut value as *mut FILE_ID_INFO).cast(),
            size_of::<FILE_ID_INFO>() as u32,
        )
    } == 0
    {
        return Err(NativeError::last("executable_rejected"));
    }
    Ok((value.VolumeSerialNumber, value.FileId.Identifier))
}

fn open_private(path: &Path, own: &Identity, directory: bool) -> Result<OwnedHandle> {
    let rejected = "unsafe_discovery";
    let access = FILE_READ_ATTRIBUTES | READ_CONTROL | if directory { 0 } else { GENERIC_READ };
    let share = FILE_SHARE_READ | if directory { FILE_SHARE_WRITE } else { 0 };
    let flags = FILE_FLAG_OPEN_REPARSE_POINT
        | if directory {
            FILE_FLAG_BACKUP_SEMANTICS
        } else {
            0
        };
    let handle = open_file(path.as_os_str(), access, share, flags, "discovery_rejected").map_err(
        |error| {
            if matches!(
                error.win32,
                Some(ERROR_FILE_NOT_FOUND | ERROR_PATH_NOT_FOUND)
            ) {
                NativeError {
                    code: "application_unavailable",
                    ..error
                }
            } else {
                error
            }
        },
    )?;
    let file = info(&handle, "discovery_rejected")?;
    if file.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT != 0
        || (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY != 0) != directory
        || (!directory
            && (file.nNumberOfLinks != 1
                || unsafe { GetFileType(handle.as_raw_handle()) } != FILE_TYPE_DISK))
    {
        return Err(NativeError::new(rejected));
    }

    let (mut owner, mut dacl, mut descriptor) = (null_mut(), null_mut(), null_mut());
    let status = unsafe {
        GetSecurityInfo(
            handle.as_raw_handle(),
            SE_FILE_OBJECT,
            OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            &mut owner,
            null_mut(),
            &mut dacl,
            null_mut(),
            &mut descriptor,
        )
    };
    let _descriptor = NonNull::new(descriptor).map(LocalAllocation);
    if status != 0 {
        return Err(NativeError::from_win32("discovery_rejected", status));
    }
    // The security descriptor owns the owner SID and DACL pointers until LocalFree.
    let owner = unsafe { read_sid(owner) }.map_err(|_| NativeError::new(rejected))?;
    if owner != own.user_sid() || dacl.is_null() || unsafe { (*dacl).AceCount } != 1 {
        return Err(NativeError::new(rejected));
    }
    let mut ace = null_mut();
    if unsafe { GetAce(dacl, 0, &mut ace) } == 0 || ace.is_null() {
        return Err(NativeError::new(rejected));
    }
    let header = unsafe { ace.cast::<ACE_HEADER>().read_unaligned() };
    let sid_offset = offset_of!(ACCESS_ALLOWED_ACE, SidStart);
    let ace_size = header.AceSize as usize;
    if u32::from(header.AceType) != ACCESS_ALLOWED_ACE_TYPE || ace_size < sid_offset + 8 {
        return Err(NativeError::new(rejected));
    }
    // A variable-length SID starts after the mask. Its full length must fit in the ACE.
    let sid = unsafe { ace.cast::<u8>().add(sid_offset) };
    let sid_bytes = 8 + 4 * usize::from(unsafe { *sid.add(1) });
    let allow = unsafe { ace.cast::<ACCESS_ALLOWED_ACE>().read_unaligned() };
    if sid_offset + sid_bytes > ace_size
        || allow.Mask != FILE_ALL_ACCESS
        || unsafe { read_sid(sid.cast()) }.map_err(|_| NativeError::new(rejected))?
            != own.user_sid()
    {
        return Err(NativeError::new(rejected));
    }
    if directory {
        let (mut control, mut revision) = (0, 0);
        let inherit = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        if unsafe { GetSecurityDescriptorControl(descriptor, &mut control, &mut revision) } == 0
            || control & SE_DACL_PROTECTED == 0
            || u32::from(header.AceFlags) & inherit != inherit
        {
            return Err(NativeError::new(rejected));
        }
    }
    Ok(handle)
}

pub(super) fn normcase(path: &Path) -> Result<String> {
    let code = "executable_rejected";
    let mut source = wide(path.as_os_str(), code)?;
    source.pop();
    for unit in &mut source {
        if *unit == b'/' as u16 {
            *unit = b'\\' as u16;
        }
    }
    // CPython's Windows normcase uses the invariant locale for lowercase.
    let count = unsafe {
        LCMapStringEx(
            w!(""),
            LCMAP_LOWERCASE,
            source.as_ptr(),
            source.len() as i32,
            null_mut(),
            0,
            null(),
            null(),
            0,
        )
    };
    if !(1..32768).contains(&count) {
        return Err(NativeError::last(code));
    }
    let mut output = vec![0u16; count as usize];
    if unsafe {
        LCMapStringEx(
            w!(""),
            LCMAP_LOWERCASE,
            source.as_ptr(),
            source.len() as i32,
            output.as_mut_ptr(),
            count,
            null(),
            null(),
            0,
        )
    } != count
    {
        return Err(NativeError::last(code));
    }
    String::from_utf16(&output).map_err(|_| NativeError::new(code))
}

fn normalized(path: &Path) -> Result<String> {
    let code = "executable_rejected";
    if !absolute_file_path(path) {
        return Err(NativeError::new(code));
    }
    let input = wide(path.as_os_str(), code)?;
    let mut buffer = vec![0u16; 32768];
    let count = unsafe {
        GetFullPathNameW(
            input.as_ptr(),
            buffer.len() as u32,
            buffer.as_mut_ptr(),
            null_mut(),
        )
    };
    if count == 0 || count as usize >= buffer.len() {
        return Err(NativeError::new(code));
    }
    normcase(&PathBuf::from(OsString::from_wide(
        &buffer[..count as usize],
    )))
}

pub fn discover(path: &Path, expected_executable: &Path, own: &Identity) -> Result<Discovery> {
    let invalid = NativeError::new("invalid_discovery");
    if !absolute_file_path(path) || !absolute_file_path(expected_executable) {
        return Err(invalid);
    }
    let parent = path.parent().ok_or(invalid)?;
    let _directory = open_private(parent, own, true)?;
    let mut file = File::from(open_private(path, own, false)?);
    let length = file
        .metadata()
        .map_err(|_| NativeError::new("discovery_rejected"))?
        .len();
    if !(1..=DISCOVERY_LIMIT as u64).contains(&length) {
        return Err(invalid);
    }
    let mut bytes = vec![0u8; length as usize];
    file.read_exact(&mut bytes)
        .map_err(|_| NativeError::new("discovery_rejected"))?;
    let mut record = decode(&bytes, DISCOVERY_LIMIT).map_err(|_| invalid)?;
    if record.len() != 7 || record.get("version").and_then(Value::as_u64) != Some(1) {
        return Err(invalid);
    }
    let field = |name| record.get(name).and_then(Value::as_str).ok_or(invalid);
    let instance = hex_bytes::<16>(field("instance")?).map_err(|_| invalid)?;
    let pipe = field("pipe")?;
    if pipe.strip_prefix(PIPE_PREFIX) != Some(field("instance")?) {
        return Err(invalid);
    }
    let number = |name, zero| decimal(field(name)?, zero).map_err(|_| invalid);
    let pid = u32::try_from(number("pid", false)?).map_err(|_| invalid)?;
    let created = number("created", false)?;
    let session = u32::try_from(number("session", true)?).map_err(|_| invalid)?;
    let server = identity(pid)?;
    if !own.same_login(&server)
        || (server.stamp.created.get(), server.session) != (created, session)
    {
        return Err(NativeError::new("identity_rejected"));
    }
    // The path in the record is a hint. Only the OS process path and explicit EXE are opened.
    let executable = field("executable")?;
    let hint = Path::new(executable);
    if executable.contains('\0') || executable.chars().count() > 32768 || !absolute_file_path(hint)
    {
        return Err(invalid);
    }
    if normalized(hint)? != normalized(&server.executable)?
        || file_identity(&server.executable)? != file_identity(expected_executable)?
    {
        return Err(NativeError::new("executable_rejected"));
    }
    let pipe = match record.remove("pipe") {
        Some(Value::String(pipe)) => pipe,
        _ => return Err(invalid),
    };
    Ok(Discovery {
        instance,
        pipe,
        server,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn executable_identity_and_path_rules() {
        let a = std::env::current_exe().unwrap();
        let b = std::fs::canonicalize(&a).unwrap();
        assert_eq!(file_identity(&a).unwrap(), file_identity(&b).unwrap());
        assert_eq!(
            normcase(Path::new("D:/Demo/İ 日本語.EXE")).unwrap(),
            normcase(Path::new(r"D:\Demo\İ 日本語.EXE")).unwrap()
        );
        assert!(file_identity(Path::new(r"\\.\pipe\QKeyMapper.Automation.other")).is_err());
        assert!(file_identity(Path::new("relative.exe")).is_err());
        assert!(file_identity(a.parent().unwrap()).is_err());
    }
}
