---
name: 2026-09-21-qt5-ini-unicode-escape-shared-cache
description: Qt5 INI 配置文件触发 Unicode 转义(\xXXXX)的深度排障记录：QConfFile 共享缓存与 iniCodec 构造期时差竞争机制及 loadKeyMapSetting 副作用写盘根治
metadata:
  type: episodic
  date: 2026-09-21
  skill: qkeymapper-workflow
  files:
    - QKeyMapper/qkeymapper.cpp
    - QKeyMapper/qkeymapper_qt_compat.h
---

# Qt5 INI 配置文件 Unicode 转义与共享缓存时差竞争排障记录

## 一、故障现象

在 `Qt5_x64`（Qt 5.15.2 MSVC 2019 64-bit）构建环境下：
1. 点击“保存设定”按钮，`keymapdata.ini` 正常以 UTF-8（中文显示正常）保存；
2. 但随后触发**前台窗口映射匹配**或 **2000ms 超时切回“全局按键映射”** 时，整份 INI 文件（特别是 `[General]` 下所有包含非 ASCII 的项，如 `SettingSelect`、`TrayIcon_Matched` 和 `SettingSelect_OrderList`）被改写为 Unicode 转义形式（`\x5339\x914d...`）。
3. 尽管在代码库所有可见的 `QSettings` 声明行都添加了 `QKeyMapperQtCompat::setIniCodecUtf8`，该转义现象依然顽固发生。

---

## 二、微观沙箱诊断过程（Level 2 工作流实战）

按照 Level 2 独立沙箱诊断工作流，在控制台微型沙箱环境下进行了单步追踪与 A/B 对照：

### 1. 微观单步插桩（`test_micro.cpp`）
在语句间逐行检测物理文件是否出现 `\xXXXX` 转义：
```text
After Save: escaped=0
Creating outer settingFile...
After outer ctor: escaped=0
After outer setIniCodecUtf8: escaped=0
Creating nested1...
After nested1 ctor: escaped=0
After nested1 dtor: escaped=0
Calling settingFile.setValue(SettingSelect)...
After settingFile.setValue: escaped=0
Creating nested2...
After nested2 ctor: escaped=1                <--- 铁证！在 nested2 构造函数退出瞬间即已发生转义！
After nested2 setIniCodecUtf8: escaped=1      <--- 此时才执行 setIniCodecUtf8，为时已晚！
```

### 2. 根因深度剖析
1. **Qt 5 内部架构缺陷（共享缓存 vs 私有 Codec）**：
   - 同路径的 INI 文件共享全局单例缓存 `QConfFile`（保存在静态哈希表 `usedHashFunc()` 中），记录未提交修改 `addedKeys`；
   - 但编码指针 `iniCodec` 保存在每个 `QSettings` 实例私有的 `QConfFileSettingsPrivate` 中，**构造时默认初始化为 `nullptr`**。
2. **致命的构造期冲盘时差窗口（Race Window）**：
   - 标准声明写法分为两行：
     ```cpp
     QSettings nested(CONFIG_FILENAME, QSettings::IniFormat); // 行 A: 构造
     QKeyMapperQtCompat::setIniCodecUtf8(nested);            // 行 B: 赋编码
     ```
   - 若此时全局共享缓存中有**未写盘的修改**（`!confFile->addedKeys.isEmpty()`），**行 A 构造函数在执行期间（`initAccess() -> sync() -> syncConfFile()`）就会立即触发写盘 `writeIniFile()`**！
   - 此时代码尚未执行到行 B，`iniCodec` 依然为 `nullptr`，底层 `iniEscapedString()` 强制把所有非 ASCII 字符转为 `\xXXXX` 写盘！
3. **引爆炸弹的元凶（`loadKeyMapSetting` 副作用写盘）**：
   - 映射匹配与超时切回调用的 `loadKeyMapSetting`（约行 23551）原本带有一句隐式写操作：
     ```cpp
     if (settingSelectStr != settingtext) {
         settingFile.setValue(SETTINGSELECT , settingtext);
         settingSelectStr = settingtext;
     }
     ```
   - 它把修改塞入共享缓存后长久悬挂未写盘（未调 `sync()`）；随后流程中任何局部临时 `QSettings` 实例一构造，就会瞬间踩进时差窗口，引爆全盘转义。
4. **关于 UTF-8 BOM 盾牌的实测澄清**：
   - 实测证实：Qt 5 的 `writeIniFile()` 在写盘时采用 `Truncate` 覆写，**不会写回 BOM**。因此一旦程序正常写盘，原先文件头的 BOM 就会被彻底抹除，无法作为持久防御手段。

---

## 三、根除方案与实施效果

### 1. 核心修改
在 `QKeyMapper/qkeymapper.cpp` 的 `loadKeyMapSetting` 中彻底剥离副作用写盘，遵循“加载操作无副作用”原则：
```diff
             if (settingSelectStr != settingtext) {
-                settingFile.setValue(SETTINGSELECT , settingtext);
+                // Do not call settingFile.setValue(SETTINGSELECT, settingtext) here.
+                // loadKeyMapSetting is a read-only configuration loader.
+                // Writing during read operations leaves uncommitted dirty keys in Qt's
+                // shared QConfFile cache, triggering Unicode escape corruption (\xXXXX)
+                // on Qt5 when subsequent local QSettings instances are constructed.
+                // Disk persistence is strictly handled by saveKeyMapSetting().
                 settingSelectStr = settingtext;
             }
```

### 2. 端到端 A/B 测试验证（`test_e2e.cpp`）
- **场景 A（修改前原有逻辑）**：`Escaped = YES (100% 稳定复现转义)`
- **场景 B（修改后优化逻辑）**：`Escaped = NO (100% 根除转义，稳定保持原生 UTF-8)`

### 3. 伴生收益
- 彻底解决 Qt 5 下 INI 被转义为 `\xXXXX` 的顽疾；
- 杜绝前台窗口焦点频繁切换和超时切回时的无谓磁盘 I/O，大幅延长 SSD 寿命；
- 消除外部编辑器（Notepad++、VS Code）由于文件高频被写而频繁弹出的重新载入骚扰弹窗；
- 对 Qt 5 与 Qt 6 双向透明、完全正向兼容。
