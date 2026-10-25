# MCP UI 英中日翻译建议

本页仅提供本次 UI 翻译建议和上下文，不维护进度；状态以[唯一主计划](qkeymapper_mcp_automation_plan_codex.md)为准。正式 `.ts/.qm` 由用户更新。

共 31 个词条。源文本保持英文及空格、标点、大小写；`%1`／`%2`／`%3` 均须保留。下表 `\n` 表示实际换行，XML 中可使用真实换行；两个状态后缀的开头标点／空格用于与状态文本拼接，不要无意删除。

使用项目的 lupdate 更新三份正式 `.ts`，在下列 context 填写译文，再用匹配 Qt 工具链的 lrelease 生成 `.qm`。英文可使用原文。已有翻译不要全局清理或批量替换；只审阅本次相关差异。

## QkmAutomation::ControlDialog

标题由控制窗口、通用高级设定入口和托盘共用，仅需翻译此 context 下的一份标题。

### 1

| 语言 | 文本 |
|---|---|
| English | Enable MCP for this run |
| 中文 | 启用本次运行的 MCP 接口 |
| 日本語 | 今回の実行で MCP を有効にする |

### 2

| 语言 | 文本 |
|---|---|
| English | Only this user and logon session may connect. Approve pairing here. Closing this window keeps MCP enabled; restarting QKeyMapper disables it. |
| 中文 | 仅允许同一用户登录会话。请在此批准配对；关闭窗口不会关闭接口。重启软件后接口关闭。 |
| 日本語 | 同じユーザーのログオンセッションのみ接続できます。ここでペアリングを承認してください。画面を閉じても有効のままです。再起動すると無効になります。 |

### 3

| 语言 | 文本 |
|---|---|
| English | MCP is disabled |
| 中文 | MCP 接口已关闭 |
| 日本語 | MCP は無効です |

### 4

| 语言 | 文本 |
|---|---|
| English | Connection information file |
| 中文 | 连接信息文件路径 |
| 日本語 | 接続情報ファイルのパス |

### 5

| 语言 | 文本 |
|---|---|
| English | Copy this path when configuring the MCP bridge. It contains no pairing key. |
| 中文 | 可复制此路径配置 MCP 桥接。文件不含配对密钥。 |
| 日本語 | MCP ブリッジの設定にこのパスをコピーできます。ペアリングキーは含まれません。 |

### 6

| 语言 | 文本 |
|---|---|
| English | Paired clients and permissions |
| 中文 | 已配对客户端及权限 |
| 日本語 | ペアリング済みクライアントと権限 |

### 7

| 语言 | 文本 |
|---|---|
| English | Revoke selected pairing |
| 中文 | 撤销选中客户端的配对 |
| 日本語 | 選択したペアリングを解除 |

### 8

| 语言 | 文本 |
|---|---|
| English | Approve a new client (within 60 seconds) |
| 中文 | 批准新的客户端（60 秒内） |
| 日本語 | 新しいクライアントを承認（60 秒以内） |

### 9

| 语言 | 文本 |
|---|---|
| English | Verified client executable path |
| 中文 | 系统核实的客户端程序路径 |
| 日本語 | 確認済みクライアントのパス |

### 10

| 语言 | 文本 |
|---|---|
| English | Read state and configuration (required) |
| 中文 | 读取状态与设定（必需） |
| 日本語 | 状態と設定の読み取り（必須） |

### 11

| 语言 | 文本 |
|---|---|
| English | Change mappings and settings |
| 中文 | 更改映射与软件设定 |
| 日本語 | マッピングと設定の変更 |

### 12

| 语言 | 文本 |
|---|---|
| English | Start and stop mapping (may send input) |
| 中文 | 开始与停止映射（可能发送输入） |
| 日本語 | マッピングの開始と停止（入力を送信する場合あり） |

### 13

| 语言 | 文本 |
|---|---|
| English | Approve selected permissions |
| 中文 | 批准选定权限 |
| 日本語 | 選択した権限で承認 |

### 14

| 语言 | 文本 |
|---|---|
| English | Reject |
| 中文 | 拒绝 |
| 日本語 | 拒否 |

### 15

| 语言 | 文本 |
|---|---|
| English | Close |
| 中文 | 关闭 |
| 日本語 | 閉じる |

### 16

| 语言 | 文本 |
|---|---|
| English | MCP is enabled |
| 中文 | MCP 接口已启用 |
| 日本語 | MCP は有効です |

### 17

| 语言 | 文本 |
|---|---|
| English | ; the last operation failed. A revocation that was not saved did not succeed. |
| 中文 | ；上次操作失败。未保存的撤销不能视为成功。 |
| 日本語 | 。前の操作に失敗しました。保存されなかった解除は完了していません。 |

### 18

| 语言 | 文本 |
|---|---|
| English |  Another instance owns this pairing store; manage it there or disable its interface first. |
| 中文 |  配对存储正由另一实例使用，请在该实例中管理或先关闭其接口。 |
| 日本語 |  別のインスタンスが保存先を使用中です。その画面で管理するか、先に接続を無効にしてください。 |

### 19

| 语言 | 文本 |
|---|---|
| English | Read |
| 中文 | 读取 |
| 日本語 | 読取 |

### 20

| 语言 | 文本 |
|---|---|
| English | Configure |
| 中文 | 设定 |
| 日本語 | 設定 |

### 21

| 语言 | 文本 |
|---|---|
| English | Run mapping |
| 中文 | 启停 |
| 日本語 | 開始・停止 |

### 22

| 语言 | 文本 |
|---|---|
| English | Observe |
| 中文 | 诊断 |
| 日本語 | 診断 |

### 23

| 语言 | 文本 |
|---|---|
| English | Test input |
| 中文 | 真实输入测试 |
| 日本語 | 実入力テスト |

### 24

| 语言 | 文本 |
|---|---|
| English | Manage devices |
| 中文 | 设备管理 |
| 日本語 | デバイス管理 |

### 25

| 语言 | 文本 |
|---|---|
| English | Execute commands |
| 中文 | 执行命令 |
| 日本語 | コマンド実行 |

### 26

| 语言 | 文本 |
|---|---|
| English | Client name (self-reported): %1\nPID: %2; creation time: %3\nVerified executable: |
| 中文 | 客户端名称（自报）：%1\nPID：%2；创建时间：%3\n系统核实的程序： |
| 日本語 | クライアント名（自己申告）：%1\nPID：%2；作成時刻：%3\n確認済みプログラム： |

### 27

| 语言 | 文本 |
|---|---|
| English | Agent / MCP Control |
| 中文 | Agent / MCP 控制 |
| 日本語 | Agent / MCP 操作 |

## QKeyMapper

以下仅改变界面解释文本，不改变停止或校验语义。

### 28

| 语言 | 文本 |
|---|---|
| English | Mapping cleanup is pending. Editing and restarting remain disabled. |
| 中文 | 正在清理映射任务，完成后才能编辑或重新开始。 |
| 日本語 | マッピング処理を終了しています。完了まで編集と再開はできません。 |

### 29

| 语言 | 文本 |
|---|---|
| English | Stop wait timed out. Cleanup continues; editing and restarting remain disabled. |
| 中文 | 停止等待已超时，仍在清理任务；编辑和重新开始继续禁用。 |
| 日本語 | 停止待機がタイムアウトしました。処理終了まで編集と再開はできません。 |

### 30

| 语言 | 文本 |
|---|---|
| English | The mapping target or conflicts changed. Review the operation again. |
| 中文 | 映射目标或冲突已变化，请重新核对本次操作。 |
| 日本語 | 対象のマッピングまたは競合が変更されました。操作内容を再確認してください。 |

### 31

| 语言 | 文本 |
|---|---|
| English | Mapping edits require idle state and completed stop cleanup. Close the current modal dialog and try again. |
| 中文 | 修改映射需要空闲且停止清理已完成，请关闭当前模态窗口后重试。 |
| 日本語 | マッピングの編集には待機状態と停止処理の完了が必要です。現在のモーダルダイアログを閉じて再試行してください。 |

## 资源更新后的交接

本次用户已完成正式资源回填，Agent已完成所选正式验收并共同提交；下文保留三条文案决策及以后维护时的资源更新流程，当前进度仍以唯一主计划为准。

2026-10-26 用户审阅后，上述两条状态中文补齐“MCP 接口”，避免只显示“接口”的指代差异。关闭按钮英文源从 `Close window` 简化为 `Close`，中文“关闭”、日文“閉じる”；只关闭窗口、接口保持启用的既有行为由说明文字表达，不改变按钮行为。请重新lupdate后在 `QkmAutomation::ControlDialog` context 填写这三条，再生成三份正式QM；其余已更新译文保持不动。

更新完本轮三份 `.ts` 及对应 `.qm` 后，告知 Agent“正式翻译资源已更新，请继续最终验收并共同提交”。Agent 应复核正式资源加载与三语言截图，连同代码、规则和记录一起形成一个独立本地提交；在此之前不得提前提交。

用户已为 MCP 入口选择 `autoDefault=false` 与 `NoFocus`，正常鼠标点击保留，不要求通过 Tab／Space 激活；随后明确 MCP 对话框也遵循同类窗口的约定：按钮／复选框为NoFocus，可选择文本的标签、只读输入框及客户端列表为ClickFocus，保留点击后的复制／选择及列表按键操作，默认不启用Tab焦点导航。
