use std::fmt;

use serde::de::{self, DeserializeSeed, MapAccess, SeqAccess, Visitor};
use serde_json::{Map, Number, Value};

pub const MAX_FRAME: usize = 256 * 1024;
pub const MAX_HANDSHAKE: usize = 4096;
pub const KNOWN_SCOPES: u16 = 511;
const MAX_DEPTH: usize = 32;

pub type Object = Map<String, Value>;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct InvalidMessage;

impl fmt::Display for InvalidMessage {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("invalid_message")
    }
}

impl std::error::Error for InvalidMessage {}

pub fn frame_length(prefix: [u8; 4], limit: usize) -> Result<usize, InvalidMessage> {
    let size = u32::from_be_bytes(prefix) as usize;
    (size > 0 && size <= limit)
        .then_some(size)
        .ok_or(InvalidMessage)
}

pub fn decode(bytes: &[u8], limit: usize) -> Result<Object, InvalidMessage> {
    if bytes.is_empty() || bytes.len() > limit || bytes.starts_with(b"\xef\xbb\xbf") {
        return Err(InvalidMessage);
    }
    let mut deserializer = serde_json::Deserializer::from_slice(bytes);
    let value = WireValue(0)
        .deserialize(&mut deserializer)
        .map_err(|_| InvalidMessage)?;
    deserializer.end().map_err(|_| InvalidMessage)?;
    match value {
        Value::Object(object) => Ok(object),
        _ => Err(InvalidMessage),
    }
}

pub fn encode(message: &Object, limit: usize) -> Result<Vec<u8>, InvalidMessage> {
    if !message.values().all(|value| within_depth(value, 1)) {
        return Err(InvalidMessage);
    }
    let data = serde_json::to_vec(message).map_err(|_| InvalidMessage)?;
    let size = u32::try_from(data.len()).map_err(|_| InvalidMessage)?;
    frame_length(size.to_be_bytes(), limit)?;
    let mut frame = Vec::with_capacity(4 + data.len());
    frame.extend_from_slice(&size.to_be_bytes());
    frame.extend_from_slice(&data);
    Ok(frame)
}

fn within_depth(value: &Value, depth: usize) -> bool {
    match value {
        Value::Object(map) => {
            depth <= MAX_DEPTH && map.values().all(|value| within_depth(value, depth + 1))
        }
        Value::Array(values) => {
            depth <= MAX_DEPTH && values.iter().all(|value| within_depth(value, depth + 1))
        }
        _ => true,
    }
}

struct WireValue(usize);

impl<'de> DeserializeSeed<'de> for WireValue {
    type Value = Value;

    fn deserialize<D: de::Deserializer<'de>>(self, de: D) -> Result<Value, D::Error> {
        de.deserialize_any(self)
    }
}

impl<'de> Visitor<'de> for WireValue {
    type Value = Value;

    fn expecting(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("bounded JSON without duplicate keys")
    }

    fn visit_map<A: MapAccess<'de>>(self, mut map: A) -> Result<Value, A::Error> {
        if self.0 > MAX_DEPTH {
            return Err(de::Error::custom("JSON object depth exceeded"));
        }
        let mut object = Object::new();
        while let Some(key) = map.next_key::<String>()? {
            if object.contains_key(&key) {
                return Err(de::Error::custom("duplicate JSON key"));
            }
            object.insert(key, map.next_value_seed(Self(self.0 + 1))?);
        }
        Ok(Value::Object(object))
    }

    fn visit_seq<A: SeqAccess<'de>>(self, mut seq: A) -> Result<Value, A::Error> {
        if self.0 > MAX_DEPTH {
            return Err(de::Error::custom("JSON array depth exceeded"));
        }
        let mut values = Vec::new();
        while let Some(value) = seq.next_element_seed(Self(self.0 + 1))? {
            values.push(value);
        }
        Ok(Value::Array(values))
    }

    fn visit_bool<E: de::Error>(self, value: bool) -> Result<Value, E> {
        Ok(Value::Bool(value))
    }

    fn visit_i64<E: de::Error>(self, value: i64) -> Result<Value, E> {
        Ok(Value::Number(value.into()))
    }

    fn visit_u64<E: de::Error>(self, value: u64) -> Result<Value, E> {
        Ok(Value::Number(value.into()))
    }

    fn visit_f64<E: de::Error>(self, value: f64) -> Result<Value, E> {
        Number::from_f64(value)
            .map(Value::Number)
            .ok_or_else(|| de::Error::custom("non-finite JSON number"))
    }

    fn visit_str<E: de::Error>(self, value: &str) -> Result<Value, E> {
        Ok(Value::String(value.to_owned()))
    }

    fn visit_unit<E: de::Error>(self) -> Result<Value, E> {
        Ok(Value::Null)
    }
}

pub fn decimal(value: &str, allow_zero: bool) -> Result<u64, InvalidMessage> {
    if value == "0" && allow_zero {
        return Ok(0);
    }
    if value.len() > 20
        || !value.starts_with(|ch| matches!(ch, '1'..='9'))
        || !value.bytes().all(|byte| byte.is_ascii_digit())
    {
        return Err(InvalidMessage);
    }
    value.parse().map_err(|_| InvalidMessage)
}

pub fn scope_bits(value: &Value) -> Result<u16, InvalidMessage> {
    let bits = value
        .as_u64()
        .and_then(|bits| u16::try_from(bits).ok())
        .ok_or(InvalidMessage)?;
    (bits & 1 == 1 && bits & !KNOWN_SCOPES == 0)
        .then_some(bits)
        .ok_or(InvalidMessage)
}

pub fn hex_bytes<const N: usize>(text: &str) -> Result<[u8; N], InvalidMessage> {
    fn nibble(byte: u8) -> Result<u8, InvalidMessage> {
        match byte {
            b'0'..=b'9' => Ok(byte - b'0'),
            b'a'..=b'f' => Ok(byte - b'a' + 10),
            _ => Err(InvalidMessage),
        }
    }
    if text.len() != N * 2 {
        return Err(InvalidMessage);
    }
    let mut result = [0; N];
    let (pairs, _) = text.as_bytes().as_chunks::<2>();
    for (byte, pair) in result.iter_mut().zip(pairs) {
        *byte = (nibble(pair[0])? << 4) | nibble(pair[1])?;
    }
    Ok(result)
}

pub fn lower_hex(bytes: &[u8]) -> String {
    let alphabet = b"0123456789abcdef";
    let mut text = String::with_capacity(bytes.len() * 2);
    for &byte in bytes {
        text.push(alphabet[(byte >> 4) as usize] as char);
        text.push(alphabet[(byte & 15) as usize] as char);
    }
    text
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn framed_utf8_round_trip_and_bounded_lengths() {
        let object = json!({"中文": [true, null, "日本語", {"key": 1.5, "nul": "x\0y"}]});
        let object = object.as_object().unwrap();
        let frame = encode(object, MAX_FRAME).unwrap();
        assert_eq!(
            frame_length(frame[..4].try_into().unwrap(), MAX_FRAME),
            Ok(frame.len() - 4)
        );
        assert_eq!(&decode(&frame[4..], MAX_FRAME).unwrap(), object);
        for invalid in [0, MAX_HANDSHAKE as u32 + 1, u32::MAX] {
            assert_eq!(
                frame_length(invalid.to_be_bytes(), MAX_HANDSHAKE),
                Err(InvalidMessage)
            );
        }
        let large = json!({"key": "A".repeat(MAX_HANDSHAKE)});
        assert_eq!(
            encode(large.as_object().unwrap(), MAX_HANDSHAKE),
            Err(InvalidMessage)
        );
    }

    #[test]
    fn rejects_ambiguous_or_malformed_json() {
        let invalid: &[&[u8]] = &[
            b"",
            b"[]",
            b"{}{}",
            br#"{"x":1,"x":2}"#,
            br#"{"nested":[{"x":1,"\u0078":2}]}"#,
            br#"{"x":NaN}"#,
            br#"{"x":1e999}"#,
            b"\xef\xbb\xbf{}",
            br#"{"x":"\ud800"}"#,
            b"{\"x\":\"\xff\"}",
        ];
        for bytes in invalid {
            assert_eq!(decode(bytes, MAX_FRAME), Err(InvalidMessage));
        }
        for depth in [32, 33] {
            let data = format!(r#"{{"x":{}0{}}}"#, "[".repeat(depth), "]".repeat(depth));
            assert_eq!(decode(data.as_bytes(), MAX_FRAME).is_ok(), depth == 32);
            let value = serde_json::from_str::<Value>(&data).unwrap();
            assert_eq!(
                encode(value.as_object().unwrap(), MAX_FRAME).is_ok(),
                depth == 32
            );
        }
    }

    #[test]
    fn numeric_and_hex_identity_fields_are_canonical() {
        assert_eq!(decimal("18446744073709551615", false), Ok(u64::MAX));
        assert_eq!(decimal("0", true), Ok(0));
        for value in [
            "",
            "0",
            "01",
            "-1",
            "+1",
            "1 ",
            "1e1",
            "18446744073709551616",
            "９",
        ] {
            assert_eq!(decimal(value, false), Err(InvalidMessage));
        }
        for bad in [
            json!(true),
            json!(1.0),
            json!(2),
            json!(512),
            json!(513),
            json!("1"),
        ] {
            assert_eq!(scope_bits(&bad), Err(InvalidMessage));
        }
        for valid in [1, 129, 257, 511] {
            assert_eq!(scope_bits(&json!(valid)), Ok(valid));
        }
        assert_eq!(hex_bytes("af01"), Ok([175, 1]));
        assert_eq!(hex_bytes::<2>("AF01"), Err(InvalidMessage));
        assert_eq!(hex_bytes::<2>("éaf"), Err(InvalidMessage));
        assert_eq!(lower_hex(&[175, 1]), "af01");
    }
}
