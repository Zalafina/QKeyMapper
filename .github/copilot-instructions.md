# QKeyMapper Workspace Instructions

## Project Context
- This project is a Windows-only Qt Widgets and WinAPI application written in C++.
- Prefer changes that fit the existing QKeyMapper architecture instead of introducing new frameworks or large refactors.
- Favor small, localized modifications that preserve existing behavior and project stability.

## Implementation Rules
- Implement feature extensions or bug fixes on top of the current code structure in the most reasonable way.
- Do not break or weaken existing functionality while making changes.
- If there are concerns about stability, compatibility, performance, driver behavior, or system API side effects, explain the concern and ask the user to help decide.
- Keep file encodings unchanged unless the task explicitly requires otherwise.
- For Qt/C++ source and header files, prefer UTF-8 without BOM.

## Localization And Comments
- Write all newly added code comments in English.
- For new tr("...") strings, provide Chinese and Japanese translation suggestions in the response when relevant.
- Do not modify .ts translation files unless the user explicitly asks for it.

## Planning And Interaction
- Use VS Code planning and askQuestion interactions when the user needs to choose or confirm an implementation direction.
- Write planning questions, confirmation prompts, and summary descriptions in Chinese.
- When presenting fixed options, include a final option that allows custom freeform input.
- Use a ToDo list to show completed and remaining implementation tasks.

## Change Safety
- Perform a final review of all code changes before finishing.
- Confirm that every changed line is intentional and directly related to the task.
- Be especially careful not to accidentally delete unrelated code in nearby functions, UI logic, or settings paths.

## Response Preferences
- Use Chinese when discussing plans, confirmations, and final summaries with the user.
- If implementation becomes blocked or risky, explain the issue clearly and collaborate with the user on the decision.
