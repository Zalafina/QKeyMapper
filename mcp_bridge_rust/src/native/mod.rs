use std::{
    ffi::{OsStr, c_void},
    fmt,
    os::windows::{
        ffi::OsStrExt,
        io::{FromRawHandle, OwnedHandle},
    },
    ptr::{NonNull, null, null_mut},
};

use windows_sys::Win32::{
    Foundation::{GetLastError, HANDLE, INVALID_HANDLE_VALUE, LocalFree},
    Storage::FileSystem::{CreateFileW, OPEN_EXISTING},
};

mod capture;
mod credentials;
pub(crate) mod demo;
mod discovery;
mod identity;
mod pipe;
mod session;

pub use credentials::{Credential, CredentialStore};
pub use demo::{DemoOptions, DemoRecord};
pub use discovery::{Discovery, discover, file_identity};
pub use identity::{Identity, identity, own_identity, require_pristine_thread};
pub use pipe::{Cancel, Pipe};
pub use session::{Connection, Target};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct NativeError {
    pub code: &'static str,
    pub win32: Option<u32>,
}

impl NativeError {
    pub(crate) fn new(code: &'static str) -> Self {
        Self { code, win32: None }
    }

    fn last(code: &'static str) -> Self {
        Self::from_win32(code, unsafe { GetLastError() })
    }

    fn from_win32(code: &'static str, win32: u32) -> Self {
        Self {
            code,
            win32: Some(win32),
        }
    }
}

impl fmt::Display for NativeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.code)
    }
}

impl std::error::Error for NativeError {}

pub(crate) type Result<T> = std::result::Result<T, NativeError>;

fn wide(text: &OsStr, code: &'static str) -> Result<Vec<u16>> {
    let mut encoded: Vec<u16> = text.encode_wide().collect();
    if encoded.is_empty() || encoded.len() >= 32768 || encoded.contains(&0) {
        return Err(NativeError::new(code));
    }
    encoded.push(0);
    Ok(encoded)
}

fn open_file(
    name: &OsStr,
    access: u32,
    share: u32,
    flags: u32,
    code: &'static str,
) -> Result<OwnedHandle> {
    let name = wide(name, code)?;
    let raw = unsafe {
        CreateFileW(
            name.as_ptr(),
            access,
            share,
            null(),
            OPEN_EXISTING,
            flags,
            null_mut(),
        )
    };
    // CreateFileW either transfers an owned handle or returns INVALID_HANDLE_VALUE.
    unsafe { take_handle(raw, code) }
}

/// The caller transfers ownership of a CloseHandle-compatible Win32 handle.
unsafe fn take_handle(raw: HANDLE, code: &'static str) -> Result<OwnedHandle> {
    if raw.is_null() || raw == INVALID_HANDLE_VALUE {
        return Err(NativeError::last(code));
    }
    Ok(unsafe { OwnedHandle::from_raw_handle(raw) })
}

struct LocalAllocation(NonNull<c_void>);

impl Drop for LocalAllocation {
    fn drop(&mut self) {
        // Only pointers allocated by Win32 APIs that specify LocalFree enter this type.
        unsafe { LocalFree(self.0.as_ptr()) };
    }
}
