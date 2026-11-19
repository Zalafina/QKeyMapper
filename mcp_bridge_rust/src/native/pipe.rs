use std::{
    ffi::OsStr,
    os::windows::io::{AsRawHandle, OwnedHandle},
    ptr::{null, null_mut},
    sync::Arc,
    time::Instant,
};

use windows_sys::Win32::{
    Foundation::{
        ERROR_IO_INCOMPLETE, ERROR_IO_PENDING, ERROR_PIPE_BUSY, ERROR_SEM_TIMEOUT, HANDLE,
        WAIT_OBJECT_0, WAIT_TIMEOUT,
    },
    Storage::FileSystem::{
        FILE_FLAG_OVERLAPPED, ReadFile, SECURITY_IDENTIFICATION, SECURITY_SQOS_PRESENT, WriteFile,
    },
    System::{
        IO::{CancelIoEx, GetOverlappedResult, OVERLAPPED},
        Pipes::{GetNamedPipeServerProcessId, GetNamedPipeServerSessionId, WaitNamedPipeW},
        Threading::{CreateEventW, SetEvent, WaitForMultipleObjects, WaitForSingleObject},
    },
};

use super::{Discovery, Identity, NativeError, Result, identity, open_file, take_handle, wide};
use crate::wire::{Object, decode, encode, frame_length};

const PIPE_ACCESS: u32 = 0x0012019b;
const CANCEL_EVENT_INDEX: u32 = WAIT_OBJECT_0 + 1;
const CLOSED: &str = "connection_closed";

fn event() -> Result<OwnedHandle> {
    // A fresh manual-reset event belongs to this job or to one overlapped call.
    unsafe { take_handle(CreateEventW(null(), 1, 0, null()), "event_unavailable") }
}

#[derive(Clone)]
pub struct Cancel(Arc<OwnedHandle>);

impl Cancel {
    pub fn new() -> Result<Self> {
        Ok(Self(Arc::new(event()?)))
    }

    pub fn signal(&self) -> Result<()> {
        if unsafe { SetEvent(self.0.as_raw_handle()) } == 0 {
            return Err(NativeError::last("cancel_signal_failed"));
        }
        Ok(())
    }

    pub(crate) fn wait(&self, milliseconds: u32) -> Result<()> {
        match unsafe { WaitForSingleObject(self.0.as_raw_handle(), milliseconds) } {
            WAIT_OBJECT_0 => Err(NativeError::new("cancelled")),
            WAIT_TIMEOUT => Ok(()),
            _ => Err(NativeError::last("cancel_wait_failed")),
        }
    }

    fn remaining(&self, deadline: Instant) -> Result<u32> {
        self.wait(0)?;
        let time = deadline.saturating_duration_since(Instant::now());
        if time.is_zero() {
            return Err(NativeError::new("timeout"));
        }
        Ok(time.as_millis().clamp(1, u128::from(u32::MAX - 1)) as u32)
    }
}

pub struct Pipe {
    handle: OwnedHandle,
    target: Discovery,
    own: Identity,
}

enum Transfer<'a> {
    Read(&'a mut [u8]),
    Write(&'a [u8]),
}

impl Pipe {
    pub fn open(
        target: Discovery,
        own: Identity,
        cancel: &Cancel,
        deadline: Instant,
    ) -> Result<Self> {
        let name = OsStr::new(target.pipe_name());
        let wide_name = wide(name, "pipe_unavailable")?;
        let mut was_busy = false;
        let handle = loop {
            let remaining = cancel.remaining(deadline).map_err(|error| {
                if was_busy && error.code == "timeout" {
                    NativeError::new("pipe_busy")
                } else {
                    error
                }
            })?;
            match open_file(
                name,
                PIPE_ACCESS,
                0,
                FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                "pipe_unavailable",
            ) {
                Ok(handle) => break handle,
                Err(error) if error.win32 == Some(ERROR_PIPE_BUSY) => {
                    was_busy = true;
                    if unsafe { WaitNamedPipeW(wide_name.as_ptr(), remaining.min(100)) } == 0 {
                        let error = NativeError::last("pipe_unavailable");
                        if !matches!(error.win32, Some(ERROR_PIPE_BUSY | ERROR_SEM_TIMEOUT)) {
                            return Err(error);
                        }
                    }
                }
                Err(error) => return Err(error),
            }
        };
        let pipe = Self {
            handle,
            target,
            own,
        };
        pipe.verify()?;
        Ok(pipe)
    }

    pub fn target(&self) -> &Discovery {
        &self.target
    }

    pub fn verify(&self) -> Result<()> {
        let raw = self.handle.as_raw_handle();
        let (mut pid, mut session) = (0, 0);
        let code = "identity_rejected";
        if unsafe { GetNamedPipeServerProcessId(raw, &mut pid) } == 0
            || unsafe { GetNamedPipeServerSessionId(raw, &mut session) } == 0
        {
            return Err(NativeError::last(code));
        }
        let actual = identity(pid)?;
        if actual != self.target.server
            || session != actual.session
            || !self.own.same_login(&actual)
        {
            return Err(NativeError::new(code));
        }
        Ok(())
    }

    pub fn send(
        &mut self,
        message: &Object,
        limit: usize,
        cancel: &Cancel,
        deadline: Instant,
    ) -> Result<()> {
        self.verify()?;
        let frame = encode(message, limit).map_err(|_| NativeError::new("invalid_message"))?;
        let mut offset = 0;
        while offset < frame.len() {
            offset += self.transfer(Transfer::Write(&frame[offset..]), cancel, deadline)?;
        }
        Ok(())
    }

    pub fn receive(&mut self, limit: usize, cancel: &Cancel, deadline: Instant) -> Result<Object> {
        let mut prefix = [0u8; 4];
        self.read_exact(&mut prefix, cancel, deadline)?;
        let size = frame_length(prefix, limit).map_err(|_| NativeError::new("invalid_message"))?;
        let mut data = vec![0u8; size];
        self.read_exact(&mut data, cancel, deadline)?;
        let object = decode(&data, limit).map_err(|_| NativeError::new("invalid_message"))?;
        self.verify()?;
        Ok(object)
    }

    fn read_exact(&mut self, buffer: &mut [u8], cancel: &Cancel, deadline: Instant) -> Result<()> {
        let mut offset = 0;
        while offset < buffer.len() {
            offset += self.transfer(Transfer::Read(&mut buffer[offset..]), cancel, deadline)?;
        }
        Ok(())
    }

    fn transfer(
        &mut self,
        buffer: Transfer<'_>,
        cancel: &Cancel,
        deadline: Instant,
    ) -> Result<usize> {
        cancel.remaining(deadline)?;
        let ready = event()?;
        let mut ov = OVERLAPPED {
            hEvent: ready.as_raw_handle(),
            ..Default::default()
        };
        let handle = self.handle.as_raw_handle();
        // Buffer, OVERLAPPED and event remain live until completion is observed below.
        let (started, length) = match buffer {
            Transfer::Read(data) => (
                unsafe {
                    ReadFile(
                        handle,
                        data.as_mut_ptr(),
                        data.len() as u32,
                        null_mut(),
                        &mut ov,
                    )
                },
                data.len(),
            ),
            Transfer::Write(data) => (
                unsafe {
                    WriteFile(
                        handle,
                        data.as_ptr(),
                        data.len() as u32,
                        null_mut(),
                        &mut ov,
                    )
                },
                data.len(),
            ),
        };
        if started == 0 {
            let error = NativeError::last(CLOSED);
            if error.win32 != Some(ERROR_IO_PENDING) {
                return Err(error);
            }
            let wait = cancel.remaining(deadline).and_then(|timeout| {
                let handles = [ready.as_raw_handle(), cancel.0.as_raw_handle()];
                match unsafe { WaitForMultipleObjects(2, handles.as_ptr(), 0, timeout) } {
                    WAIT_OBJECT_0 => Ok(()),
                    CANCEL_EVENT_INDEX => Err(NativeError::new("cancelled")),
                    WAIT_TIMEOUT => Err(NativeError::new("timeout")),
                    _ => Err(NativeError::last(CLOSED)),
                }
            });
            if let Err(mut error) = wait {
                let drained = cancel_and_drain(handle, &ov);
                error.win32 = error.win32.or(drained);
                return Err(error);
            }
        }
        let mut transferred = 0;
        if unsafe { GetOverlappedResult(handle, &ov, &mut transferred, 0) } == 0 {
            let error = NativeError::last(CLOSED);
            if error.win32 == Some(ERROR_IO_INCOMPLETE) {
                cancel_and_drain(handle, &ov);
            }
            return Err(error);
        }
        if !(1..=length).contains(&(transferred as usize)) {
            return Err(NativeError::new(CLOSED));
        }
        Ok(transferred as usize)
    }
}

fn cancel_and_drain(handle: HANDLE, ov: &OVERLAPPED) -> Option<u32> {
    let mut bytes = 0;
    // Completion can win the cancellation race. In either case wait for the actual result.
    unsafe { CancelIoEx(handle, ov) };
    if unsafe { GetOverlappedResult(handle, ov, &mut bytes, 1) } == 0 {
        NativeError::last(CLOSED).win32
    } else {
        None
    }
}

#[cfg(test)]
mod tests {
    use std::{thread, time::Duration};
    use windows_sys::Win32::{
        Foundation::ERROR_OPERATION_ABORTED,
        Storage::FileSystem::{FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_ACCESS_DUPLEX},
        System::Pipes::{CreateNamedPipeW, PIPE_REJECT_REMOTE_CLIENTS, PIPE_TYPE_BYTE},
    };

    use super::*;
    use crate::{
        native::{discovery::PIPE_PREFIX, own_identity},
        wire::MAX_HANDSHAKE,
    };

    #[test]
    fn drains_cancelled_and_timed_out_native_reads() {
        let stamp = own_identity().unwrap().stamp;
        let instance = (u128::from(stamp.pid.get()) << 64) | u128::from(stamp.created.get());
        let name = format!("{PIPE_PREFIX}{instance:032x}");
        let wide_name = wide(OsStr::new(&name), "fixture_pipe_failed").unwrap();
        let server = unsafe {
            take_handle(
                CreateNamedPipeW(
                    wide_name.as_ptr(),
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                    PIPE_TYPE_BYTE | PIPE_REJECT_REMOTE_CLIENTS,
                    1,
                    4096,
                    4096,
                    0,
                    null(),
                ),
                "fixture_pipe_failed",
            )
        }
        .unwrap();
        let target = Discovery {
            instance: instance.to_be_bytes(),
            pipe: name,
            server: own_identity().unwrap(),
        };
        let cancel = Cancel::new().unwrap();
        let mut client = Pipe::open(
            target,
            own_identity().unwrap(),
            &cancel,
            Instant::now() + Duration::from_secs(2),
        )
        .unwrap();
        let timeout = client
            .receive(
                MAX_HANDSHAKE,
                &cancel,
                Instant::now() + Duration::from_millis(60),
            )
            .unwrap_err();
        assert_eq!(timeout.code, "timeout");
        assert_eq!(timeout.win32, Some(ERROR_OPERATION_ABORTED));

        let write_fragment = |data: &[u8]| {
            let mut count = 0;
            assert_ne!(
                unsafe {
                    WriteFile(
                        server.as_raw_handle(),
                        data.as_ptr(),
                        data.len() as u32,
                        &mut count,
                        null_mut(),
                    )
                },
                0
            );
            assert_eq!(count as usize, data.len());
        };
        let sample = serde_json::json!({"kind": "read", "value": "中文"});
        let object = sample.as_object().unwrap();
        write_fragment(&encode(object, MAX_HANDSHAKE).unwrap());
        assert_eq!(
            &client
                .receive(
                    MAX_HANDSHAKE,
                    &cancel,
                    Instant::now() + Duration::from_secs(1)
                )
                .unwrap(),
            object
        );

        // A prefix interrupted after two bytes must drain the second native read.
        write_fragment(&[0, 0]);
        let signal = cancel.clone();
        let worker = thread::spawn(move || {
            thread::sleep(Duration::from_millis(100));
            signal.signal().unwrap();
        });
        let cancelled = client
            .receive(
                MAX_HANDSHAKE,
                &cancel,
                Instant::now() + Duration::from_secs(2),
            )
            .unwrap_err();
        worker.join().unwrap();
        assert_eq!(cancelled.code, "cancelled");
        assert_eq!(cancelled.win32, Some(ERROR_OPERATION_ABORTED));
        drop(client);
        drop(server);
    }
}
