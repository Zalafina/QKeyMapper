---
name: qt-hide-show-white-flash-fix
description: Qt Widgets 深色主题下 hide()→show() 白色闪烁的恢复期首帧保护模式
metadata:
  type: pattern
  category: qt_windows_rendering
  confidence: 0.95
  applications: 1
  created: 2026-07-24
  verified: 2026-07-29
---

# Qt Dark Theme: hide()→show() 白色闪烁修复模式

<!-- Correction: 2026-07-29 | was: global WM_ERASEBKGND + WA_OpaquePaintEvent + custom paintEvent | reason: title-bar dragging flickered -->

## 问题

Qt Widgets 程序使用深色主题（`QPalette::Window` = 深色），窗口从 `hide()` 状态恢复 `show()` 时，会先闪一下白色背景，然后才显示深色内容。浅色主题下不明显（白色接近浅色内容），但深色主题下非常刺眼。

## 已验证修复模式：恢复期首帧保护

仅在窗口原本 `isHidden()` 且即将恢复时启用一个默认 `false` 的私有状态，例如 `m_TrayRestoreBackgroundFillActive`：

1. 必须在任何 `ShowWindow()` 或带 `SWP_SHOWWINDOW` 的 `SetWindowPos()` 前启用该状态。
2. `nativeEvent()` 仅对主窗口自身、且该状态有效时的 `WM_ERASEBKGND` 使用 `msg->wParam` HDC、`GetClientRect()` 与 `palette().color(QPalette::Window)` 填色；仅在 `FillRect()` 成功时返回已处理，并立即释放临时 brush。
3. `event()` 先让 `QMainWindow::event()` 处理首个 `QEvent::UpdateRequest`，再关闭该状态；收到 `QEvent::Hide` 时也关闭，避免状态残留。
4. 其余时间全部使用 Qt 的默认 backing-store 绘制路径，保持原有 `SWP_SHOWWINDOW`、显示、置顶与激活逻辑。

2026-07-29 Windows 实机验证确认：主题色首帧保护消除了深色主题白闪，标题栏拖动不再抽搐闪烁。

## 禁止回归

- 不要全生命周期拦截每一次 `WM_ERASEBKGND`。
- 不要恢复 `WA_OpaquePaintEvent` 或主窗口自定义 `paintEvent()`。
- 不要把移除 `SWP_SHOWWINDOW` 记录为白闪修复手段；该尝试未解决问题，且可能延长中间帧。
- 不要用 opacity、定时器、`processEvents()` 或改变 `hide()`/`show()` 语义来替代上述有限状态保护。

## 历史根因假设（已弃用）

Windows 原生窗口层面：
1. `QWidget::hide()` → 销毁 HWND 的 backing store
2. `QWidget::show()` → Windows 重新创建 backing store → 立即发送 `WM_ERASEBKGND`
3. 窗口类默认 `HBRBACKGROUND` = `COLOR_WINDOW`（白色）
4. `DefWindowProc` 用白色画刷填充客户区
5. Qt 的 `QPalette` 感知 paintEvent 之后才执行，用深色覆盖
6. 时间差 = 白色闪烁

## 适用条件

- Qt Widgets (QMainWindow / QWidget / QDialog)
- Windows 平台
- 使用 `hide()` / `show()` 切换可见性（系统托盘恢复场景最常见）
- 使用深色 QPalette 或深色 stylesheet

## 历史双重绘制方案（已弃用，禁止采用）

### 第一层：nativeEvent 拦截 WM_ERASEBKGND（主力修复）

在 `nativeEvent()` 中添加：

```cpp
else if (msg->message == WM_ERASEBKGND) {
    HDC hdc = reinterpret_cast<HDC>(msg->wParam);
    if (hdc) {
        QColor bgColor = palette().color(QPalette::Window);
        HBRUSH brush = CreateSolidBrush(RGB(bgColor.red(), bgColor.green(), bgColor.blue()));
        if (brush) {
            RECT rect;
            GetClipBox(hdc, &rect);
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);
        }
    }
    *result = TRUE;   // 告诉 Windows 背景已擦除
    return true;      // 阻止默认白色填充
}
```

**注意：** 必须用 `msg->wParam` 中的 HDC，不要用 `GetDC()` 或 `GetWindowDC()` —— `WM_ERASEBKGND` 传入的 DC 是正确的。

### 第二层：paintEvent + WA_OpaquePaintEvent（安全网）

在构造函数中（`createWinId()` 之后）：

```cpp
setAttribute(Qt::WA_OpaquePaintEvent, true);
```

重写 `paintEvent`：

```cpp
void MyWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Window));
}
```

**注意：** 不要调用 `QMainWindow::paintEvent(event)` —— `WA_OpaquePaintEvent` 表示我们完全接管背景绘制。Qt 仍会自动绘制所有子控件。

## 为什么旧方案不再是最小方案

- 不需要 `WA_TranslucentBackground`（会强制无边框窗口）
- 不需要 `Qt::FramelessWindowHint`
- 不需要修改 `hide()`/`show()` 的行为语义
- 不需要定时器、延迟、或 opacity hack
- 不需要改动子控件或布局

## 为什么不用其他方案

| 方案 | 为什么拒绝 |
|------|-----------|
| Opacity fade-in | 增加 200ms 延迟，改变"立即弹出"的交互感 |
| 用 `showMinimized` 替代 `hide` | 改变窗口状态管理逻辑，多个调用点需修改 |
| `SetClassLongPtr(GCLP_HBRBACKGROUND)` | 影响同窗口类所有实例，可能破坏其他 Qt 窗口 |
| `WA_TranslucentBackground` | 要求 `FramelessWindowHint`，不适用于标准 QMainWindow |

## 相关模式

- 本项目浮动控件使用 `WA_TranslucentBackground` + `WA_NoSystemBackground` 实现透明无边框覆盖层 → 见 [[floatingbutton-tooltip-manual-qttooltip]]
- 本项目弹窗使用 opacity fade-in 实现无闪烁淡入 → 见 `qkeymapper.cpp` 的 `QPopupNotification` 动画
