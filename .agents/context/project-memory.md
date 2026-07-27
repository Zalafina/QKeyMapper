# QKeyMapper Shared Project Memory

This file is the shared entry point for durable project knowledge used by both
Claude Code and Codex.

## How to use this memory

1. Before work that touches an existing feature, architecture, persistence,
   input handling, UI behavior, or a known Qt/WinAPI edge case, read the
   relevant entries in [lessons/MEMORY.md](lessons/MEMORY.md).
2. Treat the entries as project experience, not as a substitute for checking
   the current source tree. Confirm that an entry still applies before relying
   on it.
3. When a decision, constraint, or pitfall is likely to matter in a future
   task, add or update a concise entry in `lessons/` and add it to the index.
4. Do not record chat transcripts, temporary task status, credentials, tokens,
   personal data, or other sensitive information here.

## Current durable knowledge

The detailed, version-controlled knowledge base is indexed at
[lessons/MEMORY.md](lessons/MEMORY.md). It covers Qt UI and persistence
patterns, input-driver and FakeInput behavior, concurrency pitfalls, mapping
data behavior, and past implementation lessons.

## Agent asset policy

- QKeyMapper self-improvement artifacts are workspace knowledge: store them in
  `lessons/`, update `lessons/MEMORY.md`, and do not write them to a global
  self-improving-agent `memory/` directory.
- `qkeymapper-workflow` is intentionally maintained as two ordinary files:
  `.agents/skills/qkeymapper-workflow/SKILL.md` for Codex and
  `.claude/skills/qkeymapper-workflow/SKILL.md` for Claude Code. Do not use a
  junction or a setup script for this skill; manually synchronize the two
  files only when the playbook changes.
