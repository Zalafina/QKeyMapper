# IB-MCP-138：固定 IbInput 核心接纳的选定证据

[唯一进度与停止位置](qkeymapper_mcp_automation_plan_codex.md) · [IbInput唯一计划](../ibinput-simulator/qkeymapper_ibinputsimulator_integration_plan.md)。本页仅记录证据，不维护进度。

PC-W11，2026-11-09；MCP输入`94f0bb8485e7e607cff5c73bf6c0a45725e052d9`、固定IbInput输入`b5b484a63bf97716c407ae9680fd1b63f8d5eb01`，共同祖先`0d053c17590589f9a2b8492b61ab7e77b915456a`。检查时为独立集成树的待提交普通合并及本轮局部差异，最终提交由两计划卡片及Git记录。开发／Git／构建／分析／SDK／图片普通权限；仅原manifest产品／固定助手由公共登记提权。

## 源码、制品与资源

- 集成树：`D:/agent_home/.codex/worktrees/ibinput-mcp-integration/QKeyMapper`；构建／组件／适配驱动原件在其`out/ibinput_mcp_integration/`。实际产品运行原件在原MCP工作区`out/mcp_product/ibinput138_20261109/`，以下简称QA。两个out均只在本机，不随Git。
- 最终Qt6.8.3／MSVC14.44普通Release：集成树`out/build_qt6/release/QKeyMapper.exe`，SHA256 `f7302cee3861dc7fb59b97412c7aedc4616015696e86158c82c313999f46de7e`。最终真实输入维护目标：`out/ibinput_mcp_integration/product-check/release/QKeyMapperIbInputProductCheck.exe`，SHA256 `585fe1e7282f177254f3025f6158e9678e0cd678b6e6629165bfe59e593c46ef`；实际部署分别位于QA各Default组的runtime。后者保留真实main／manifest／业务入口，包含维护探针，不能称为普通产品EXE的新运行验收。
- 普通Release的GUI／MCP配置组实际测试EXE为QA下`gui_runtime`、`mcp_runtime`及`sources/mcp_runtime`中的`QKeyMapperMcpAcceptance_ibinput_qt6.exe`，SHA256 `ec7250ea10a323944e2223a702796ca7d2c99cd570a8c92102e8a0dadc1a3531`。它在最终无代次重启过滤修正前构建；该修正不在这些空闲配置场景的执行路径，配置证据原范围沿用，修正另由真实维护入口定向复验。
- 0009 x64 DLL保持SHA256 `2d8c46db5b7b5b8b5504321aa43bedc02e174b1fd758a7dd91b5da86e902ef30`，x86制品／许可亦沿固定输入，未重造依赖。TS按context／source／comment等身份三方并集保留双方词条（en/zh1415，ja1414），Qt6 lrelease重生QM均1291 finished、0 unfinished，无新译文。v1.1手册及维护源／图／公共导航保留。

## 选定检查

| 范围 | 实际结果与原件 |
|---|---|
| 构建／静态 | 最终Qt6普通Release及维护目标native0、零编译链接警告；Qt5.15.2 x64／x86兼容构建native0，只见已批准官方qtmain缺PDB LNK4099。static-analysis-04为6文件／17编译上下文，最终局部修改后05复查主窗口2上下文，均项目诊断0、工具退出0；Qt／第三方原诊断仍在raw日志，不称全部raw零告警 |
| 组件／桥接 | Qt6控制目标私有QkmMcp桌面12项通过；客户端20组、生产能力替身矩阵通过，Python正式SDK对固定端点的工具结果／schema8项通过。参数0–3、错误类型／字段、结果绑定及观察时间与语义状态分离均核验；不是普通产品验收 |
| 独立GUI基线 | QA/gui-12：High普通Release实际私有QkmProduct桌面，从版本化空白INI真实添加A→B、窗口选择方式3和Razer／启动前SendInput偏好、实际输入名称并点击保存；正常退出后独立冷读方法3及偏好。普通新桥接复用真实GUI批准的专属Read+Configure配对，无Run。两进程exit0、凭据撤销及FilterKeys前后相同 |
| 正式MCP业务 | QA/mcp-01、02：实际认证stdio路径的方式3新增、受限更新、偏好预览／取消／提交；同值不推进修订，GUI语义变化使旧预览失效，被动查询后合法预览仍可提交。共享两键列入完整保存足迹；取消保存保留草稿，普通进程只锁自有INI阻止替换后实际save_write_failed／failed operation、磁盘不变、草稿／偏好保留；新预览恢复保存并独立冷读通过。重启后旧源／预览失效、旧operation为unknown，两代正常退出及专属配对清理 |
| 权限／源边界 | QA/read-01：真实GUI只授Read，配置／偏好／保存预览均permission_denied；重复读不改变源、配置、偏好修订或INI。QA/sources-02：有效Common、可见禁用行更新、Common冲突自动禁用新行、同移动组混用拒绝且零修改／落盘，完整保存及冷读通过；仅在产品退出后将自有样例HideDisabled设true，冷启动仍可读禁用源行但修改隐藏行invalid_target，无落盘或连接副作用 |
| 生命周期／释放 | 客户端替身覆盖旧失败与新请求、延迟发送／关闭、释放失败继续阻止重启；QA/lifecycle-private-01真实维护入口通知注入复现无代次旧重启解除新清理屏障，native2。只补零代次且client.busy时拒收；02分离旧代及零代通知断言通过，4次结构冲突保存拒绝字节不变，匹配代次释放后可保存，native0。是组件故障注入，不是厂商真实故障或GUI点击等价性 |
| 实际ASan | 客户端实际动态ASan导入、生产能力替身矩阵native0／无报告。实际Worker／SendInputTask的wait_clear、queue_keyup、cancel_reset、exit_wait、borrowers五项在自有私有桌面通过；最终源码的相同五项QA/asan-02亦全部native0／passed且无ASan报告。不用于性能结论，不是驱动输入ASan |
| 私有演示守卫 | QA/output-demo-01：真实configuration-only入口、产品自身只读私有记录及正式MCP两图／回收，native0／25项通过。有效事务播种后10个场景分别核IbSendInit／IbSendInput／IbSendDestroy／SendInput／SetCursorPos边界全零，退出仍零；普通策略11项正对照均可达。此维护配置以记录替身替换原生输出边界，防止守卫失败时发真实输入；不冒充普通Release厂商I/O验收。高级窗A→B和单条窗C→D真实PNG已自主检查，未向QQ发送 |
| Default真实接收 | 四个组QA/default-{ghub,razer}-{keyboard,absolute}-01均产品／普通接收窗exit0，实际Default归属、前台和输入范围逐步核验。三组固定原键到F／2／F24实际收发，持有停止／重启／退出释放；鼠标覆盖屏幕／窗口坐标、点按、保存／恢复、缺失目标UP及持有释放，两后端各5次Left DOWN／UP匹配，最终按键／按钮保持0。G HUB另两次实际切表，新代1→4；Razer为1→2。四张仓库工具接收窗截图已自主检查，无黑图／错误窗 |

两条独立冷读的A→B行全部74个持久化字段和两项偏好比较相同，原件为QA/gui-mcp-equivalence.json；不称完整INI逐字节相等或全部软件设置等价。Default实测仅当前PC-W11、2560×1440／虚拟原点0、指针速度10／参数6,10,1、FilterKeys flags58且时间参数0。G HUB11个定位点各轴误差≤2px并有后续稳定采样；Razer9点保持其原生路径实测范围。夹具显式光标居中／8px轻移及有条件恢复只用于自有接收窗，不补偿映射算法，不改系统指针／筛选键设置。用户主动改变前台不算产品干扰；失焦时守卫停止后续输入并只清理自有保持。本批不是任意游戏、多屏／DPI或所有驱动版本保证。

本轮PID1884剩余五项授权已按历史中的instance／definition_hash／target_id关联为本任务，撤出后18:36:05技能off及后续status核实closed、scope及五项在途资源零、cache_available=false、native_sessions_observed=0；未观察到其他QKeyMapper进程。历史未知请求仍保留，未接管或重放。

## 失败轮次与边界

- 最初构建传参、两处schema初始化括号、客户端user32链接及ASan三个旧调用签名均有原失败日志，修正后的实际构建／检查另记；静态分析前两轮路径前置错误不能算分析通过。
- QA/gui-01部署缺少SDL2等已核实导入依赖，原生Start wrapper退出0／报告PID19664，进程身份复核取得Windows错误87；未取得目标实际退出码，不推算装载错误码。旧控制器以start_result_unknown关闭，原请求`b588469d172e4e4bbff2b4fc1797792a`保留且未重放；用户另批后才创建新控制器1884至19:00。
- gui-02至11分别保留重复模态打开、venv启动器路径断言、错误的空白草稿前置、Default专用助手边界、Qt完整AutomationId／自定义Combo假设等失败。gui-09固定探针对不支持模式的空指针解引用，实际退出`-1073741819`；已修夹具，不写成产品崩溃。gui-11业务保存和冷读断言通过但最后清理打开顺序失败；12才是完整通过轮次。sources-01误用目录字段is_common，实际契约kind=common；02通过。
- 只复用未受影响历史证据；本轮无Win10新运行、FakerInput专项、其他后端／物理设备完整矩阵、PC重启／跨Windows登录、三包Release验证或QQ gateway联调。用户转交的指定DLL／低级Hook dwExtraInfo结果仍按IbInput计划§34.7原范围，未重跑或扩展。
- 主动MCP设备检查／试发工具、ibinput_settings受限窗口角色及其权限／演示验收尚未交付；不把产品自身Popup或维护探针算成正式窗口协议支持。完整M1／M2及PERSIST-137不因本切片完成。
