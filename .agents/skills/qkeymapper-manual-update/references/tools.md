# 手册维护工具

以下命令从仓库根目录运行。技能路径可赋给`$skill`；路径都是仓库相对路径，不需要复制历史out或plan。普通正文修订只执行生成及相关浏览器检查，不构建截图助手。

## 依赖与生成

- 生成：Python 3.9+，标准库即可；有多套Python时选择真实解释器，避免Windows商店占位命令。
- 网页检查：Node.js、可供其加载的Playwright及本机Chrome／Edge。通过正常Node包解析查找，找不到时用`--module`传包目录或`QKM_PLAYWRIGHT_MODULE`。不自动下载浏览器或安装依赖。
- 新界面截图：Windows开发PC、PowerShell 7、Qt6.8.3 x64 MSVC套件、MSVC x64工具、JOM及Windows SDK的mt.exe。Qt通过`-QtRoot`、QTDIR或PATH查找，VS通过vswhere或`-VcVars`，JOM可用`-Jom`指定。依赖不齐时报具体路径，不跳过或安装驱动。

```powershell
$skill = '.agents/skills/qkeymapper-manual-update'
python readme/manual/build.py
node "$skill/scripts/check_browser.cjs" --chapters quickstart,gamepad --query 原始按键 --query 双重输入
```

`--chapters`接受逗号分隔的article／section ID，`--query`可重复。检查只在忽略目录生成独立HTML副本、结果和页面截图；成功不等于所有搜索结果语义正确，仍需审阅对应结果及截图。脚本检查两种阅读主题、PNG解码、离线资源、窄窗口和首章图片放大／Escape；不取代按改动追加的键盘、主题存储或历史导航检查。

## 构建和启动观察助手

```powershell
pwsh -NoProfile -File "$skill/scripts/build_probe.ps1" -QtRoot $env:QTDIR
pwsh -NoProfile -File "$skill/scripts/launch_probe.ps1"
```

Python不在PATH时传`-Python`。构建只生成`out/user_manual/observer/`，从当前产品main.cpp派生入口：禁用Hook／Interception启动连接，接入独立观察控制器；保留产品线程退出流程。源码锚点缺失／重复则失败，需核对新启动路径后局部适配生成器，不能退回旧入口快照。

观察目标链接正式manifest；启动器只给当前会话副本替换asInvoker，复制安全演示INI，不改正式EXE／manifest／用户INI。每次启动使用唯一会话目录及单实例名称。构建产物和DLL不进Git；DLL及简中Qt标准按钮翻译从本机Qt部署及仓库已有非Qt库获取，不依赖plan中的解压发布包。若配图出现英文标准按钮，先核对匹配翻译资源，不能把缺失资源计作中文验收。

启动命令持续运行至正常退出，打印`Session=...`及私有桌面名称；另开控制执行上下文操作该会话。会话位于`out/user_manual/observer/sessions/<唯一ID>/`，等待`response_ready.json`，而不是盲等固定秒数。查看状态中的实际桌面、DPR、屏幕矩形及原生可见性；不足2K／100%时报告，不偷偷调整显示器或把图片缩放冒充标准截图。

## 控制、截图与关闭

```powershell
. "$skill/scripts/control.ps1"
$session = '<启动器输出的Session绝对路径>'
$s = Send-ManualCommand -Session $session -Command @{action='state'}
$s.roots
Save-ManualCapture -State $s -Destination "$session/captures" -Label main
$s = Send-ManualCommand -Session $session -Command @{action='editor'}
Save-ManualCapture -State $s -Root editor -Destination "$session/captures" -Label item
Send-ManualCommand -Session $session -Command @{action='quit'}
```

调用经过产品控件和入口，程序信号操作不表述为硬件鼠标。`editor`从实际当前映射项打开编辑窗；其他窗口使用实际按钮打开，`click`的`name`来自UI objectName，`root`可用可见窗口类名。若同类弹窗有多个，先检查当前窗口清单；不能随意选择第一个、重建业务窗口或用show()跳过初始化。

常用请求：

| action | 参数／用途 |
|---|---|
| state | 当前根、窗口、条目、控件和选项；状态文件只留out |
| tab | index：主窗口左下七页，0起算 |
| set | root（默认main）、name、text／checked／value；填写控件 |
| click | root、name；点击实际按钮，可能打开模态，后续用状态确认结果 |
| editor／hide-editor | 打开第一条映射项／隐藏编辑窗 |
| select | first、last、column：连续行选择；不得越界 |
| copy／paste／move-down／delete-rows | 当前专用配置中的真实表格操作 |
| new-tab／mapping-tab／setup | text为新表名／index选择表／打开当前表属性 |
| category-view／category-header | 显示分类列／打开表头筛选 |
| category-input／category-apply／check-label | text、checked：分类弹层的编辑与筛选 |
| validate-mapping／regex-check | cases数组：使用实际校验器验证例子，不发送输入 |
| quit | 正常退出；之后确认启动器exit.txt为0 |

更多已有操作看`assets/manual_probe.h`，仅按本次需要阅读／局部适配。请求同步串行，超时先看真实模态与状态，不重复点击或解除主窗口禁用。未知action、受限系统按钮、菜单激活会报错，不将请求成功当作功能已执行。IN/OUT文件只用当前专用会话目录，不能指向用户或正式配置。

截图通过既有`scripts/capture_ui_snapshots.ps1`的PrintWindow，在目标私有桌面启动捕获worker。该脚本会尝试SetForegroundWindow，但不切换活动桌面或向Default发送输入。窗口必须原生可见、归属对应PID／私有桌面；捕获失败不算通过。日志及原始PNG留会话目录；逐张自主审阅后才复制审核图到手册images。菜单／分离弹层可能需要各自捕获并按记录位置合成，需注明方法，不重绘内容。

## 安全边界与维护

- 助手只用于UI编辑和截图。没有“开始映射／启用Hook”模式；驱动安装、输入路径启用、系统设置和菜单执行不在默认控制范围。部分系统相关控件被拒绝，不能为截图取消保护。查看设备列表后取消，不确认设备禁用。
- 默认隐藏非受控演示窗口的进程行；并不替代截图隐私检查。不要运行游戏或读取办公标题来演示。新增图仍需检查配置名、路径、设备标识和标题。
- 正常quit关闭自启实例；超时会报告PID，不擅自杀进程。确认退出后再清理会话。不要关闭用户已有实例或自动扩大Default范围。
- 外壳／生成器保持单一维护位置；skill只引用，避免复制后漂移。轻量自检：`python "$skill/scripts/prepare_probe.py" --self-test`。工具变化时运行自检及受影响实际操作；不将当前开发PC测试宣称为第二台PC／Win7实机测试。
