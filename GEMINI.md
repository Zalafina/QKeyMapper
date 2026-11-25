# Antigravity Workspace Instructions (QKeyMapper)

- This file contains Antigravity-only workspace guardrails layered alongside shared [AGENTS.md](AGENTS.md) so Codex and Claude Code are unaffected.
- **Step-1 gsudo controller precheck**: When starting or resuming any non-Q&A task in the QKeyMapper workspace that may involve code modifications, first run a read-only check before beginning edits:
  `out/mcp_bridge/venv/Scripts/python.exe -B -X utf8 .agents/skills/qkeymapper-gsudo-cache/scripts/cache_control.py status`
- If no active controller is running (absent, closed, or expired), immediately remind the user upfront whether they want to start the external gsudo controller in an external non-elevated terminal so subsequent administrator-privileged verification or debugging will not stall after code changes.
- Keep this check strictly read-only: do not create, start, restart, upgrade, or extend a gsudo controller from inside Antigravity, do not trigger UAC, and do not block ordinary non-elevated reading, planning, or preparation. Cache status or timing never grants new test scope or resumes paused development; follow [.agents/skills/qkeymapper-gsudo-cache/SKILL.md](.agents/skills/qkeymapper-gsudo-cache/SKILL.md).
