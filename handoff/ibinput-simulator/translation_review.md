# IbInput 新增文案审阅稿

这是正式 TS/QM 更新前的文案提案，不维护开发进度；状态只见 [唯一主计划](qkeymapper_ibinputsimulator_integration_plan.md)。批准前不写正式翻译资源。

每组注明 Qt context、占位符和换行。正文没有实际 LF；帮助说明以 HTML <br> 换行，链接目标保持不变。所有 %1／%2 原样保留。品牌名称保持原文；英文资源使用英文源本身。

此稿覆盖本检查点新增可见源文，批准后由 Agent 使用项目 lupdate-pro／lrelease 更新对应 TS/QM，并核对加载及运行时刷新；完整产品三语 UI 仍按主计划验收。

## Context: QkmIbInputClient

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Invalid IbInput preferences; defaults were used.` |
| 中文 | `IbInput 偏好无效，已使用默认值。` |
| 日本語 | `IbInput の設定が無効なため、既定値を使用しました。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Previous IbInput cleanup is incomplete.` |
| 中文 | `上一次 IbInput 清理尚未完成。` |
| 日本語 | `前回の IbInput の後処理が完了していません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `The selected IbInput backend does not support this mapping or still awaits validation.` |
| 中文 | `所选 IbInput 后端不支持此映射，或相关能力尚待验证。` |
| 日本語 | `選択した IbInput バックエンドはこのマッピングに対応していないか、検証待ちです。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput support is not included in this build.` |
| 中文 | `此构建未包含 IbInput 支持。` |
| 日本語 | `このビルドには IbInput サポートが含まれていません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput startup was cancelled.` |
| 中文 | `IbInput 启动已取消。` |
| 日本語 | `IbInput の起動をキャンセルしました。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Connected; input reception has not been verified.` |
| 中文 | `已连接；输入接收情况尚未验证。` |
| 日本語 | `接続済み。入力の受信は未確認です。` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput connection failed: %1` |
| 中文 | `IbInput 连接失败：%1` |
| 日本語 | `IbInput の接続に失敗しました: %1` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `SendInput is frozen for this run; configuration is unchanged.` |
| 中文 | `本轮已固定使用 SendInput；配置未改变。` |
| 日本語 | `この実行では SendInput を使用します。設定は変更されません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `This IbInput event was not admitted for the current run.` |
| 中文 | `本轮准入未允许此 IbInput 输入事件。` |
| 日本語 | `この IbInput 入力イベントは、現在の実行では許可されていません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Unsupported IbInput keyboard event.` |
| 中文 | `不支持此 IbInput 键盘事件。` |
| 日本語 | `この IbInput キーボードイベントには対応していません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `The IbInput keyboard report is full.` |
| 中文 | `IbInput 键盘报告的普通键槽位已满。` |
| 日本語 | `IbInput キーボードレポートの通常キー枠がいっぱいです。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput send failed; mapping must stop without retrying another backend.` |
| 中文 | `IbInput 发送失败；必须停止映射，不会换后端重试。` |
| 日本語 | `IbInput の送信に失敗しました。別のバックエンドで再試行せず、マッピングを停止します。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Unsupported IbInput mouse event.` |
| 中文 | `不支持此 IbInput 鼠标事件。` |
| 日本語 | `この IbInput マウスイベントには対応していません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Unsupported combined IbInput mouse event.` |
| 中文 | `不支持此 IbInput 鼠标组合事件。` |
| 日本語 | `この IbInput マウスイベントの組み合わせには対応していません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `This IbInput mouse button still awaits backend validation.` |
| 中文 | `此 IbInput 鼠标按钮尚待后端验证。` |
| 日本語 | `この IbInput マウスボタンはバックエンドでの検証待ちです。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `This IbInput button combination still awaits backend validation.` |
| 中文 | `此 IbInput 按钮组合尚待后端验证。` |
| 日本語 | `この IbInput ボタンの組み合わせはバックエンドでの検証待ちです。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput wheel send failed; mapping must stop.` |
| 中文 | `IbInput 滚轮发送失败；必须停止映射。` |
| 日本語 | `IbInput のホイール送信に失敗しました。マッピングを停止します。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Unsupported IbInput event type.` |
| 中文 | `不支持此 IbInput 事件类型。` |
| 日本語 | `この IbInput イベントの種類には対応していません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput release is incomplete; automatic restart is blocked.` |
| 中文 | `IbInput 释放未完成，已阻止自动重启映射。` |
| 日本語 | `IbInput の解放が完了していないため、自動再起動を禁止しています。` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput is mixed with another send method in one mouse movement group: %1` |
| 中文 | `同一鼠标移动组混用了 IbInput 和其他发送方式：%1` |
| 日本語 | `同じマウス移動グループに IbInput と別の送信方式が混在しています: %1` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `This IbInput feedback/blocking combination still awaits validation: %1` |
| 中文 | `此 IbInput 反馈／屏蔽组合尚待验证：%1` |
| 日本語 | `この IbInput のフィードバック／ブロックの組み合わせは検証待ちです: %1` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Complex IbInput combinations, repetition, locking or stop/tab commands still await validation: %1` |
| 中文 | `复杂 IbInput 组合、重复、锁定或停止／切表命令尚待验证：%1` |
| 日本語 | `複雑な IbInput の組み合わせ、繰り返し、ロック、停止／テーブル切替コマンドは検証待ちです: %1` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput output can trigger an original key; this feedback path awaits validation: %1` |
| 中文 | `IbInput 输出可能触发原始按键，此反馈路径尚待验证：%1` |
| 日本語 | `IbInput の出力が元のキーを発火する可能性があります。このフィードバック経路は検証待ちです: %1` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `This IbInput mouse button still awaits backend validation: %1` |
| 中文 | `此 IbInput 鼠标按钮尚待后端验证：%1` |
| 日本語 | `この IbInput マウスボタンはバックエンドでの検証待ちです: %1` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `This IbInput key representation still awaits validation: %1` |
| 中文 | `此 IbInput 键码表示尚待验证：%1` |
| 日本語 | `この IbInput キー表現は検証待ちです: %1` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput macro/repeat expansion still awaits validation: %1` |
| 中文 | `IbInput 宏／重复展开尚待验证：%1` |
| 日本語 | `IbInput のマクロ／繰り返し展開は検証待ちです: %1` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput send failed; mapping must stop.` |
| 中文 | `IbInput 发送失败；必须停止映射。` |
| 日本語 | `IbInput の送信に失敗しました。マッピングを停止します。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput absolute positioning is not available for this backend.` |
| 中文 | `此后端尚未开放 IbInput 绝对定位。` |
| 日本語 | `このバックエンドでは IbInput の絶対位置指定は使用できません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Wait for IbInput cleanup before starting.` |
| 中文 | `请等待 IbInput 清理完成后再启动。` |
| 日本語 | `IbInput の後処理が完了してから開始してください。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput tab switching is awaiting lifecycle validation; mapping was stopped.` |
| 中文 | `IbInput 切表尚待生命周期验证；映射已停止。` |
| 日本語 | `IbInput のテーブル切替はライフサイクル検証待ちです。マッピングを停止しました。` |

占位符：%1、%2；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Table %1, row %2` |
| 中文 | `表 %1，第 %2 行` |
| 日本語 | `テーブル %1、行 %2` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Row %1` |
| 中文 | `第 %1 行` |
| 日本語 | `行 %1` |

## Context: QkmIbInputPopup

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Preferred backend` |
| 中文 | `首选后端` |
| 日本語 | `優先バックエンド` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `If unavailable` |
| 中文 | `无法连接时` |
| 日本語 | `接続できない場合` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Automatic (recommended)` |
| 中文 | `自动（推荐）` |
| 日本語 | `自動（推奨）` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Logitech legacy` |
| 中文 | `Logitech 旧版通道` |
| 日本語 | `Logitech 旧方式` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Do not start mapping` |
| 中文 | `不启动映射` |
| 日本語 | `マッピングを開始しない` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Use SendInput before start` |
| 中文 | `启动前改用 SendInput` |
| 日本語 | `開始前に SendInput に切り替える` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Refresh status` |
| 中文 | `刷新状态` |
| 日本語 | `状態を更新` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Check connection` |
| 中文 | `连接检查` |
| 日本語 | `接続を確認` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `May activate the vendor virtual device. No test input is sent. Available only after mapping cleanup.` |
| 中文 | `可能激活厂商虚拟设备，不发送测试输入。仅在映射清理完成后可用。` |
| 日本語 | `メーカーの仮想デバイスを有効にする場合があります。テスト入力は送信しません。マッピングの後処理完了後のみ使用できます。` |

占位符：无；换行：HTML <br>。

| 语言 | 文案 |
|---|---|
| English | `<a href="https://www.logitechg.com/innovation/g-hub.html">G HUB</a> \| <a href="https://www.razer.com/synapse">Razer Synapse</a><br>Connection and accepted calls do not prove that an application received input.` |
| 中文 | `<a href="https://www.logitechg.com/innovation/g-hub.html">G HUB</a> \| <a href="https://www.razer.com/synapse">Razer Synapse</a><br>连接成功或调用被接受，并不代表应用已收到输入。` |
| 日本語 | `<a href="https://www.logitechg.com/innovation/g-hub.html">G HUB</a> \| <a href="https://www.razer.com/synapse">Razer Synapse</a><br>接続成功や呼び出しの受理は、アプリでの入力受信を保証しません。` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `G HUB version: %1` |
| 中文 | `G HUB 版本：%1` |
| 日本語 | `G HUB バージョン: %1` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Unknown` |
| 中文 | `未知` |
| 日本語 | `不明` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Observed process names: %1` |
| 中文 | `观察到的进程名：%1` |
| 日本語 | `確認したプロセス名: %1` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `G HUB driver registration: %1` |
| 中文 | `G HUB 驱动登记：%1` |
| 日本語 | `G HUB ドライバー登録: %1` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Observed` |
| 中文 | `已观察到` |
| 日本語 | `確認済み` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Razer installation and driver: Unknown` |
| 中文 | `Razer 安装与驱动：未知` |
| 日本語 | `Razer のインストールとドライバー: 不明` |

占位符：%1、%2；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Backend: %1; fallback: %2` |
| 中文 | `后端：%1；是否回退：%2` |
| 日本語 | `バックエンド: %1、代替方式: %2` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Not connected` |
| 中文 | `未连接` |
| 日本語 | `未接続` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Yes` |
| 中文 | `是` |
| 日本語 | `はい` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `No` |
| 中文 | `否` |
| 日本語 | `いいえ` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Last result: %1` |
| 中文 | `最近结果：%1` |
| 日本語 | `前回の結果: %1` |

占位符：%1；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Sampled: %1` |
| 中文 | `采样时间：%1` |
| 日本語 | `確認日時: %1` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Stop mapping and wait for cleanup before changing preferences.` |
| 中文 | `请停止映射并等待清理完成后再修改偏好。` |
| 日本語 | `マッピングを停止し、後処理が完了してから設定を変更してください。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Status refresh does not initialize a driver.` |
| 中文 | `刷新状态不会初始化驱动。` |
| 日本語 | `状態の更新ではドライバーを初期化しません。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `This checkpoint enables G HUB relative movement only. Other outputs await validation.` |
| 中文 | `此检查点仅开放 G HUB 相对移动，其他输出尚待验证。` |
| 日本語 | `このチェックポイントでは G HUB の相対移動のみ有効です。他の出力は検証待ちです。` |

## Context: QKeyMapper

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Wait for IbInput cleanup before starting or changing mappings.` |
| 中文 | `请等待 IbInput 清理完成后再启动或更改映射。` |
| 日本語 | `IbInput の後処理が完了してから、マッピングを開始または変更してください。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Wait for the IbInput connection check to finish.` |
| 中文 | `请等待 IbInput 连接检查完成。` |
| 日本語 | `IbInput の接続確認が完了するまでお待ちください。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Wait for IbInput cleanup before saving.` |
| 中文 | `请等待 IbInput 清理完成后再保存。` |
| 日本語 | `IbInput の後処理が完了してから保存してください。` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `IbInput settings...` |
| 中文 | `IbInput 设置...` |
| 日本語 | `IbInput 設定...` |

占位符：无；换行：无。

| 语言 | 文案 |
|---|---|
| English | `Not connected` |
| 中文 | `未连接` |
| 日本語 | `未接続` |
