---
name: qkeymapper-workflow
description: QKeyMapper playbook for MyPlan and other agents. Use for planning, implementation, debugging, and validation in this repo.
---

# QKeyMapper Workflow

Use this skill for repo-specific work in QKeyMapper. Keep scope narrow, reuse existing architecture, and validate the smallest slice that proves the change.

## Start here
1. Anchor on the user-visible behavior, failing command, or touched file.
2. Step to the nearest owning function, class, or dialog.
3. Check same-module patterns before broad search.
4. Keep the first change small and reversible.

## Core files
- qkeymapper.cpp / qkeymapper.h
- qkeymapper_worker.cpp / qkeymapper_worker.h
- qkeymapper_constants.h
- qitemsetupdialog.cpp / qitemsetupdialog.h
- qfloatingbuttonsetupdialog.cpp / qfloatingbuttonsetupdialog.h
- qkeymapper.ui
- qkeymapper_qt_compat.h
- QKeyMapper.pro

## Playbooks
- Dialog/UI: update load, apply, save, and restore together; if generated UI members are involved, check for a source-tree ui_*.h shadow file first.
- Floating Button: treat constants, worker state, dialog load/apply, runtime visuals, and save/restore as one pipeline; watch resize loops; validate the smallest slice.
- Forza/runtime mapping: keep legacy and new syntax on the same runtime path; verify press/release ordering and per-player state transitions; add DEBUG_LOGOUT_ON only at the exact transition point.
- Serialization/settings: keep keymapdata.ini and Save Setting paths in sync; check load/apply/save/recover paths together.

## Validation
- Use QKeyMapper/build/Desktop_Qt_6_8_3_MSVC2022_64bit-Release/ for Qt/MSVC validation.
- Prefer targeted object builds over full rebuilds.
- If compile errors mention Ui::QKeyMapper members, first check for stray source-tree ui_qkeymapper.h.
- Use build-directory generated ui_*.h.
- If qkeymapper.cpp compiles but behavior is still suspect, do one follow-up link build or the smallest runtime check.

## Style
- Keep scope narrow.
- Reuse existing architecture.
- New comments in English.
- No .ts edits unless asked.
- Keep Qt/C++ source UTF-8 without BOM.
