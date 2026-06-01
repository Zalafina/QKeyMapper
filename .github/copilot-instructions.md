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
- Perform a review of all code changes.
- Confirm that every changed line is intentional and directly related to the task.
- Be especially careful not to accidentally delete unrelated code in nearby functions, UI logic, or settings paths.

## Response Preferences
- Use Chinese when discussing plans, confirmations with the user.
- If implementation becomes blocked or risky, explain the issue clearly and collaborate with the user on the decision.

## 编译验证临时文件生成位置
- 由用户来实施文件修改后的编译验证，根据用户反馈的编译错误进行适当代码修改。

## 尽量节省输出tokens规则
- 在不影响用户理解的前提下，尽量节省输出tokens，避免冗长的解释和描述。
- 直接给出代码实现，必要时附上简短的说明和注释，避免过多的背景介绍和细节描述。
- 在用户明确要求提供详细解释时，才进行更深入的说明。
