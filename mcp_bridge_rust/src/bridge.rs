use std::{
    io,
    sync::{Arc, Mutex, mpsc},
    thread::{self, JoinHandle},
};

use serde_json::{Value, json};
use tokio::sync::oneshot;

use crate::demo_scene::DemoSession;
use crate::native::{
    Cancel, Connection, DemoOptions, NativeError, Result, Target, require_pristine_thread,
};
use crate::wire::Object;

enum Operation {
    Connect { pair: bool },
    Request(&'static str, Object),
}

enum Command {
    Run(Operation, Cancel, oneshot::Sender<Result<Value>>),
    Disconnect,
    Stop,
}

struct State {
    status: Value,
    active: Option<Cancel>,
    closing: bool,
    stopped: bool,
}

#[derive(Clone)]
pub struct Bridge {
    state: Arc<Mutex<State>>,
    sender: mpsc::Sender<Command>,
}

pub struct Worker {
    pub client: Bridge,
    thread: Option<JoinHandle<()>>,
}

pub struct Pending {
    pub cancel: Cancel,
    pub reply: oneshot::Receiver<Result<Value>>,
}

impl Worker {
    pub fn start(target: Target, demo: Option<DemoOptions>) -> io::Result<Self> {
        let (sender, queue) = mpsc::channel();
        let state = Arc::new(Mutex::new(State {
            status: json!({"status": "disconnected"}),
            active: None,
            closing: false,
            stopped: false,
        }));
        let client = Bridge {
            state: state.clone(),
            sender,
        };
        let thread = thread::Builder::new()
            .name("qkm-pipe".to_owned())
            .spawn(move || run(target, demo, queue, state))?;
        Ok(Self {
            client,
            thread: Some(thread),
        })
    }

    pub fn join(mut self) -> thread::Result<()> {
        self.client.stop();
        self.thread.take().map(JoinHandle::join).unwrap_or(Ok(()))
    }
}

impl Drop for Worker {
    fn drop(&mut self) {
        if let Some(thread) = self.thread.take() {
            self.client.stop();
            let _ = thread.join();
        }
    }
}

impl Bridge {
    pub fn state(&self) -> Value {
        let inner = self.state.lock().unwrap();
        let mut result = inner.status.clone();
        result["busy"] = (inner.active.is_some() || inner.closing).into();
        result
    }

    fn submit(&self, operation: Operation) -> Result<Pending> {
        let mut state = self.state.lock().unwrap();
        if state.active.is_some() || state.closing || state.stopped {
            return Err(NativeError::new("busy_or_closed"));
        }
        let cancel = Cancel::new()?;
        let (sender, reply) = oneshot::channel();
        // Admission and publication are atomic with respect to disconnect and stop.
        state.active = Some(cancel.clone());
        if self
            .sender
            .send(Command::Run(operation, cancel.clone(), sender))
            .is_err()
        {
            state.active = None;
            state.stopped = true;
            return Err(NativeError::new("busy_or_closed"));
        }
        Ok(Pending { cancel, reply })
    }

    pub fn connect(&self, pair: bool) -> Value {
        let result = self.submit(Operation::Connect { pair });
        let mut state = self.state();
        state["accepted"] = result.is_ok().into();
        if let Err(error) = result {
            state["ok"] = false.into();
            state["error"] = error.code.into();
        }
        state
    }

    pub fn request(&self, method: &'static str, arguments: Object) -> Result<Pending> {
        if !matches!(
            method,
            "qkm_get_state"
                | "qkm_get_capabilities"
                | "qkm_get_help"
                | "qkm_query"
                | "qkm_get_operation"
                | "qkm_preview_changes"
                | "qkm_apply_changes"
                | "qkm_cancel_changes"
                | "qkm_open_mapping_editor"
                | "qkm_close_mapping_editor"
                | "qkm_open_mapping_settings"
                | "qkm_close_mapping_settings"
                | "qkm_capture_window"
                | "qkm_get_demo_state"
                | "qkm_demo_mapping"
        ) {
            return Err(NativeError::new("unsupported_method"));
        }
        self.submit(Operation::Request(method, arguments))
    }

    pub fn disconnect(&self) -> Value {
        self.close(false);
        json!({"accepted": true, "status": "disconnecting"})
    }

    pub fn stop(&self) {
        self.close(true);
    }

    fn close(&self, stop: bool) {
        let mut state = self.state.lock().unwrap();
        if state.stopped || (state.closing && !stop) {
            return;
        }
        state.closing = true;
        state.stopped = stop;
        if let Some(cancel) = &state.active {
            let _ = cancel.signal();
        }
        let command = if stop {
            Command::Stop
        } else {
            Command::Disconnect
        };
        if self.sender.send(command).is_err() {
            state.stopped = true;
        }
    }
}

fn run(
    target: Target,
    demo: Option<DemoOptions>,
    queue: mpsc::Receiver<Command>,
    state: Arc<Mutex<State>>,
) {
    let mut session = None;
    let mut scene = demo
        .as_ref()
        .filter(|options| options.enable_mapping)
        .map(|_| DemoSession::default());
    while let Ok(command) = queue.recv() {
        match command {
            Command::Run(operation, cancel, reply) => {
                let result = require_pristine_thread().and_then(|()| {
                    perform(
                        operation,
                        &target,
                        demo.as_ref(),
                        &cancel,
                        &state,
                        &mut session,
                        scene.as_mut(),
                    )
                });
                if let Err(error) = &result {
                    drop(session.take());
                    let status = if error.code == "cancelled" {
                        "disconnected"
                    } else {
                        "failed"
                    };
                    state.lock().unwrap().status = json!({"status": status, "error": error.code});
                }
                // Native work and its buffers are finished before the next job can enter.
                state.lock().unwrap().active = None;
                let _ = reply.send(result);
            }
            Command::Disconnect | Command::Stop => {
                drop(session.take());
                let mut inner = state.lock().unwrap();
                inner.status = json!({"status": "disconnected"});
                inner.closing = false;
                if matches!(command, Command::Stop) {
                    break;
                }
            }
        }
    }
}

fn perform(
    operation: Operation,
    target: &Target,
    demo: Option<&DemoOptions>,
    cancel: &Cancel,
    state: &Mutex<State>,
    session: &mut Option<Connection>,
    scene: Option<&mut DemoSession>,
) -> Result<Value> {
    match operation {
        Operation::Connect { pair } => {
            drop(session.take());
            state.lock().unwrap().status = json!({"status": "connecting"});
            *session = Connection::connect(target, pair, cancel, || {
                state.lock().unwrap().status = json!({"status": "awaiting_gui_approval"});
            })?;
            let status = session
                .as_ref()
                .map(Connection::info)
                .unwrap_or_else(|| json!({"status": "pairing_required"}));
            state.lock().unwrap().status = status.clone();
            Ok(status)
        }
        Operation::Request(method, arguments) => {
            let session = session
                .as_mut()
                .ok_or_else(|| NativeError::new("not_connected"))?;
            let value = match method {
                "qkm_get_demo_state" => {
                    let options = demo.ok_or_else(|| NativeError::new("demo_not_configured"))?;
                    let mut value =
                        session.demo_state(&options.record, &target.executable, cancel)?;
                    if let Some(scene) = scene {
                        value["result"]["demo_session"] = scene.status();
                    }
                    value
                }
                "qkm_demo_mapping" => {
                    let (Some(options), Some(scene)) = (demo, scene) else {
                        return Ok(json!({"ok":false, "error":"demo_not_enabled"}));
                    };
                    let binding = session.demo_binding(&options.record, &target.executable)?;
                    let granted = session.scopes();
                    scene.run(
                        &mut |method, args| session.call(method, args, cancel),
                        &binding,
                        granted,
                        &arguments,
                    )?
                }
                _ => session.call(method, arguments, cancel)?,
            };
            state.lock().unwrap().status = session.info();
            Ok(value)
        }
    }
}
