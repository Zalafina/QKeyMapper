# QKeyMapper Workspace Instructions

- Windows-only Qt Widgets + WinAPI C++.
- Keep changes small and local; reuse existing QKeyMapper architecture; avoid new frameworks and large refactors.
- For QKeyMapper work, invoke the qkeymapper-workflow skill as the repo playbook.
- Qt/C++ source files use UTF-8 without BOM; new code comments in English; do not edit .ts files unless asked.
- Plans, confirmations, and summaries in Chinese; use AskUserQuestion for decisions; include a freeform option when offering choices.
- If a change touches stability, driver behavior, or system API side effects, call it out and ask the user to decide.
- After edits, let the user do compile validation unless they ask otherwise.
