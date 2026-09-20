---
name: qt-headless-sandbox-diagnostic-workflow
description: QKeyMapper 复杂底层问题（Qt内部机制/时序竞争/驱动交互）的独立无界面沙箱诊断工作流规范，含分级阶梯机制与用户授权门禁。
metadata:
  type: pattern
  skill: qkeymapper-workflow
---

# Qt/Windows 复杂底层问题独立沙箱诊断工作流

## 一、核心原则：分级阶梯与用户授权门禁

为了防止过度工程（Over-engineering）与重型工具滥用，严禁在遇到任何普通问题时都直奔该工作流。Agent 必须严格遵守**两级分阶机制**与**用户授权门禁**。

### 1. Level 1：默认常规轻量排查（覆盖 90% 以上日常场景）
- **范围**：日常业务逻辑、UI 控件样式与布局、普通参数传递、已知模块内部缺陷。
- **手段**：静态阅读源码、同模块模式对比、添加必要局部 `DEBUG_LOGOUT_ON` 日志、小步安全修改、交付用户执行常规工程构建验证。
- **铁律**：**严禁在未获用户授权的情况下，擅自搭建独立的重型编译沙箱。**

### 2. Level 2：高级独立沙箱深度诊断（仅在满足门槛且经用户授权时启用）
- **升级门槛（必须至少满足以下条件之一）**：
  1. **底层机制深水区**：涉及 Qt 核心源码层未暴露的内部机制（如 `QConfFile` 全局共享缓存）、Win32 底层消息/钩子循环、物理驱动层通信等难以通过普通工程日志宏观观测的场景；
  2. **时序与并发竞争**：涉及跨线程、定时器毫秒级时序、全局共享单例状态与局部实例生命周期交织的时差竞争（Race Condition）；
  3. **顽固性故障**：已在 Level 1 下进行了常规排查与分析，但修改后依然顽固重现、无法通过静态逻辑解释。
- **用户授权确认门禁（User Opt-in Gate）**：
  当 Agent 识别到满足上述门槛时，**必须暂停工具调用，主动向用户说明现状并明确请示**：
  > “当前问题涉及 Qt/Win32 底层未显式暴露的内部机制与时序竞争，常规静态分析与局部日志难以直接定性。**是否需要让 Agent 自主搭建完整诊断工作流（独立控制台沙箱 + 微观单步插桩 + 跨版本工具链）来隔离复现并精准定位复杂问题根因？**”
- **执行准则**：**只有在用户明确回复同意/批准后，Agent 才能拉起 Level 2 独立沙箱！**

---

## 二、Windows + Qt 独立沙箱工具链直通车

在 Windows 环境下，直接通过 PowerShell 拼接复杂带括号和空格的命令行容易引发引号解析错误，且直接调用可能因缺失 Qt 运行环境 DLL 而导致静默退出（Exit Code 1）。
**铁律标准**：统一编写轻量 `.bat` 批处理脚本，通过 `cmd.exe /c run_xxx.bat` 运行。

### 1. 固定环境路径速查表
- **MSVC 2019 x64 构建初始化脚本**：
  `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat`
- **Qt 5.15.2 MSVC 2019 64-bit 安装根目录**：
  `C:\Qt\Qt6\5.15.2\msvc2019_64`
  - 包含目录：`include`、`include\QtCore`
  - 库目录：`lib\Qt5Core.lib`
  - 二进制目录：`bin`
- **Qt 6.8.3 MSVC 2022 64-bit 安装根目录**：
  `C:\Qt\Qt6\6.8.3\msvc2022_64`

### 2. 极速编译运行批处理模板（`run_test.bat`）
在临时工作目录（如 `<artifacts>/scratch/`）下创建该批处理，2 秒内即可完成编译并运行：
```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl.exe /EHsc /std:c++17 /I C:\Qt\Qt6\5.15.2\msvc2019_64\include /I C:\Qt\Qt6\5.15.2\msvc2019_64\include\QtCore %1.cpp /link /LIBPATH:C:\Qt\Qt6\5.15.2\msvc2019_64\lib Qt5Core.lib
set PATH=C:\Qt\Qt6\5.15.2\msvc2019_64\bin;%PATH%
%1.exe
```

---

## 三、微观单步插桩与断点探测技术

### 1. 核心思想
QKeyMapper 主窗口近 5 万行代码，流程极其冗长。宏观测试只能看到“最终结果异常”，却无法知道究竟是哪一行在哪个毫秒触发了底层破坏。
将怀疑的可疑调用链剥离到单文件控制台应用中，在**每一个细粒度语句执行的前后**插入物理状态检测探针。

### 2. 单步探针范式代码
```cpp
bool checkCorrupted(const QString &filePath) {
    QFile f(filePath);
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray content = f.readAll();
        return content.contains("\\x5339"); // 探测是否存在非预期的底层转义或损坏
    }
    return false;
}

// 在语句序列中穿插物理探针：
std::cout << "Step 0 (Initial): " << checkCorrupted(iniPath) << std::endl;

settingFile.setValue("SomeKey", "SomeValue");
std::cout << "Step 1 (After setValue): " << checkCorrupted(iniPath) << std::endl;

{
    std::cout << "Creating nested instance..." << std::endl;
    QSettings nested(iniPath, QSettings::IniFormat);
    std::cout << "Step 2 (Inside nested ctor): " << checkCorrupted(iniPath) << std::endl;
    // 如果在 ctor 退出瞬间 checkCorrupted 变为 true，直接将罪魁祸首精确锁定在构造函数内部冲盘！
}
```

---

## 四、端到端 A/B 对照验证法

在正式修改主工程业务源码之前，必须在沙箱内构建并列的 Scenario A 与 Scenario B，实现零风险闭环验证：

```cpp
// Scenario A: 复现原始故障逻辑（验证假说与复现率）
void testBuggyScenario(const QString &testPath) {
    // 运行带有潜在 Bug 的原流程
    // 预期：100% 稳定复现故障现象（Escaped = YES）
}

// Scenario B: 验证拟定根除方案（验证有效性与零回归）
void testFixedScenario(const QString &testPath) {
    // 运行去除有害副作用/修复时序后的新流程
    // 预期：100% 稳定消除故障（Escaped = NO）
}
```

---

## 五、沙箱诊断完毕后的清理规范

1. **环境与产物隔离**：测试代码与构建产物必须统一存放在 `<artifacts>/scratch/` 或专用临时目录中，严禁污染主仓库源码树；
2. **清理临时文件**：诊断完成后，及时清理 `.obj`、`.exe` 等大型中间二进制文件，保留关键的 `.cpp` 测试源码供案例引用；
3. **沉淀经验至知识库**：将排障过程中发现的深层机制陷阱提炼后记录至 `.agents/context/lessons/`。
