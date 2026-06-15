---
name: qkeymapper-workflow
description: QKeyMapper playbook for planning, implementation, debugging, and validation in this repo.
when_to_use: When working on QKeyMapper code - planning changes, implementing features, debugging issues, or validating fixes. Also when the user mentions qkeymapper, Qt, floating button, key mapping, Forza, or any core file like qkeymapper.cpp, qkeymapper_worker, qkeymapper_constants, qitemsetupdialog, qfloatingbuttonsetupdialog.
---

# QKeyMapper Workflow

Use this skill for repo-specific work in QKeyMapper. Keep scope narrow, reuse existing architecture, and validate the smallest slice that proves the change.

## Start here
1. Anchor on the user-visible behavior, failing command, or touched file.
2. Step to the nearest owning function, class, or dialog.
3. Check same-module patterns before broad search.
4. Keep the first change small and reversible.
5. Before editing, identify a unique anchor for the exact snippet; if the file has repeated similar blocks, do not patch until the target is uniquely disambiguated by nearby literal text or line context.

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
- Do not spend multiple iterations inventing new jom commands; use the known build-directory workflow first. If a jom failure is due to environment, path, or quoting uncertainty, stop after one retry and hand the validation back to the user.
- After any patch, run the cheapest discriminating check immediately. If the check shows the edit landed in the wrong place, repair that same slice before widening scope or editing other files.

## Style
- Keep scope narrow.
- Reuse existing architecture.
- New comments in English.
- No .ts edits unless asked.
- Keep Qt/C++ source UTF-8 without BOM.
