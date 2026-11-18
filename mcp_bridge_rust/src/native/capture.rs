use base64::{Engine, engine::general_purpose::STANDARD};
use chrono::DateTime;
use serde_json::Value;
use sha2::{Digest, Sha256};

use super::{Discovery, NativeError, Result};
use crate::wire::{Object, decimal, hex_bytes, lower_hex};

const MAX_PNG: usize = 160 * 1024;
const MAX_PIXELS: u64 = 8 * 1024 * 1024;
const PNG_SIGNATURE: &[u8] = b"\x89PNG\r\n\x1a\n";

pub(super) fn verify(value: &mut Value, target: &Discovery, arguments: &Object) -> Result<()> {
    let verified =
        validate(value, target, arguments).ok_or_else(|| NativeError::new("invalid_image"))?;
    value["sha256"] = verified.into();
    Ok(())
}

fn fields(value: &Value, names: &[&str]) -> bool {
    value.as_object().is_some_and(|object| {
        object.len() == names.len() && names.iter().all(|key| object.contains_key(*key))
    })
}

fn validate(value: &Value, discovery: &Discovery, arguments: &Object) -> Option<String> {
    let role = value.get("role")?.as_str()?;
    let mut names = vec![
        "target",
        "role",
        "window_ref",
        "source_revision",
        "source_tab_ref",
        "source_context",
        "presentation",
        "format",
        "pixel_width",
        "pixel_height",
        "device_pixel_ratio",
        "bytes",
        "captured_at_utc",
        "png_base64",
    ];
    match role {
        "main" => {}
        "item_setup" => names.push("edit_target"),
        "mapping_advanced" | "ibinput_settings" => {
            names.push("settings_scope");
            let expected = if role == "ibinput_settings" {
                names.push("preferences_revision");
                hex_bytes::<32>(value.get("preferences_revision")?.as_str()?).ok()?;
                "live_ibinput_preferences_and_passive_status"
            } else {
                "live_loaded_settings_not_committed_by_source_revision"
            };
            if value["settings_scope"] != expected {
                return None;
            }
            if let Some(state) = value.get("settings_editor_unchanged") {
                state.as_bool()?;
                names.push("settings_editor_unchanged");
            }
        }
        _ => return None,
    }
    if !fields(value, &names)
        || !["role", "window_ref", "source_revision", "source_tab_ref"]
            .iter()
            .all(|key| arguments.get(*key) == value.get(*key))
        || value["format"] != "image/png"
        || value["presentation"] != "live_qt_client_ui_drafts_may_be_visible"
    {
        return None;
    }

    let instance = lower_hex(&discovery.instance);
    let target = &value["target"];
    if !fields(
        target,
        &["instance", "pid", "created", "executable", "desktop"],
    ) || target["instance"] != instance
        || decimal(target["pid"].as_str()?, false).ok()?
            != u64::from(discovery.server.stamp.pid.get())
        || decimal(target["created"].as_str()?, false).ok()? != discovery.server.stamp.created.get()
        || target["executable"].as_str()? != discovery.server.executable.to_str()?
        || !(1..=255).contains(&target["desktop"].as_str()?.chars().count())
    {
        return None;
    }

    hex_bytes::<32>(value["window_ref"].as_str()?).ok()?;
    let revision = value["source_revision"].as_str()?;
    hex_bytes::<32>(revision).ok()?;
    let tab = value["source_tab_ref"]
        .as_str()?
        .strip_prefix(revision)?
        .strip_prefix(":source:")?;
    if decimal(tab, true).ok()? > i32::MAX as u64 {
        return None;
    }
    let context = &value["source_context"];
    if !fields(
        context,
        &[
            "revision",
            "instance",
            "reference_scope",
            "scope",
            "read_basis",
            "cleanup_generation",
            "configuration_selection",
            "mapping_load_context",
            "common_feature_enabled",
        ],
    ) || context["revision"] != revision
        || context["instance"] != instance
        || context["reference_scope"] != "source_snapshot_only"
        || context["scope"] != "current_memory_sources"
    {
        return None;
    }
    match context["read_basis"].as_str()? {
        "pristine_idle" if context["cleanup_generation"].is_null() => {}
        "completed_cleanup" => {
            decimal(context["cleanup_generation"].as_str()?, false).ok()?;
        }
        _ => return None,
    }
    if role == "item_setup" {
        let edit = &value["edit_target"];
        let row = edit["source_row"].as_u64()?;
        if !fields(edit, &["source_tab_ref", "source_ref", "source_row"])
            || edit["source_tab_ref"] != value["source_tab_ref"]
            || row > i32::MAX as u64
            || edit["source_ref"] != format!("{revision}:row:{tab}:{row}")
        {
            return None;
        }
    }
    let captured = value["captured_at_utc"].as_str()?;
    if captured.len() > 40
        || !captured.ends_with('Z')
        || DateTime::parse_from_rfc3339(captured).is_err()
    {
        return None;
    }

    let width = value["pixel_width"].as_u64()?;
    let height = value["pixel_height"].as_u64()?;
    let size = usize::try_from(value["bytes"].as_u64()?).ok()?;
    let ratio = value["device_pixel_ratio"].as_f64()?;
    let encoded = value["png_base64"].as_str()?;
    if !(1..=MAX_PIXELS).contains(&width.checked_mul(height)?)
        || !(1..=MAX_PNG).contains(&size)
        || !ratio.is_finite()
        || ratio <= 0.0
        || encoded.is_empty()
        || encoded.len() > 4 * MAX_PNG.div_ceil(3)
    {
        return None;
    }
    let png = STANDARD.decode(encoded).ok()?;
    if png.len() != size || !valid_png(&png, width as u32, height as u32) {
        return None;
    }
    Some(lower_hex(&Sha256::digest(&png)))
}

fn crc32(bytes: &[u8]) -> u32 {
    let mut crc = !0u32;
    for &byte in bytes {
        crc ^= u32::from(byte);
        for _ in 0..8 {
            crc = (crc >> 1) ^ (0xedb8_8320 & 0u32.wrapping_sub(crc & 1));
        }
    }
    !crc
}

fn valid_png(png: &[u8], width: u32, height: u32) -> bool {
    let Some(mut bytes) = png.strip_prefix(PNG_SIGNATURE) else {
        return false;
    };
    let mut has_image_data = false;
    for index in 0..128 {
        let Some((length, rest)) = bytes.split_first_chunk::<4>() else {
            return false;
        };
        let length = u32::from_be_bytes(*length) as usize;
        if length > rest.len().saturating_sub(8) || rest.len() < 8 {
            return false;
        }
        let (chunk, rest) = rest.split_at(length + 4);
        let (checksum, tail) = rest.split_first_chunk::<4>().expect("checked chunk size");
        if crc32(chunk) != u32::from_be_bytes(*checksum) {
            return false;
        }
        let (kind, body) = chunk.split_first_chunk::<4>().expect("checked chunk type");
        match kind {
            b"IHDR"
                if index == 0
                    && length == 13
                    && body[..4] == width.to_be_bytes()
                    && body[4..8] == height.to_be_bytes()
                    && body[8] == 8
                    && matches!(body[9], 2 | 6)
                    && body[10..] == [0, 0, 0] => {}
            b"IHDR" => return false,
            _ if index == 0 => return false,
            b"IDAT" if length > 0 => has_image_data = true,
            b"IEND" => return length == 0 && has_image_data && tail.is_empty(),
            _ => {}
        }
        bytes = tail;
    }
    false
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    const SAMPLE: &str = "iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAAEklEQVR4nGP4z8DAAMIM/4EAAB/uBfsL2WiLAAAAAElFTkSuQmCC";

    #[test]
    fn checks_target_context_and_png_before_publishing() {
        let server = super::super::own_identity().unwrap();
        let target = Discovery {
            instance: [0xab; 16],
            server,
            pipe: String::new(),
        };
        let revision = "c".repeat(64);
        let arguments = json!({"role":"main", "window_ref":"b".repeat(64),
            "source_revision":revision, "source_tab_ref":format!("{revision}:source:0")});
        let arguments = arguments.as_object().unwrap();
        let mut value = Value::Object(arguments.clone());
        let instance = lower_hex(&target.instance);
        value["target"] = json!({"instance":instance, "pid":target.server.stamp.pid.to_string(),
            "created":target.server.stamp.created.to_string(), "executable":target.server.executable, "desktop":"Private"});
        value["source_context"] = json!({"revision":revision, "instance":instance,
            "reference_scope":"source_snapshot_only", "scope":"current_memory_sources", "read_basis":"pristine_idle",
            "cleanup_generation":null, "configuration_selection":{}, "mapping_load_context":{}, "common_feature_enabled":false});
        value["presentation"] = "live_qt_client_ui_drafts_may_be_visible".into();
        value["format"] = "image/png".into();
        value["pixel_width"] = 2.into();
        value["pixel_height"] = 2.into();
        value["bytes"] = 75.into();
        value["device_pixel_ratio"] = 1.25.into();
        value["captured_at_utc"] = "2026-11-18T03:00:00.000Z".into();
        value["png_base64"] = SAMPLE.into();

        assert_eq!(
            validate(&value, &target, arguments).as_deref(),
            Some("e6d66889131220f931fddfb05730d647a0992456c63ae0a8154b4ae32ff219ef")
        );
        for (key, replacement) in [
            ("bytes", json!(MAX_PNG + 1)),
            ("device_pixel_ratio", json!(true)),
            ("pixel_width", json!(3)),
            ("png_base64", json!("%%%")),
            ("captured_at_utc", json!("2026-11-31T03:00:00Z")),
            ("window_ref", json!("a".repeat(64))),
        ] {
            let mut bad = value.clone();
            bad[key] = replacement;
            assert!(validate(&bad, &target, arguments).is_none(), "{key}");
        }
        let mut bad = value.clone();
        bad["target"]["created"] = "1".into();
        assert!(validate(&bad, &target, arguments).is_none());
        let mut bad = value.clone();
        bad["source_context"]["cleanup_generation"] = "1".into();
        assert!(validate(&bad, &target, arguments).is_none());

        let mut png = STANDARD.decode(SAMPLE).unwrap();
        *png.last_mut().unwrap() ^= 1;
        let mut bad = value.clone();
        bad["png_base64"] = STANDARD.encode(png).into();
        assert!(validate(&bad, &target, arguments).is_none());

        let mut args = arguments.clone();
        for role in ["item_setup", "mapping_advanced", "ibinput_settings"] {
            args.insert("role".into(), role.into());
            let mut changed = value.clone();
            changed["role"] = role.into();
            assert!(validate(&changed, &target, &args).is_none());
            match role {
                "item_setup" => {
                    changed["edit_target"] = json!({"source_row":0,
                    "source_ref":format!("{revision}:row:0:0"), "source_tab_ref":format!("{revision}:source:0")})
                }
                "mapping_advanced" => {
                    changed["settings_scope"] =
                        "live_loaded_settings_not_committed_by_source_revision".into()
                }
                "ibinput_settings" => {
                    changed["settings_scope"] =
                        "live_ibinput_preferences_and_passive_status".into();
                    changed["preferences_revision"] = "d".repeat(64).into();
                    changed["settings_editor_unchanged"] = true.into();
                }
                _ => unreachable!(),
            }
            assert!(validate(&changed, &target, &args).is_some(), "{role}");
        }
    }
}
