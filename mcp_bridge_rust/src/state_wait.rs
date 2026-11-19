use std::time::{Duration, Instant};

use serde::{Deserialize, Serialize};
use serde_json::{Value, json};

use crate::native::{NativeError, Result};

const POLL_INTERVAL: Duration = Duration::from_millis(100);

#[derive(Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
enum Condition {
    MonitoringEnabled,
    MappingActive,
    StoppedAndQuiescent,
}

fn default_timeout() -> u16 {
    1000
}

#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StateWait {
    condition: Condition,
    #[serde(default = "default_timeout")]
    timeout_ms: u16,
}

impl StateWait {
    pub fn valid(&self) -> bool {
        (100..=5000).contains(&self.timeout_ms)
    }

    pub(crate) fn observe(
        &self,
        instance: &str,
        mut sample: impl FnMut(Instant) -> Result<Value>,
        mut pause: impl FnMut(Duration) -> Result<()>,
    ) -> Result<Value> {
        let deadline = Instant::now() + Duration::from_millis(u64::from(self.timeout_ms));
        loop {
            let mut reply = sample(deadline)?;
            if reply["ok"].as_bool() != Some(true) {
                return Ok(reply);
            }
            let state = reply["result"].take();
            let matches = self.matches(instance, &state)?;
            let remaining = deadline.saturating_duration_since(Instant::now());
            let reached = matches && !remaining.is_zero();
            if !reached && !remaining.is_zero() {
                pause(POLL_INTERVAL.min(remaining))?;
                if Instant::now() < deadline {
                    continue;
                }
            }
            return Ok(json!({"ok": true, "result": {
                "condition": self.condition,
                "outcome": if reached { "satisfied" } else { "timed_out" },
                "state": state
            }}));
        }
    }

    fn matches(&self, instance: &str, state: &Value) -> Result<bool> {
        let invalid = || NativeError::new("invalid_message");
        if state["schema_version"].as_u64() != Some(1)
            || state["instance"].as_str() != Some(instance)
        {
            return Err(invalid());
        }
        let run = &state["run"];
        let monitoring = run["monitoring_enabled"].as_bool().ok_or_else(invalid)?;
        let gui = run["gui_state"].as_str().ok_or_else(invalid)?;
        let worker = run["worker_state_sample"].as_str().ok_or_else(invalid)?;
        let completion = run["stop_completion"].as_str().ok_or_else(invalid)?;
        Ok(match self.condition {
            Condition::MonitoringEnabled => monitoring,
            Condition::MappingActive => {
                monitoring
                    && matches!(gui, "mapping_global" | "mapping_matched")
                    && worker == "started"
            }
            Condition::StoppedAndQuiescent => {
                !monitoring && gui == "idle" && worker == "stopped" && completion == "observed"
            }
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::native::Cancel;

    fn state(monitoring: bool, gui: &str, worker: &str, completion: &str) -> Value {
        json!({"schema_version": 1, "instance": "test", "run": {
            "monitoring_enabled": monitoring, "gui_state": gui,
            "worker_state_sample": worker, "stop_completion": completion,
        }})
    }

    #[test]
    fn monitoring_mapping_and_cleanup_have_distinct_predicates() {
        let mut wait = StateWait {
            condition: Condition::MonitoringEnabled,
            timeout_ms: 1000,
        };
        let checking = state(true, "checking", "started", "unobserved");
        assert!(wait.matches("test", &checking).unwrap());
        wait.condition = Condition::MappingActive;
        assert!(!wait.matches("test", &checking).unwrap());
        assert!(
            !wait
                .matches(
                    "test",
                    &state(true, "mapping_global", "starting", "unobserved")
                )
                .unwrap()
        );
        assert!(
            wait.matches(
                "test",
                &state(true, "mapping_global", "started", "unobserved")
            )
            .unwrap()
        );
        assert!(
            wait.matches(
                "test",
                &state(true, "mapping_matched", "started", "unobserved")
            )
            .unwrap()
        );
        wait.condition = Condition::StoppedAndQuiescent;
        assert!(
            !wait
                .matches("test", &state(false, "stopping", "stopped", "observed"))
                .unwrap()
        );
        assert!(
            !wait
                .matches("test", &state(false, "idle", "stopped", "unobserved"))
                .unwrap()
        );
        assert!(
            wait.matches("test", &state(false, "idle", "stopped", "observed"))
                .unwrap()
        );
        assert_eq!(
            wait.matches("changes", &checking).unwrap_err().code,
            "invalid_message"
        );
    }

    #[test]
    fn polls_use_one_deadline_and_timeout_keeps_observation() {
        let mut wait = StateWait {
            condition: Condition::MappingActive,
            timeout_ms: 1000,
        };
        let settled = state(true, "mapping_matched", "started", "unobserved");
        let mut samples = [
            state(true, "checking", "started", "unobserved"),
            settled.clone(),
        ]
        .into_iter();
        let mut deadlines = Vec::new();
        let result = wait
            .observe(
                "test",
                |deadline| {
                    deadlines.push(deadline);
                    Ok(json!({"ok": true, "result": samples.next().unwrap()}))
                },
                |_| Ok(()),
            )
            .unwrap();
        assert_eq!(deadlines, [deadlines[0]; 2]);
        assert_eq!(
            result["result"],
            json!({"state": settled, "condition": "mapping_active", "outcome": "satisfied"})
        );

        wait.condition = Condition::StoppedAndQuiescent;
        wait.timeout_ms = 100;
        let idle = state(false, "idle", "stopped", "unobserved");
        let result = wait
            .observe(
                "test",
                |_| Ok(json!({"ok": true, "result": idle})),
                |delay| {
                    std::thread::sleep(delay);
                    Ok(())
                },
            )
            .unwrap();
        assert_eq!(
            result["result"],
            json!({"state": idle, "condition": "stopped_and_quiescent", "outcome": "timed_out"})
        );
    }

    #[test]
    fn cancellation_and_product_errors_are_not_condition_timeouts() {
        let wait = StateWait {
            condition: Condition::MonitoringEnabled,
            timeout_ms: 1000,
        };
        let cancel = Cancel::new().unwrap();
        cancel.signal().unwrap();
        let result = wait.observe(
            "test",
            |_| Ok(json!({"ok": true, "result": state(false, "idle", "stopped", "unobserved")})),
            |_| cancel.wait(75),
        );
        assert_eq!(result.unwrap_err().code, "cancelled");

        let denied = json!({"ok": false, "error": "permission_denied"});
        let response = wait
            .observe("test", |_| Ok(denied.clone()), |_| unreachable!())
            .unwrap();
        assert_eq!(response, denied);
    }
}
