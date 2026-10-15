# QSettings Save Results and Shared Cache

Verified on Qt 6.8.3 and Qt 5.15.2 with isolated INIs and real application save/load paths.

- Complete setting-name validation and overwrite confirmation before any save-related writes. A cancelled overwrite must not persist global settings or close an editor containing an uncommitted draft.
- `QSettings::setValue()` and a same-process reload do not prove disk persistence. Explicitly `sync()` and check `status()` before success/reload/dirty clearing. Verify disk contents and an independent cold process.
- Same-path QSettings objects share pending values. After a failed write, a later reader/destructor can commit those values when the obstruction disappears. Snapshot old values, determine changed/removed keys before sync, and restore only this save's pending changes on failure. Test existing and newly named profiles, unlock, another reader, exit and cold restart.
- A format error may have been consumed by an earlier same-path reader. A later object's `NoError` alone does not prove the existing INI is valid. Validate a fresh temporary snapshot using Qt's own INI parser before modifying a malformed file; preserve its original bytes on refusal.
- Give `QTemporaryFile` an explicit safe template. QKeyMapper's application name includes the executable path; Qt's default template can consequently contain an invalid embedded Windows path. Always pair negative protection tests with a successful normal save to avoid a vacuous pass.
- Explicit global reload must apply the loaded enum through the existing live-scaling path while preserving immutable B and the prior dirty state. A programmatic ComboBox index change only synchronizes selection/menu; it does not activate scaling. Ordinary profile switching continues to retain the current T.

These checks do not establish a multi-process INI transaction or hardware-input release. Retain raw INIs, tool exits, source/EXE identity and actual desktop evidence separately from UI-only probes.
