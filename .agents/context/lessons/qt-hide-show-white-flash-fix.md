---
name: qt-hide-show-white-flash-fix
description: Qt Widgets 深色主题下 hide()→show() 恢复显示时白色闪烁的根因与双重修复模式
metadata:
  type: pattern
  category: qt_windows_rendering
  confidence: 0.95
  applications: 1
  created: 2026-07-24
---

# Qt Dark Theme: hide()→show() 白色闪烁修复模式

## 问题

Qt Widgets 程序使用深色主题（`QPalette::Window` = 深色），窗口从 `hide()` 状态恢复 `show()` 时，会先闪一下白色背景，然后才显示深色内容。浅色主题下不明显（白色接近浅色内容），但深色主题下非常刺眼。

## 根因

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

## 修复模式：双重保障

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

## 为什么这是最小方案

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
