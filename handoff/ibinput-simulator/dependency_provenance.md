# IbInputSimulator 依赖来源与本机制品

记录日期：2026-10-29。本文保存来源、制品身份和证据边界；设计、进度及下一步只维护在 [主计划](qkeymapper_ibinputsimulator_integration_plan.md)。

## 证据边界

本机已有材料位于 plan/ibinput_simulator/，由用户复制到独立仓库。本次读取原 BUILD_INFO.md、查看接口／相关源码，并核对四份 DLL／LIB 的文件大小及 SHA256；未重建、加载或执行 DLL，未运行导出表／依赖扫描，也未验证厂商软件与驱动。

下表的固定源码版本和历史工具链来自原 BUILD_INFO.md，作为后续恢复与核对依据。现有二进制与记录的哈希一致，只证明文件身份一致；不独立证明它们确实由这些源码、参数和未修改的依赖生成。完整构建日志、准确 CMake 版本、补丁清单和可复现构建尚待 IB-01 补齐。

原 BUILD_INFO.md 保持不变，其中“完全精准复现”“确保开机与重启后开箱即用”“等价于 Win32 SendInput”等表述不作为本任务验收结论。上游接口和后端差异见主计划第 2、6 节。

## 固定源码

| 组件 | 固定提交 | 历史标签／用途 |
|---|---|---|
| [IbInputSimulator](https://github.com/Chaoses-Ib/IbInputSimulator/tree/0aba4f4f2b2053f106cee4c3be0a6201d76ca585) | 0aba4f4f2b2053f106cee4c3be0a6201d76ca585 | 包含 G HUB 初始化激活修改；不能据此宣称本机复连通过 |
| [IbWinCppLib](https://github.com/Chaoses-Ib/IbWinCppLib/tree/f630f0e3a2c757652298c626fcd52469e5d5bc81) | f630f0e3a2c757652298c626fcd52469e5d5bc81 | Windows C++ 辅助依赖 |
| [Microsoft Detours](https://github.com/microsoft/Detours/tree/1dbd4ea281d73d478aa8d09a8b83fbd265f715f7) | 1dbd4ea281d73d478aa8d09a8b83fbd265f715f7 | 历史记录标签 v4.0.1 |
| [RapidJSON](https://github.com/Tencent/rapidjson/tree/24b5e7a8b27f42fa16b96fc70aade9106cf7102f) | 24b5e7a8b27f42fa16b96fc70aade9106cf7102f | 历史记录标签 v1.1.0 |

使用完整提交号固定依赖，不使用浮动 master 或仅凭标签判断。每个依赖的许可证、再分发要求与最终使用方式在 IB-01 核对；本次未新增或分发依赖代码、头文件和二进制。

## 本机现有制品

来源机器：PC-A（2026-10-29 本次工作所在 PC）；以下路径均相对 plan/ibinput_simulator/。本次大小与哈希与原构建记录一致。路径中的架构是原记录标注，本轮未用 PE 工具重新确认。

| 文件 | 大小（字节） | SHA256 |
|---|---:|---|
| x64/IbInputSimulator.dll | 214528 | 02EA541A5464C3EE75CF922B25F171DD8A319B10DA9ADA6AFCE9B608B7D01014 |
| x64/IbInputSimulator.lib | 4448 | 5309E2DF6C178DAE5A105C23F2BD8D694CA97DB2082A4384AD573A692EE014C0 |
| x86/IbInputSimulator.dll | 167424 | 6E92C745DB4BF10889385004039673DC8BD5C9FE312E9362E18328BA562EA29B |
| x86/IbInputSimulator.lib | 4656 | 197FC68A76E35E36A9FB3C8728E1DAB9DFBF05BCF3943DFA63200DFB93FAA52D |

本地还存在 include/IbInputSimulator/InputSimulator.hpp、common.hpp 及 SendTypes/ 下的头文件。已查看公共接口和有关实现，尚未完成整个头文件目录与固定上游提交的逐文件一致性核对。仅有文件名或目录不代表已经完成产品接入。

原 BUILD_INFO.md 的本机 SHA256 为 B83C54600D5D52663F8B442AC3543B11B74435E728163DA0F5B098F5BCE49974，用于识别这次引用的历史记录；它与上述制品均未包含在本次 Git 推送中。

## 历史工具链与参数

以下均是原构建记录，不是本次执行结果：

| 项目 | 原记录 |
|---|---|
| 宿主系统 | Windows 11，build 26200 |
| 编译器 | MSVC 19.44.35228.0，Visual Studio 2022 17.14.38 BuildTools |
| SDK | Windows SDK 10.0.19041.0 |
| 生成器 | CMake 3.x，NMake Makefiles；准确 CMake 版本待补 |
| 配置 | Release，BUILD_SHARED_LIBS=ON，静态 C/C++ 运行时 /MT |
| 语言标准 | IbInputSimulator C++20；不自动改变 QKeyMapper 全局语言标准 |
| 架构 | 分别在 x64／x86 开发环境构建 Detours 和 IbInputSimulator |

原示例还使用 DETOURS_INCLUDE_DIR、DETOURS_LIBRARY、RapidJSON_DIR 和 FETCHCONTENT_SOURCE_DIR_IBWINCPP 指向本地依赖。它未充分记录具体配置缓存、/MT 的传入方式、全部工具版本及本地补丁；IB-01 需按固定源码实际 CMake 定义核对这些变量和路径，不能把该历史示例直接称为已验证重建脚本。

IbInputSimulator.dll 是动态库，.lib 为其导入库；/MT 指 C/C++ 运行时的静态链接，不代表无需部署 DLL。隐式链接的缺失／错误架构 DLL 会在进程装载阶段阻止启动，GUI 无法补救。

## 另一台 PC 的恢复路径

本次文档推送不传输依赖制品。接收 PC 无需原 plan/ 目录即可阅读计划；恢复依赖属于用户恢复开发后的 IB-01 工作。

1. 阅读主计划及本机 AGENTS.md，确认分支和当前授权范围。保留已有源码与制品，不覆盖不明改动。
2. 在本机独立的依赖工作目录获取上表仓库；对每个仓库使用 git checkout --detach 加完整提交号，核对 git rev-parse HEAD。已有目录先检查工作区，不对有改动的目录强行切换。
3. 核对源码、许可证、公共 API／调用约定、Qt5／Qt6 消费头文件的编译要求和 DLL 最低系统依赖。尤其评审主计划中已发现的鼠标状态／拆包、键盘报告、错误传播与生命周期限制。
4. 若必须修正第三方实现，先形成最小可审阅补丁，记录基准、用途、源位置和验证；分别重建相配套的 x64／x86 DLL、导入库与头文件，不能继续把原制品标作已修复。
5. 在匹配架构的 MSVC 环境使用独立构建目录，记录准确 CMake／SDK／编译器版本、参数与原始日志。按实际构建系统选择生成器，不复用另一台 PC 的缓存。
6. 检查真实 PE 架构、导入／导出和 ABI，核对运行时／系统依赖、Qt 目标链接及正确 DLL 部署；用无厂商软件的独立运行目录验证普通应用启动。源码编译、装载成功、驱动连接和实际输入分别记录。
7. 若由用户显式提供已有制品，则比对表中身份并记录传输来源；一致只表示复用同一文件，仍需完成本机接续所需检查。重建制品可能产生不同哈希，应记录新的真实身份和验证，不要求未知条件下位级一致。
8. 更新本文的来源事实及主计划中的阶段／能力／验证记录。只提交获授权、经许可核对的必要依赖内容；完整日志、构建缓存和个人配置保留在本机忽略的 out/。

本轮到此为文档交付，不执行上述恢复、重建、软件安装或驱动操作。
