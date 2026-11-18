use std::{
    ffi::OsString,
    mem::{offset_of, size_of},
    num::{NonZeroU32, NonZeroU64},
    os::windows::{ffi::OsStringExt, io::AsRawHandle},
    path::PathBuf,
    ptr::null_mut,
    slice,
};

use windows_sys::Win32::{
    Foundation::{ERROR_INSUFFICIENT_BUFFER, ERROR_NO_TOKEN, FILETIME, HANDLE, LUID, WAIT_TIMEOUT},
    Security::{
        GetLengthSid, GetTokenInformation, IsValidSid, PSID, SID_AND_ATTRIBUTES,
        TOKEN_ELEVATION_TYPE, TOKEN_GROUPS, TOKEN_INFORMATION_CLASS, TOKEN_LINKED_TOKEN,
        TOKEN_QUERY, TOKEN_STATISTICS, TOKEN_USER, TokenElevationType,
        TokenElevationTypeFull as ELEVATION_FULL, TokenElevationTypeLimited as ELEVATION_LIMITED,
        TokenGroups, TokenLinkedToken, TokenPrimary, TokenSessionId, TokenStatistics, TokenType,
        TokenUser,
    },
    System::{
        SystemServices::SE_GROUP_LOGON_ID,
        Threading::{
            GetCurrentThread, GetProcessTimes, OpenProcess, OpenProcessToken, OpenThreadToken,
            PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_SYNCHRONIZE, QueryFullProcessImageNameW,
            WaitForSingleObject,
        },
    },
};

use super::{NativeError, Result, take_handle};
use crate::auth::ProcessStamp;

const ERROR: &str = "identity_rejected";
const GROUP_LOGON: u32 = SE_GROUP_LOGON_ID as u32;

#[derive(PartialEq, Eq)]
pub struct Identity {
    pub stamp: ProcessStamp,
    pub session: u32,
    pub executable: PathBuf,
    user: Vec<u8>,
    logon: Vec<u8>,
    authentication: NonZeroU64,
    linked: Option<NonZeroU64>,
}

impl Identity {
    pub fn same_login(&self, other: &Self) -> bool {
        self.user == other.user
            && self.logon == other.logon
            && self.session == other.session
            && (self.authentication == other.authentication
                || (self.linked == Some(other.authentication)
                    && other.linked == Some(self.authentication)))
    }

    pub(super) fn user_sid(&self) -> &[u8] {
        &self.user
    }
}

struct TokenBuffer {
    words: Vec<usize>,
    bytes: usize,
}

impl TokenBuffer {
    fn get(token: HANDLE, class: TOKEN_INFORMATION_CLASS) -> Result<Self> {
        let mut bytes = 0u32;
        // First query the length. The second call writes into storage aligned for token structs.
        let found = unsafe { GetTokenInformation(token, class, null_mut(), 0, &mut bytes) };
        let error = NativeError::last(ERROR);
        if found != 0
            || error.win32 != Some(ERROR_INSUFFICIENT_BUFFER)
            || !(1..=1_048_576).contains(&bytes)
        {
            return Err(error);
        }
        let capacity = bytes;
        let mut words = vec![0usize; (bytes as usize).div_ceil(size_of::<usize>())];
        if unsafe {
            GetTokenInformation(
                token,
                class,
                words.as_mut_ptr().cast(),
                capacity,
                &mut bytes,
            )
        } == 0
        {
            return Err(NativeError::last(ERROR));
        }
        if bytes == 0 || bytes > capacity {
            return Err(NativeError::new(ERROR));
        }
        Ok(Self {
            words,
            bytes: bytes as usize,
        })
    }

    /// T must be the output type of the queried token class.
    unsafe fn read<T: Copy>(&self) -> Result<T> {
        if self.bytes < size_of::<T>() || std::mem::align_of::<T>() > std::mem::align_of::<usize>()
        {
            return Err(NativeError::new(ERROR));
        }
        Ok(unsafe { self.words.as_ptr().cast::<T>().read() })
    }

    fn logon_sid(&self) -> Result<Vec<u8>> {
        let count = unsafe { self.read::<TOKEN_GROUPS>() }?.GroupCount as usize;
        let offset = offset_of!(TOKEN_GROUPS, Groups);
        if count > (self.bytes - offset) / size_of::<SID_AND_ATTRIBUTES>() {
            return Err(NativeError::new(ERROR));
        }
        let base = self.words.as_ptr().cast::<u8>();
        // The returned group count is bounded by the Win32 token buffer length.
        let groups =
            unsafe { slice::from_raw_parts(base.add(offset).cast::<SID_AND_ATTRIBUTES>(), count) };
        let mut logons = groups
            .iter()
            .filter(|group| group.Attributes & GROUP_LOGON == GROUP_LOGON);
        let sid = logons.next().ok_or_else(|| NativeError::new(ERROR))?.Sid;
        if logons.next().is_some() {
            return Err(NativeError::new(ERROR));
        }
        // The token buffer retains the storage for each returned SID.
        unsafe { read_sid(sid) }
    }
}

/// The SID must be a pointer returned by a Win32 token or security descriptor query.
pub(super) unsafe fn read_sid(sid: PSID) -> Result<Vec<u8>> {
    if sid.is_null() || unsafe { IsValidSid(sid) } == 0 {
        return Err(NativeError::new(ERROR));
    }
    let len = unsafe { GetLengthSid(sid) } as usize;
    Ok(unsafe { slice::from_raw_parts(sid.cast::<u8>(), len) }.to_vec())
}

fn luid(value: LUID) -> Option<NonZeroU64> {
    NonZeroU64::new((u64::from(value.HighPart as u32) << 32) | u64::from(value.LowPart))
}

struct Login {
    user: Vec<u8>,
    logon: Vec<u8>,
    session: u32,
    authentication: NonZeroU64,
}

fn token_login(token: HANDLE) -> Result<Login> {
    let user_buf = TokenBuffer::get(token, TokenUser)?;
    // Each read uses the type returned by the corresponding token class.
    let user = unsafe { read_sid(user_buf.read::<TOKEN_USER>()?.User.Sid) }?;
    let logon = TokenBuffer::get(token, TokenGroups)?.logon_sid()?;
    let session = unsafe { TokenBuffer::get(token, TokenSessionId)?.read::<u32>() }?;
    let stats = unsafe { TokenBuffer::get(token, TokenStatistics)?.read::<TOKEN_STATISTICS>() }?;
    let authentication = luid(stats.AuthenticationId).ok_or_else(|| NativeError::new(ERROR))?;
    Ok(Login {
        user,
        logon,
        session,
        authentication,
    })
}

fn linked_login(token: HANDLE, login: &Login) -> Option<NonZeroU64> {
    // A failed link query leaves only exact authentication-ID matching available.
    let kind = unsafe {
        TokenBuffer::get(token, TokenElevationType)
            .ok()?
            .read::<TOKEN_ELEVATION_TYPE>()
            .ok()?
    };
    let opposite = match kind {
        ELEVATION_FULL => ELEVATION_LIMITED,
        ELEVATION_LIMITED => ELEVATION_FULL,
        _ => return None,
    };
    let mut link = TOKEN_LINKED_TOKEN::default();
    let mut bytes = 0;
    // TokenLinkedToken returns ERROR_BAD_LENGTH on Windows for a size-only query.
    if unsafe {
        GetTokenInformation(
            token,
            TokenLinkedToken,
            (&mut link as *mut TOKEN_LINKED_TOKEN).cast(),
            size_of::<TOKEN_LINKED_TOKEN>() as u32,
            &mut bytes,
        )
    } == 0
    {
        return None;
    }
    let linked = unsafe { take_handle(link.LinkedToken, ERROR) }.ok()?;
    if bytes as usize != size_of::<TOKEN_LINKED_TOKEN>() {
        return None;
    }
    let handle = linked.as_raw_handle();
    if unsafe {
        TokenBuffer::get(handle, TokenElevationType)
            .ok()?
            .read::<TOKEN_ELEVATION_TYPE>()
            .ok()?
    } != opposite
    {
        return None;
    }
    let other = token_login(handle).ok()?;
    (login.user == other.user && login.logon == other.logon && login.session == other.session)
        .then_some(other.authentication)
}

pub fn require_pristine_thread() -> Result<()> {
    let mut raw = null_mut();
    if unsafe { OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, 1, &mut raw) } != 0 {
        drop(unsafe { take_handle(raw, "thread_context_rejected") }?);
        return Err(NativeError::new("thread_context_rejected"));
    }
    let error = NativeError::last("thread_context_rejected");
    if error.win32 != Some(ERROR_NO_TOKEN) {
        return Err(error);
    }
    Ok(())
}

pub fn own_identity() -> Result<Identity> {
    identity(std::process::id())
}

pub fn identity(pid: u32) -> Result<Identity> {
    let pid = NonZeroU32::new(pid).ok_or_else(|| NativeError::new(ERROR))?;
    let process = unsafe {
        take_handle(
            OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SYNCHRONIZE,
                0,
                pid.get(),
            ),
            ERROR,
        )
    }?;
    let handle = process.as_raw_handle();
    if unsafe { WaitForSingleObject(handle, 0) } != WAIT_TIMEOUT {
        return Err(NativeError::new(ERROR));
    }
    let [mut created, mut exited, mut kernel, mut user] = [FILETIME::default(); 4];
    if unsafe { GetProcessTimes(handle, &mut created, &mut exited, &mut kernel, &mut user) } == 0 {
        return Err(NativeError::last(ERROR));
    }
    let time = (u64::from(created.dwHighDateTime) << 32) | u64::from(created.dwLowDateTime);
    let created = NonZeroU64::new(time).ok_or_else(|| NativeError::new(ERROR))?;
    let mut path = vec![0u16; 32768];
    let mut length = path.len() as u32;
    if unsafe { QueryFullProcessImageNameW(handle, 0, path.as_mut_ptr(), &mut length) } == 0 {
        return Err(NativeError::last(ERROR));
    }
    if length == 0 || length as usize >= path.len() {
        return Err(NativeError::new(ERROR));
    }
    let executable = PathBuf::from(OsString::from_wide(&path[..length as usize]));
    let mut raw = null_mut();
    if unsafe { OpenProcessToken(handle, TOKEN_QUERY, &mut raw) } == 0 {
        return Err(NativeError::last(ERROR));
    }
    let token = unsafe { take_handle(raw, ERROR) }?;
    let token = token.as_raw_handle();
    if unsafe { TokenBuffer::get(token, TokenType)?.read::<i32>() }? != TokenPrimary {
        return Err(NativeError::new(ERROR));
    }
    let login = token_login(token)?;
    let linked = linked_login(token, &login);
    if unsafe { WaitForSingleObject(handle, 0) } != WAIT_TIMEOUT {
        return Err(NativeError::new(ERROR));
    }
    Ok(Identity {
        stamp: ProcessStamp { pid, created },
        session: login.session,
        executable,
        user: login.user,
        logon: login.logon,
        authentication: login.authentication,
        linked,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[ignore = "compares this process with the locked Windows Python implementation"]
    fn current_link_query_matches_python() {
        use std::{path::Path, process::Command};
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap();
        let code = "import sys; from qkeymapper_mcp.windows import identity; \
                    print(int(bool(identity(int(sys.argv[1])).linked_authentication)))";
        let result = Command::new(root.join("out/mcp_bridge/venv/Scripts/python.exe"))
            .env("PYTHONPATH", root.join("mcp_bridge/src"))
            .args(["-B", "-X", "utf8", "-c", code])
            .arg(std::process::id().to_string())
            .output()
            .unwrap();
        assert!(result.status.success(), "Python identity helper failed");
        let expected = std::str::from_utf8(&result.stdout).unwrap().trim() == "1";
        assert_eq!(own_identity().unwrap().linked.is_some(), expected);
    }

    #[test]
    fn verifies_live_process_and_reciprocal_login() {
        require_pristine_thread().unwrap();
        let first = own_identity().unwrap();
        let mut second = identity(first.stamp.pid.get()).unwrap();
        assert!(first == second);
        assert!(first.same_login(&second));
        let mut first = first;
        second.authentication = NonZeroU64::new(first.authentication.get() ^ u64::MAX).unwrap();
        assert!(!first.same_login(&second));
        first.linked = Some(second.authentication);
        assert!(!first.same_login(&second));
        second.linked = Some(first.authentication);
        assert!(first.same_login(&second));
        second.session ^= 1;
        assert!(!first.same_login(&second));
        assert!(identity(0).is_err());
    }
}
