"""The bounded JSON and authentication contract shared with the C++ endpoint."""

import hashlib
import hmac
import json
import math
import re
import struct
import unicodedata
from dataclasses import dataclass, field

PIPE_PREFIX = "\\\\.\\pipe\\QKeyMapper.Automation."
MAX_FRAME = 256 * 1024
MAX_HANDSHAKE = 4096
KNOWN_SCOPES = 127


class BridgeError(Exception):
    def __init__(self, code: str):
        self.code = code
        super().__init__(code)


def require(condition: bool, code: str = "invalid_message") -> None:
    if not condition:
        raise BridgeError(code)


def fields(value: dict, names: str) -> None:
    require(type(value) is dict and value.keys() == set(names.split()))


def hex_bytes(value: str, size: int) -> bytes:
    require(type(value) is str and re.fullmatch("[0-9a-f]{%d}" % (size * 2), value) is not None)
    return bytes.fromhex(value)


def decimal(value: str, bits: int, *, zero: bool = False) -> int:
    require(type(value) is str and len(value) <= 20 and re.fullmatch(r"0|[1-9][0-9]*", value) is not None)
    number = int(value)
    require((0 if zero else 1) <= number < 1 << bits)
    return number


def scopes(value: int) -> int:
    require(type(value) is int and value & 1 == 1 and value & ~KNOWN_SCOPES == 0)
    return value


def client_name(value: str) -> None:
    require(type(value) is str and bool(value.strip()), "invalid_client_name")
    try:
        require(len(value.encode("utf-8")) <= 80, "invalid_client_name")
    except UnicodeError:
        raise BridgeError("invalid_client_name") from None
    require(all(unicodedata.category(c) not in ("Cc", "Cf") for c in value), "invalid_client_name")


def _pairs(items: list) -> dict:
    result = {}
    for key, value in items:
        require(key not in result)
        result[key] = value
    return result


def _json_value(value, depth: int = 0) -> None:
    if type(value) in (dict, list):
        require(depth <= 32)
    if type(value) is dict:
        for key, item in value.items():
            require(type(key) is str)
            _json_value(key, depth)
            _json_value(item, depth + 1)
    elif type(value) is list:
        for item in value:
            _json_value(item, depth + 1)
    elif type(value) is str:
        require(all(not 0xD800 <= ord(c) <= 0xDFFF for c in value))
    elif type(value) is float:
        require(math.isfinite(value))


def decode(data: bytes, limit: int = MAX_FRAME) -> dict:
    require(0 < len(data) <= limit and not data.startswith(b"\xef\xbb\xbf"))
    try:
        result = json.loads(data.decode("utf-8"), object_pairs_hook=_pairs,
                            parse_constant=lambda _: require(False))
        require(type(result) is dict)
        _json_value(result)
        return result
    except (UnicodeError, ValueError, RecursionError):
        raise BridgeError("invalid_message") from None


def encode(message: dict, limit: int = MAX_FRAME) -> bytes:
    try:
        require(type(message) is dict)
        _json_value(message)
        data = json.dumps(message, ensure_ascii=False, allow_nan=False, separators=(",", ":")).encode("utf-8")
    except (UnicodeError, ValueError, RecursionError, TypeError):
        raise BridgeError("invalid_message") from None
    require(0 < len(data) <= limit)
    return struct.pack(">I", len(data)) + data


@dataclass(frozen=True)
class Credential:
    pair_id: str
    epoch: int
    scopes: int
    secret: bytes = field(repr=False)

    @classmethod
    def from_json(cls, value: dict):
        fields(value, "pair_id epoch scopes secret")
        hex_bytes(value["pair_id"], 16)
        return cls(value["pair_id"], decimal(value["epoch"], 64), scopes(value["scopes"]),
                   hex_bytes(value["secret"], 32))

    def to_json(self) -> dict:
        return {"pair_id": self.pair_id, "epoch": str(self.epoch), "scopes": self.scopes,
                "secret": self.secret.hex()}


def proof(role: int, instance: str, pair_id: str, epoch: int, client, server,
          client_nonce: bytes, server_nonce: bytes, key: bytes) -> bytes:
    require(role in (1, 2, 3) and 0 < epoch < 1 << 64)
    require(len(client_nonce) == len(server_nonce) == len(key) == 32)
    transcript = (b"QKeyMapper-MCP-AUTH\0" + bytes([role]) + struct.pack(">I", 1)
                  + hex_bytes(instance, 16) + hex_bytes(pair_id, 16)
                  + struct.pack(">QIQIQ", epoch, client.pid, client.created, server.pid, server.created)
                  + client_nonce + server_nonce)
    return hmac.digest(key, transcript, hashlib.sha256)
