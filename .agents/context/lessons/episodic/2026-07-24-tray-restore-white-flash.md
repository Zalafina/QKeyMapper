---
name: 2026-07-24-tray-restore-white-flash
description: 深色主题下从系统托盘恢复显示时白色闪烁的根因分析与双重修复
metadata:
  type: episodic
  date: 2026-07-24
  skill: qkeymapper-workflow
  files:
    - QKeyMapper/qkeymapper.h
    - QKeyMapper/qkeymapper.cpp
---

# 深色主题托盘恢复白色闪烁修复

## 问题

深色主题下，从系统托盘恢复主窗口（双击托盘图标或快捷键）时，先看到白色窗口背景闪一下，然后深色内容才显示。浅色主题下不明显。最小化→恢复没有此问题。

## 根因分析

`hide()` 调用会销毁 Win32 window backing store。再次 `show()`/`showNormal()` 时：
1. Windows 重新创建 backing store → 立即发送 `WM_ERASEBKGND`
2. 窗口类默认背景画刷是 `COLOR_WINDOW`（白色）
3. `WM_ERASEBKGND` 用白色画刷填充客户区
4. Qt 的 `QPalette::Window` 感知绘制之后才运行，覆盖为深色
5. 这个时间差导致白→深的闪烁

最小化（`showMinimized()`）不销毁 backing store，所以没有此问题。

## 候选方案评估

| 方案 | 原理 | 有效性 | 风险 | 采纳 |
|------|------|--------|------|------|
| A: nativeEvent WM_ERASEBKGND 拦截 | GDI FillRect 用调色板颜色替代默认白色画刷 | ⭐⭐⭐⭐⭐ | 低 | ✅ |
| B: paintEvent + WA_OpaquePaintEvent | Qt 层填充背景 + 跳过默认背景绘制 | ⭐⭐⭐⭐ | 低 | ✅ |
| C: 淡入动画 | setWindowOpacity(0.0)→show()→动画→1.0 | ⭐⭐⭐⭐ | 最低 | ❌ (改变交互节奏) |
| D: A+B 组合 | 双重保障 | ⭐⭐⭐⭐⭐ | 低 | ✅ 采纳 |
| E: 不完整隐藏 | 用 SW_HIDE 替代 hide() 保留 backing store | ⭐⭐⭐⭐ | 高 | ❌ (太侵入) |

## 实施的修改

### qkeymapper.h (1 处)
- L1524: `paintEvent(QPaintEvent*)` 声明，插入 `showEvent` 与 `closeEvent` 之间

### qkeymapper.cpp (3 处)
1. **构造函数 L2981-2985**: `createWinId()` 后设 `setAttribute(Qt::WA_OpaquePaintEvent, true)`
2. **L14781-14791**: 新 `paintEvent` 重写 —— 用 `palette().color(QPalette::Window)` 填充 `rect()`
3. **L14709-14727**: `nativeEvent` 中拦截 `WM_ERASEBKGND` —— `CreateSolidBrush` + `GetClipBox` + `FillRect` + `DeleteObject`，`*result = TRUE; return true;`

总共 ~35 行新代码，2 个文件。

## 关键经验

1. **Qt hide() vs minimize() 的区别不仅在于可见性** —— hide() 销毁 backing store，minimize() 保留。这影响恢复时的绘制行为。
2. **`WM_ERASEBKGND` 是 Windows 原生窗口白色闪烁的根因** —— Qt 的 palette 系统在它之后运行，无法阻止第一次白色填充。
3. **nativeEvent 已有基础设施** —— 本项目 nativeEvent 已处理 `WM_WTSSESSION_CHANGE` 和 `WM_SETTINGCHANGE`，添加 `WM_ERASEBKGND` 风格一致。
4. **WA_OpaquePaintEvent + paintEvent 作为安全网** —— 即使 nativeEvent 在某些边缘情况未触发，paintEvent 保证 Qt 层背景正确。
5. **不改变 hide/show 语义** —— 保持 `hide()` 和 `DISPLAYSWITCHMODE` 逻辑不变，只在渲染层面修复。

## 待验证

- [ ] 编译通过
- [ ] 深色主题托盘恢复无白色闪烁
- [ ] 浅色/系统默认主题背景颜色正确
- [ ] 最小化恢复路径无回归
- [ ] 窗口缩放无白色边缘
- [ ] 子控件绘制正常
- [ ] 隐藏期间切换主题后恢复颜色匹配

## 关联记忆

- [[qt-hide-show-white-flash-fix]] — 可复用的通用模式
