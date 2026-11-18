use std::{
    num::NonZeroU64,
    path::PathBuf,
    time::{Duration, Instant},
};

use serde_json::{Value, json};
use zeroize::Zeroizing;

use super::{
    Cancel, Credential, CredentialStore, NativeError, Pipe, Result, discover, own_identity,
};
use crate::{
    auth::{AuthBinding, ProcessStamp, ProofRole},
    wire::{MAX_FRAME, MAX_HANDSHAKE, Object, decimal, hex_bytes, lower_hex, scope_bits},
};

const SHORT: Duration = Duration::from_secs(3);
const PAIR_WAIT: Duration = Duration::from_secs(62);
const CALL_WAIT: Duration = Duration::from_secs(10);

pub struct Target {
    pub executable: PathBuf,
    pub discovery: Option<PathBuf>,
}

pub struct Connection {
    pipe: Pipe,
    credential: Credential,
}

fn invalid() -> NativeError {
    NativeError::new("invalid_message")
}

fn object(value: Value) -> Object {
    match value {
        Value::Object(value) => value,
        _ => unreachable!("fixed IPC message always has an object payload"),
    }
}

fn string<'a>(value: &'a Object, key: &str) -> Result<&'a str> {
    value.get(key).and_then(Value::as_str).ok_or_else(invalid)
}

fn number(value: &Object, key: &str) -> Result<u64> {
    decimal(string(value, key)?, false).map_err(|_| invalid())
}

fn scope(value: &Object) -> Result<u16> {
    scope_bits(value.get("scopes").ok_or_else(invalid)?).map_err(|_| invalid())
}

fn nonce() -> Result<[u8; 32]> {
    let mut nonce = [0; 32];
    getrandom::fill(&mut nonce).map_err(|_| NativeError::new("random_unavailable"))?;
    Ok(nonce)
}

fn matches_stamp(value: &Object, pid: &str, created: &str, stamp: ProcessStamp) -> Result<bool> {
    Ok(number(value, pid)? == u64::from(stamp.pid.get())
        && number(value, created)? == stamp.created.get())
}

impl Connection {
    pub fn connect(
        target: &Target,
        pair: bool,
        cancel: &Cancel,
        awaiting_approval: impl FnOnce(),
    ) -> Result<Option<Self>> {
        let own = own_identity()?;
        let (store, discovery_path) = match &target.discovery {
            Some(path) => (
                CredentialStore::legacy(path, &target.executable, own.session)?,
                path.clone(),
            ),
            None => CredentialStore::persistent(&target.executable, own.session)?,
        };
        let discovery = discover(&discovery_path, &target.executable, &own)?;
        let mut loaded = if pair { None } else { store.load()? };
        if !pair && loaded.is_none() {
            return Ok(None);
        }

        let client_stamp = own.stamp;
        let server_stamp = discovery.server.stamp;
        let instance = discovery.instance;
        let instance_text = lower_hex(&instance);
        let mut pipe = Pipe::open(discovery, own, cancel, Instant::now() + SHORT)?;
        let client_nonce = nonce()?;
        let mut hello = object(json!({
            "type": if pair { "pair.request" } else { "hello" },
            "version": 1, "instance": instance_text, "client_nonce": lower_hex(&client_nonce)
        }));
        if let Some(credential) = &loaded {
            hello.insert("pair_id".to_owned(), lower_hex(&credential.pair_id).into());
        } else {
            hello.insert("client_name".to_owned(), "QKeyMapper MCP bridge".into());
        }
        pipe.send(&hello, MAX_HANDSHAKE, cancel, Instant::now() + SHORT)?;
        if pair {
            awaiting_approval();
        }
        let mut challenge = pipe.receive(
            MAX_HANDSHAKE,
            cancel,
            Instant::now() + if pair { PAIR_WAIT } else { SHORT },
        )?;
        let deadline = Instant::now() + SHORT;
        if challenge.len() != if pair { 13 } else { 11 }
            || string(&challenge, "type")? != if pair { "pair.grant" } else { "challenge" }
            || challenge.get("version").and_then(Value::as_u64) != Some(1)
            || string(&challenge, "instance")? != instance_text
        {
            return Err(invalid());
        }
        if !matches_stamp(&challenge, "client_pid", "client_created", client_stamp)?
            || !matches_stamp(&challenge, "server_pid", "server_created", server_stamp)?
        {
            return Err(NativeError::new("identity_rejected"));
        }
        let epoch = NonZeroU64::new(number(&challenge, "epoch")?).ok_or_else(invalid)?;
        let pair_id = hex_bytes(string(&challenge, "pair_id")?).map_err(|_| invalid())?;
        let server_nonce = hex_bytes(string(&challenge, "server_nonce")?).map_err(|_| invalid())?;
        let server_proof = hex_bytes(string(&challenge, "server_proof")?).map_err(|_| invalid())?;
        let mut credential = if pair {
            let Some(Value::String(secret)) = challenge.remove("secret") else {
                return Err(invalid());
            };
            let secret = Zeroizing::new(secret);
            Credential {
                pair_id,
                epoch,
                scopes: scope(&challenge)?,
                secret: Zeroizing::new(hex_bytes(&secret).map_err(|_| invalid())?),
            }
        } else {
            loaded.take().ok_or_else(invalid)?
        };
        if credential.pair_id != pair_id || epoch < credential.epoch {
            return Err(NativeError::new("credential_rejected"));
        }
        let binding = AuthBinding {
            instance: &instance,
            pair_id: &pair_id,
            epoch,
            client: client_stamp,
            server: server_stamp,
            client_nonce: &client_nonce,
            server_nonce: &server_nonce,
        };
        if !binding.verify(ProofRole::Server, &credential.secret, &server_proof) {
            return Err(NativeError::new("server_proof_rejected"));
        }
        if pair {
            // The product may accept confirmation only after this client key is durable.
            store.save(&credential)?;
        }
        let role = if pair {
            ProofRole::PairingConfirmation
        } else {
            ProofRole::Client
        };
        let confirmation = object(json!({
            "type": if pair { "pair.confirm" } else { "proof" },
            "proof": lower_hex(&binding.proof(role, &credential.secret)),
        }));
        pipe.send(&confirmation, MAX_HANDSHAKE, cancel, deadline)?;
        let ready = pipe.receive(MAX_HANDSHAKE, cancel, deadline)?;
        let granted = scope(&ready)?;
        if ready.len() != 6
            || string(&ready, "type")? != "ready"
            || ready.get("version").and_then(Value::as_u64) != Some(1)
            || string(&ready, "instance")? != instance_text
            || string(&ready, "pair_id")? != lower_hex(&pair_id)
            || number(&ready, "epoch")? != epoch.get()
            || (pair && granted != credential.scopes)
        {
            return Err(invalid());
        }
        if (credential.epoch, credential.scopes) != (epoch, granted) {
            credential.epoch = epoch;
            credential.scopes = granted;
            store.save(&credential)?;
        }
        Ok(Some(Self { pipe, credential }))
    }

    pub fn info(&self) -> Value {
        let timestamp = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default();
        json!({
            "status": "connected",
            "instance": lower_hex(&self.pipe.target().instance),
            "scopes": self.credential.scopes,
            "last_verified_unix": timestamp.as_secs_f64(),
        })
    }

    pub(crate) fn call(
        &mut self,
        method: &'static str,
        arguments: Object,
        cancel: &Cancel,
    ) -> Result<Value> {
        let mut random_id = [0; 16];
        getrandom::fill(&mut random_id).map_err(|_| NativeError::new("random_unavailable"))?;
        let id = lower_hex(&random_id);
        let request = object(json!({"id": id, "method": method, "arguments": arguments}));
        let deadline = Instant::now() + CALL_WAIT;
        self.pipe.send(&request, MAX_FRAME, cancel, deadline)?;
        let mut response = self.pipe.receive(MAX_FRAME, cancel, deadline)?;
        if response.len() != 3 || string(&response, "id")? != id {
            return Err(invalid());
        }
        match response.get("ok").and_then(Value::as_bool) {
            Some(true) => match response.remove("result") {
                Some(value @ Value::Object(_)) => {
                    Ok(json!({"id": id, "ok": true, "result": value}))
                }
                _ => Err(invalid()),
            },
            Some(false) => {
                let error = response
                    .get("error")
                    .and_then(Value::as_object)
                    .ok_or_else(invalid)?;
                let code = string(error, "code")?;
                let message = string(error, "message")?;
                if error.len() != 2
                    || !(1..=64).contains(&code.len())
                    || !code.as_bytes()[0].is_ascii_lowercase()
                    || !code
                        .bytes()
                        .all(|b| b.is_ascii_lowercase() || b.is_ascii_digit() || b == b'_')
                    || message.len() > 1024
                {
                    return Err(invalid());
                }
                Ok(json!({"ok": false, "error": code, "message": message}))
            }
            None => Err(invalid()),
        }
    }
}
