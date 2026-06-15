---
name: self-improving-agent
description: Logs corrections, errors, and feature requests during AI-assisted coding sessions to enable continuous improvement across sessions. Use when the user corrects you, a command fails, the user requests a new capability, or you discover a better approach.
when_to_use: When the user corrects a mistake, a command or tool call fails, the user asks for a missing feature, or you discover outdated knowledge or a better approach. Also when wrapping up a task — review pending learnings.
---

# Self-Improving Agent

Log corrections, errors, and feature requests to `.claude/.learnings/` so you improve across sessions.

## First-use initialization

The first time this skill activates in a project, create the `.claude/.learnings/` directory:

```bash
mkdir -p .claude/.learnings
```

If `.claude/.learnings/LEARNINGS.md` does not exist:
```bash
printf "# Learnings\n\nCorrections, insights, and knowledge gaps discovered during AI-assisted work.\n" > .claude/.learnings/LEARNINGS.md
```

If `.claude/.learnings/ERRORS.md` does not exist:
```bash
printf "# Errors\n\nCommand failures and integration errors encountered during AI-assisted work.\n" > .claude/.learnings/ERRORS.md
```

If `.claude/.learnings/FEATURE_REQUESTS.md` does not exist:
```bash
printf "# Feature Requests\n\nCapabilities requested by the user that are not yet implemented.\n" > .claude/.learnings/FEATURE_REQUESTS.md
```

Never overwrite existing files. This is a no-op if `.claude/.learnings/` is already initialized.

## Quick reference

| Situation | Action |
|---|---|
| Command or tool call fails | Log to `ERRORS.md` |
| User corrects you | Log to `LEARNINGS.md` (category: `correction`) |
| User wants a missing feature | Log to `FEATURE_REQUESTS.md` |
| External tool / API fails | Log to `ERRORS.md` with full context |
| Knowledge was outdated or wrong | Log to `LEARNINGS.md` (category: `knowledge_gap`) |
| Found a better approach | Log to `LEARNINGS.md` (category: `best_practice`) |

## Logging formats

### Learning entry

```
## [LRN-YYYYMMDD-XXX] category
**Logged**: ISO-8601 timestamp
**Priority**: low | medium | high | critical
**Status**: pending
**Area**: frontend | backend | infra | tests | docs | config
### Summary
One-line summary of the learning.
### Details
What happened — include the original mistake, the correction, and why it matters.
### Suggested Action
What should change going forward to avoid repeating this.
### Metadata
- Source: conversation | code_review | error | user_feedback
```

### Error entry

```
## [ERR-YYYYMMDD-XXX] command_or_tool_name
**Logged**: ISO-8601 timestamp
**Priority**: high
**Status**: pending
### Summary
One-line description of the error.
### Error
The exact error message or exit code.
### Context
What you were trying to do, relevant environment details.
### Suggested Fix
What resolved it (or what to try next time).
### Metadata
- Exit code: N
- Tool: tool_name
```

### Feature request entry

```
## [FEAT-YYYYMMDD-XXX] capability_name
**Logged**: ISO-8601 timestamp
**Priority**: medium
**Status**: pending
### Requested Capability
What the user asked for.
### User Context
Why they need it, what problem it solves.
### Complexity Estimate
low | medium | high | unknown
### Suggested Implementation
High-level approach if obvious.
### Metadata
- Source: user_request
```

## ID generation

Format: `TYPE-YYYYMMDD-XXX` where TYPE is `LRN`, `ERR`, or `FEAT`. Generate XXX as sequential within the day (001, 002, …) by counting existing entries with the same date prefix in the target file.

## Resolving entries

When a learning is applied, an error has a confirmed fix, or a feature is implemented, update `**Status**: pending` to `**Status**: resolved` and add `**Resolved**: ISO-8601 timestamp`.

## Priority guidelines

| Priority | When to use |
|---|---|
| **critical** | Causes data loss, security issue, or blocks all work |
| **high** | Breaks a workflow, repeated corrections on same topic |
| **medium** | Useful improvement, moderate impact |
| **low** | Minor style preference, nice-to-have |

## Area tags

| Area | Applies to |
|---|---|
| `frontend` | UI, dialogs, widgets, layouts |
| `backend` | Core logic, workers, state machines |
| `infra` | Build system, CI, environment, dependencies |
| `tests` | Testing, validation, verification |
| `docs` | Documentation, comments, README |
| `config` | Settings, project files, .pro, .ini |

## Promotion to CLAUDE.md

When a learning is confirmed across multiple sessions, promote it to a permanent instruction file. For Claude Code projects, promote to `.claude/CLAUDE.md` or the appropriate `.claude/rules/*.md`.

| Learning type | Promote to | Example |
|---|---|---|
| Behavioral patterns | `.claude/CLAUDE.md` | "Prefer targeted builds over full rebuilds" |
| Module-specific rules | `.claude/rules/<area>.md` | "Never edit .ts files unless asked" |
| Tool gotchas | `.claude/CLAUDE.md` | "Check for stray ui_*.h before reporting compile errors" |

Promotion criteria:
- `Recurrence-Count >= 3` across at least 2 distinct sessions
- All occurrences within a 30-day window
- Solution is tested and working

## Detection triggers

Automatically detect these conversation patterns and log without being asked:

| User says | Action |
|---|---|
| "No, that's not right" / "Wrong" / "Incorrect" | Log correction to LEARNINGS.md |
| "Can you also…" / "I wish you could…" / "Add a feature to…" | Log feature request |
| User provides information you didn't know | Log knowledge gap |
| Any tool returns non-zero exit code | Log error to ERRORS.md |

## Periodic review

When wrapping up a task, run:
```bash
grep -c "Status\*\*: pending" .claude/.learnings/*.md 2>/dev/null || echo "0 pending items"
```

If there are pending items, briefly summarize the top 3 by priority and ask the user if any should be promoted to CLAUDE.md or resolved.

## Security

Never log secrets, tokens, API keys, passwords, private keys, or full environment variable values. If a credential is involved in an error, redact it and note that a credential was present.

## Best practices

1. **Log immediately** — write the entry while context is fresh, don't queue it
2. **Be specific** — include exact error messages, file paths, and line references
3. **Include reproduction steps** — what command or action triggered the issue
4. **Link related files** — reference paths like `QKeyMapper/qkeymapper.cpp`
5. **Suggest concrete fixes** — not just "fix it" but how to fix it
6. **Use consistent categories** — same Area and Priority tags across entries
7. **Promote aggressively** — when a pattern is clear, move it to CLAUDE.md
8. **Review regularly** — scan pending items at natural breakpoints

## Gitignore

Add to project `.gitignore` to keep learnings local (recommended):
```
.claude/.learnings/
```

Or commit `.claude/.learnings/` to share with the team. At minimum, never commit entries that contain internal URLs, server names, or local paths.
