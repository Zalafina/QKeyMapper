# Agent Workspace Sharing Migration

**Date:** 2026-07-27

## Objective

Make repository-level agent guidance and durable QKeyMapper knowledge reusable
by both Codex and Claude Code, while keeping tool-specific configuration
separate and version-controllable.

## Intended layout and changes

- Root `AGENTS.md` is the shared instruction source of truth.
- Root `CLAUDE.md` imports `AGENTS.md` so Claude Code receives the same
  repository instructions.
- Existing `.claude/memory/` content is preserved under
  `.agents/context/lessons/`; `project-memory.md` is the shared entry point and
  `lessons/MEMORY.md` remains the detailed index.
- `qkeymapper-workflow` exists as two ordinary, identical files:
  `.agents/skills/qkeymapper-workflow/SKILL.md` for Codex and
  `.claude/skills/qkeymapper-workflow/SKILL.md` for Claude Code.
- `.claude/settings.json` and `.codex/config.toml` remain tool-specific.

## Important decisions

- Do not use a Windows junction or a setup script for the Claude skill
  directory. Manually synchronize the two workflow skill copies when the
  playbook changes.
- QKeyMapper-specific self-improvement artifacts must be stored only in
  `.agents/context/lessons/` and added to `lessons/MEMORY.md`.
- Do not write QKeyMapper experience to the global self-improving-agent
  installation's `memory/` directory.
- Keep only durable, reusable decisions and verified pitfalls in repository
  memory. Exclude chat transcripts, temporary task state, and sensitive data.

## Remote integration note

The user saved the earlier local patch as
`0001-Update-agent-.md-files-for-codex.patch`. It conflicted with incoming
remote changes and was not rebased directly. This migration was reapplied
after synchronizing the remote branch; create a new commit from the current
workspace state.

## Scope and verification

- No QKeyMapper product source code or build configuration was changed.
- The original 42 memory files were preserved during the migration.
- The two workflow skill copies were verified to have identical content after
  the workspace memory routing rule was added.
