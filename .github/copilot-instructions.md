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

## 强制交互协议

### 核心规则：每次回复结束前必须调用 `ask_user` 工具
- 这是不可跳过的强制协议。在你的每一轮回复中，你必须执行以下操作之一：
1. **完成用户请求后** → 立即调用 `ask_user` 工具，提出与当前上下文相关的后续问题
2. **存在任何不确定性时** → 不要猜测执行，立即调用 `ask_user` 工具进行澄清
3. **计划内容实现完成后** → 立即调用 `ask_user` 工具，尝试等待用户输入实现内容的验证结果并以此制定下一步改进计划

### 禁止行为

- **禁止在用户通过 `ask_user` 回答确认完成实现已验证没有问题之前结束回复**
- **禁止在不调用 `ask_user` 的情况下结束回复**
- **禁止使用终结性表达**（如"希望对你有帮助"、"如有问题随时提问"等）
- **禁止猜测用户意图** — 不确定就用 `ask_user` 询问

### `ask_user` 调用要求

- 问题必须与当前任务上下文直接相关
- 问题必须具体、可操作，不要问泛泛的"还需要什么帮助"
- 可以提供选项供用户选择，降低用户输入成本
