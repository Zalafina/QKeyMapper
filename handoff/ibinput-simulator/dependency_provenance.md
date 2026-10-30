# IbInputSimulator 依赖来源与本机制品

记录日期：2026-10-30。本文保存来源、制品身份和证据边界；设计、进度及下一步只维护在 [主计划](qkeymapper_ibinputsimulator_integration_plan.md)。

## DOC-00 历史证据边界

以下至“2026-10-29 PC-W10 当前恢复与配套重建”之前为 DOC-00 历史记录，不是当前工作区或当前重建结果；现行制品身份和恢复命令见末节。

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

来源机器：PC-A（DOC-00 记录机器）；以下路径均相对 plan/ibinput_simulator/。本次大小与哈希与原构建记录一致。路径中的架构是原记录标注，本轮未用 PE 工具重新确认。

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

上述停止边界属于 DOC-00；本次恢复及实际重建结果如下。

## 2026-10-29 PC-W10 恢复与配套重建（历史）

本节是本次真实执行；前文 PC-A 制品表继续作为历史身份记录，本机没有收到其原始制品。四个仓库按前述完整提交重新检出；固定源码及本机 out 不随 Git。

- 实际工具链：VS 2022 Community 17.14.23，MSVC 19.44.35222.0，Windows SDK 10.0.19041.0，CMake 3.28.6，NMake；依赖 Release、BUILD_SHARED_LIBS=ON、上游 /MT，产品保持 C++17。
- IbInputSimulator／IbWinCppLib／Detours 的原始 MIT 许可及 RapidJSON 的完整许可已保存至 QKeyMapper/IbInputSimulator/licenses。RapidJSON 仅消费 include 下的实际头文件，不使用或分发 bin/jsonchecker；完整声明保留其附加 BSD／JSON 条款，不将整份来源简单改写成单一许可。
- 用户已分别批准 0001 Logitech 报告安全与 0002 参数返回边界补丁；完整文本在 QKeyMapper/IbInputSimulator/patches，基准与用途见其 README，设计／批准／进度仍以唯一主计划为准。
- 原版与两补丁版的 x64／x86 DLL 和导入库均已重建；配套修正版部署到 QKeyMapper/IbInputSimulator/lib/x64、lib/x86，固定正式公共头文件在 include/IbInputSimulator。未采用手写 DLL 函数表或修改产品语言标准。
- dumpbin 已检查 DLL、导入库机器类型、导出／导入／依赖；公开 C 入口与调用约定保持原版形态。Qt6 C++17 客户端替身已通过；Qt5 消费及正式 ABI 链接的当前结果由主计划记录，不以 PE 检查冒充运行。
- 依赖原版存在 C4715，原始日志保留；修正版消除这两处返回路径警告，未屏蔽 warning。早期路径／安装配置失败保留在 out/ibinput/logs，不通过提权或改系统安装目录解决。
- 尚未证明 Win7 启动、新依赖与所有历史系统／厂商版本兼容，静态导入不能替代实机验证。厂商驱动连接与实际接收分别记录。

恢复命令（在仓库根目录，普通权限 Python 3.11；需要 VS C++ 工具、SDK 10.0.19041.0、CMake 和 Git）：

~~~powershell
python scripts/build_ibinput_dependency.py --arch x64 --review-patch --tests --deploy
python scripts/build_ibinput_dependency.py --arch x86 --review-patch --tests --deploy
~~~

脚本验证固定提交、保留不明改动，在忽略 out 下重建；--review-patch 在独立生成副本应用补丁，--deploy 显式更新产品依赖目录。依赖 SOURCE 路径可以恢复，不复制另一台 PC 的 CMake 缓存。构建和测试均不初始化驱动或发送真实输入。重建可能产生不同 PE 时间戳／哈希，不要求未知条件下位级一致。

当前纳入 Git 的制品身份：

| 文件 | 大小 | SHA256 |
|---|---:|---|
| lib/x64/IbInputSimulator.dll | 214528 | 3CB142F46AD056862A5C252E035876F74FA8CD98A9D8D7E80686962E2C2C91E7 |
| lib/x64/IbInputSimulator.lib | 4448 | 02D9BF0199958199CC16A80D66CF0FF6F825CDB40994289EBC44D4FAEEAA6C55 |
| lib/x86/IbInputSimulator.dll | 168960 | 3E3272FBB9BE70EB908FDE4E3D8A28796C414EDD8CE32ABC1895787DE45390AB |
| lib/x86/IbInputSimulator.lib | 4656 | D89E6A3BB16DF5D5DBD22601345566F78AA2E166F6637EA7C2FEEACEF308CE72 |


## 2026-10-30 PC-W11 当前配套制品

固定四项源码及0001／0002上叠加分别批准的0003／0004，公共头、导出ABI及IOCTL／报告布局不变。0003跨清理保留Windows错误；0004复用所选G HUB虚拟总线下的健康Mouse节点，不按实体型号限定。

实际VS2022 BuildTools、MSVC19.44.35228.0、SDK10.0.19041.0、CMake3.31.12／NMake，Release、BUILD_SHARED_LIBS=ON、上游/MT；两架构独立目录，产品仍C++17。CfgMgr32.dll为0004新增只读系统导入。PE、导入库、原版对照和六组报告／就绪替身、参数边界通过，修正版日志无编译警告。未验证Win7或全部历史系统，不从API最低版本推定产品通过。

当前Git制品身份（路径相对QKeyMapper/IbInputSimulator）：

| 文件 | 大小 | SHA256 |
|---|---:|---|
| lib/x64/IbInputSimulator.dll | 218112 | 13BDF841A5B15057A939BC1DC5C41FC1848FDE433DCC5B0941248D16FDB9D548 |
| lib/x64/IbInputSimulator.lib | 4448 | 5309E2DF6C178DAE5A105C23F2BD8D694CA97DB2082A4384AD573A692EE014C0 |
| lib/x86/IbInputSimulator.dll | 172032 | 384667923B87DDA042CFAEC9F488869AA3D4BF181B1BA25CCA8A007E52CD89EE |
| lib/x86/IbInputSimulator.lib | 4656 | 197FC68A76E35E36A9FB3C8728E1DAB9DFBF05BCF3943DFA63200DFB93FAA52D |

恢复命令仍为前述两架构--review-patch --tests --deploy；脚本恢复四个受补丁影响文件后按序应用0001～0004。原0003中间制品仅历史，不替代本表当前配套版本。日志为out/ibinput/logs/w11-dependency-*-0004-driver-free.log、logs/x64及x86/review-*、w11-cmake-version.log。Win11真实连接／关闭／一次复连和最小接收通过；Win10本版本待复验，产品能力限制及环境矩阵以主计划为准。
