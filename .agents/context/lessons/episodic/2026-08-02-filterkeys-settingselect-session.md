# FilterKeys SettingSelect Session Management

## Context

FilterKeys options became SettingSelect-scoped, so the previous Worker-owned single restore flag could no longer handle profile switches or the interaction between `FKF_FILTERKEYSON` and `FKF_CLICKON`.

## Decisions

- Own the FilterKeys session in `QKeyMapper`, where mapping status and the current SettingSelect are available on the UI thread.
- Capture and restore only `FKF_FILTERKEYSON` and `FKF_CLICKON`; do not snapshot the complete `FILTERKEYS` structure.
- Before every write, read the current structure and preserve all unrelated `dwFlags`; keep the established behavior of zeroing the four timing fields only when a write is actually needed.
- Treat `WM_SETTINGCHANGE` as a FilterKeys notification whenever `wParam == SPI_SETFILTERKEYS`, regardless of `lParam`.
- Use the post-write full structure only as a notification fingerprint. It is never restoration data.
- During mapping, a different fingerprint is a user override: update the two-bit baseline and pause management for that SettingSelect. A different SettingSelect re-baselines from the current system state and resumes management.

## Validation status

Source call paths and diffs were checked. Qt 5.12.10/Qt 6.8.3 compilation and Windows behavior validation remain user-run.
