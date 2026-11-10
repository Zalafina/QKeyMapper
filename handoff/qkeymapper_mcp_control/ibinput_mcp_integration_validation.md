# IB-MCP-138：固定 IbInput 核心接纳的选定证据

[唯一进度与停止位置](qkeymapper_mcp_automation_plan_codex.md) · [IbInput唯一计划](../ibinput-simulator/qkeymapper_ibinputsimulator_integration_plan.md)。本页仅记录证据，不维护进度。

PC-W11，2026-11-09；MCP输入`94f0bb8485e7e607cff5c73bf6c0a45725e052d9`、固定IbInput输入`b5b484a63bf97716c407ae9680fd1b63f8d5eb01`，共同祖先`0d053c17590589f9a2b8492b61ab7e77b915456a`。验证时为独立集成树的普通合并及本轮局部差异，随后形成集成提交`e73f719b23ee1bd0df99045727acf9554f33959b`并快进接纳至原MCP分支；收尾文档提交不改变产品源码。开发／Git／构建／分析／SDK／图片普通权限；仅原manifest产品／固定助手由公共登记提权。

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

文档收尾核对：本轮修改的UTF-8／无BOM、冲突标记、Python语法、本轮新增相对链接及定向git diff --check通过；固定输入24份受控INI样例逐项与b5b484a6一致，不含本机测试凭据。依赖许可证／补丁／INF的原有空白告警原样保留并核对源blob一致，详见本机final-whitespace-inherited.log；两主计划大小已记录，未为收尾扩大历史归档范围。

## 失败轮次与边界

- 最初构建传参、两处schema初始化括号、客户端user32链接及ASan三个旧调用签名均有原失败日志，修正后的实际构建／检查另记；静态分析前两轮路径前置错误不能算分析通过。
- QA/gui-01部署缺少SDL2等已核实导入依赖，原生Start wrapper退出0／报告PID19664，进程身份复核取得Windows错误87；未取得目标实际退出码，不推算装载错误码。旧控制器以start_result_unknown关闭，原请求`b588469d172e4e4bbff2b4fc1797792a`保留且未重放；用户另批后才创建新控制器1884至19:00。
- gui-02至11分别保留重复模态打开、venv启动器路径断言、错误的空白草稿前置、Default专用助手边界、Qt完整AutomationId／自定义Combo假设等失败。gui-09固定探针对不支持模式的空指针解引用，实际退出`-1073741819`；已修夹具，不写成产品崩溃。gui-11业务保存和冷读断言通过但最后清理打开顺序失败；12才是完整通过轮次。sources-01误用目录字段is_common，实际契约kind=common；02通过。
- 只复用未受影响历史证据；本轮无Win10新运行、FakerInput专项、其他后端／物理设备完整矩阵、PC重启／跨Windows登录、三包Release验证或QQ gateway联调。用户转交的指定DLL／低级Hook dwExtraInfo结果仍按IbInput计划§34.7原范围，未重跑或扩展。
- 138时点主动MCP设备检查／试发工具和ibinput_settings窗口角色尚未交付；139窗口角色续验见下节。主动工具、完整M1／M2及PERSIST-137仍未因核心合流完成。

## IB-MCP-139：固定 IbInput Popup（2026-11-10，PC-W11）

源码为395e684d之后本切片的窗口／桥接／定向测试差异；普通Qt6 Release为作业树`out/build_qt6/release/QKeyMapper.exe`，精确部署至原MCP树`out/mcp_product/ibinput139_20261110/mcp_runtime/QKeyMapperMcpAcceptance_ibinput_qt6.exe`，SHA256为`7cc600d6589731027593dfbff725f069d5c18e1ae14f302a079f706e8a42cb2e`。原manifest保持，Product启动／固定GUI助手走公共登记，普通控制器／SDK／桥接／截图保持Medium；合成INI独立于个人实例，未授RunMapping、未启动映射、未安装驱动或改全局设置。本节不重计138验收。

| 当前检查 | 实际结果与边界 |
|---|---|
| Qt6普通Release | build-product-05原生0／零编译链接警告；GUI助手实际build-gui-qt6-popup139-06原生0，部署身份在QA/deployment-round-06.json。主产品使用原GUI入口，不以维护目标代替普通窗口验收 |
| 双Qt组件 | Qt6／Qt5.15.2 x64在各自实际QkmPopup139私有桌面，各10项含init／cleanup通过。惰性创建、父窗口、权限epoch、连接归属、关闭重开／销毁与严格解析；Qt5共享JSON take导致原window_ref被清空的旧问题先复现，再以读值后remove修正，两种close输入不变／重复解析断言通过。Qt5横幅windows10不是Win10机器证据 |
| 定向静态 | main／window_access／window_image共8编译上下文工具exit0；第一轮window_access的4处临时QList循环产生8条项目诊断，改const局部变量后该文件3上下文复验项目0。main／image原轮项目0复用；第三方原始诊断保留。最后能力／错误字符串澄清不改变已测逻辑 |
| Python | 图片边界10项、正式工具结果8项、原演示编排18项通过；新角色严格元数据／偏好修订、缺省高级角色兼容。没有新增依赖或TS/QM词条 |
| 普通产品popup-07 | PID37764／实际QkmProduct_b555ce636dbd404ab5c2898cdacaccda，正式GUI配对与认证、Show无Capture拒绝、GUI已开窗口不能接管、MCP真实开关和PNG、旧源／旧窗拒绝、重连不继承关闭权；clean源变化及已dirty源上偏好变化分别阻止关闭。窗口／捕获不改配置／INI或偏好语义修订，同值预览仍可提交且modified=false。正常exit0，GUI撤销专属配对且凭据删除 |
| 独立权限popup-read-01 | PID33276／自有私有桌面，Read+Capture=257可捕获GUI已开Popup但不能开关；Read+Show+Capture=385无Configure仍可正式开关和捕获，偏好预览permission_denied。状态／源／INI不变，正常exit0／配对和专属凭据清理 |
| 真实图片 | 普通产品至少GUI打开及MCP打开两种场景的产品内部PNG，460×404、中文可读；自动／雷蛇及启动前回退选项与场景一致。普通仓库PrintWindow尝试无法捕获High产品，保留UNVERIFIED，不提权截图、不写成外部GUI图片对比通过 |
| 私有演示 | QA/output-demo-01的Qt6维护EXE SHA256 `5e24ea1932867e6a1c0f8178884a498fa6a7436fe847d14672b2aabb7a71d995`，PID38528／实际QkmDemoRecord126_0b38a0b04da44c019c58f7c5ef5fe22d，原生0／25项；正式MCP33条请求完成原两场景／回收，再显式开／捕获／关IbInput Popup。三张高级、C→D单项、IbInput图片均自主检查；sdk-pass的native_images=2指原两场景，第三张另有真实PNG及ibinput_popup_verified=true |
| 演示输出边界 | 10个原守卫场景分别记录IbSendInit、IbSendInput、IbSendDestroy、SendInput、SetCursorPos全零，包含A3像素／归一化、内部步进、释放和回退；新增Popup后退出总计仍逐API零。它是替换最终原生API的维护计数验证，普通正对照11项按138未变路径复用，不冒称普通Release厂商I/O实测 |
| 无打扰与清理 | 两普通产品分别61／17组窗口动作前后只读采样均Default且前台PID不是自有产品；允许用户自行换应用。这是离散采样加私有身份及动作边界，不声称全程焦点监控。无Default激活或系统输入调用；两轮FilterKeys前后相同，INI等于合成样例；演示工作／生产者及拥有资源清零、记录正常回收 |

### 本片失败及复用范围

popup-01／02在GUI配对与关闭管理窗native0后，缓存state.json原子发布实际WinError5导致控制器退出；已知自有PID29688／32476另经用户批准未缓存受限Close正常exit0。CACHE-139A单独395e684d修正与99项普通测试另有记录。新控制器42036至原19:00连续支持popup-03至07、read及demo后保持可用；没有记录到现场重试次数，不宣称已复现并恢复真实发布故障。历史未知请求未接管或重放。

popup-03／04的C++ UIA虚拟ListItem枚举为空、05的焦点／Home选择无效；06的DPI有界第一行点击已成功批准准确pair_id，但授权后仍停留通用页，找不到ibInputSettingsButton。固定夹具改为真实ReadMapping选页，07独立通过；这些是GUI夹具失败，产品均正常关闭。先前组件桌面包装／IPC环境缺失、Qt5 close解析和首轮静态诊断日志原样保留，不相加当通过。

本机原件：作业树`out/ibinput_popup_20261110`（component-*-08、static-analysis-01／02、product-popup-01至07、product-popup-read-01、popup-demo-01及图片／工具回归日志）与`out/ibinput_mcp_integration`构建日志；QA指原MCP树`out/mcp_product/ibinput139_20261110`（独立轮次、身份、部署、PNG、desktop-observations、SDK及原生结果）。不提交凭据／原始私有记录。

本片无新增Default输入／前台验收、Win10新运行、完整Qt5产品运行／x86 Popup、Worker／ASan或厂商输入重验、其他语言／DPI全矩阵、QQ gateway或三包Release。受影响窗口生命周期由双Qt组件与当前Qt6产品证明；138的业务／生命周期／设备／ASan证据按未变路径原范围复用。主动连接检查／试发工具、普通用户多实例前台绑定及PERSIST-137仍待。
