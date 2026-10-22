"""Current-user, local-machine credentials; no secret appears in MCP results."""

import hashlib
import os
import winreg
from pathlib import Path

import pywintypes
import win32cred
import win32crypt

from .protocol import BridgeError, Credential, decode, encode, hex_bytes, require


class CredentialStore:
    def __init__(self, discovery: Path, executable: Path, session: int):
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Microsoft\Cryptography", 0,
                            winreg.KEY_READ | winreg.KEY_WOW64_64KEY) as key:
            machine, kind = winreg.QueryValueEx(key, "MachineGuid")
        require(kind == winreg.REG_SZ and bool(machine), "credential_context_unavailable")
        context = ("QKeyMapper-MCP-Bridge/v1\0" + machine.lower() + "\0"
                   + os.path.normcase(str(discovery)) + "\0"
                   + os.path.normcase(str(executable)) + "\0" + str(session)).encode("utf-8")
        self._entropy = hashlib.sha256(context).digest()
        self.target = "QKeyMapper.MCP.Bridge/" + self._entropy.hex()

    def load(self) -> Credential | None:
        try:
            value = win32cred.CredRead(self.target, win32cred.CRED_TYPE_GENERIC)
        except pywintypes.error as error:
            if error.winerror == 1168:
                return None
            raise BridgeError("credential_read_failed") from None
        try:
            require(value["Persist"] == win32cred.CRED_PERSIST_LOCAL_MACHINE, "credential_rejected")
            blob = value["CredentialBlob"]
            require(type(blob) is bytes and 0 < len(blob) <= 2560, "credential_rejected")
            text = blob.decode("utf-16-le")
            require(bool(text) and len(text) % 2 == 0, "credential_rejected")
            cipher = hex_bytes(text, len(text) // 2)
            _, plain = win32crypt.CryptUnprotectData(cipher, self._entropy, None, None, 1)
            return Credential.from_json(decode(plain, 1024))
        except (pywintypes.error, BridgeError, KeyError, TypeError, UnicodeError):
            raise BridgeError("credential_rejected") from None

    def save(self, credential: Credential) -> None:
        try:
            plain = encode(credential.to_json(), 1024)[4:]
            blob = win32crypt.CryptProtectData(plain, "QKeyMapper MCP bridge", self._entropy, None, None, 1)
            # pywin32's CredWrite accepts Unicode; encode only the DPAPI ciphertext as text.
            text = blob.hex()
            require(0 < len(text.encode("utf-16-le")) <= 2560, "credential_write_failed")
            win32cred.CredWrite({"Type": win32cred.CRED_TYPE_GENERIC, "TargetName": self.target,
                "CredentialBlob": text, "Persist": win32cred.CRED_PERSIST_LOCAL_MACHINE,
                "UserName": "QKeyMapper MCP bridge"}, 0)
        except pywintypes.error:
            raise BridgeError("credential_write_failed") from None

    def forget(self) -> None:
        try:
            win32cred.CredDelete(self.target, win32cred.CRED_TYPE_GENERIC, 0)
        except pywintypes.error as error:
            if error.winerror != 1168:
                raise BridgeError("credential_delete_failed") from None
