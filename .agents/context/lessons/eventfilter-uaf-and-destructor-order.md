# Qt eventFilter Use-After-Free 规避与析构顺序规范

## 1. 现象与典型崩溃场景
在顶层窗口（如 `QMainWindow` / `QKeyMapper`）关闭退出流程中，Qt Creator / Debugger 报告异常终止（`Access Violation 0xC0000005`），崩溃发生在 `Qt6Widgets!QSplitter::handle` 或类似内部子控件方法中。
使用 CDB / WinDbg 加载 Dump 文件时，异常寄存器指向 `0xdddddddddddddde5`（MSVC Debug CRT 堆内存释放后的 `0xDD` 填充标记），指令为 `mov rax, [rax]`（解引用已释放对象的成员变量或 `d_ptr`）。

## 2. 根因机制剖析
### 2.1 过滤器无条件动态求值
在重写 `eventFilter(QObject *object, QEvent *event)` 时，若直接在函数入口检查：
```cpp
// 危险写法：对进入过滤器的每一个事件都会无条件执行 handle(1) 动态求值
if (ui && ui->mainTableSplitter && object == ui->mainTableSplitter->handle(1)) {
    if (event->type() == QEvent::MouseButtonDblClick) { ... }
}
```
进入此过滤器的所有事件（焦点、悬停、绘制、失焦、窗口隐藏等成千上万个事件）都会频繁解引用 `ui->mainTableSplitter`。

### 2.2 析构时序与悬空指针（Dangling Pointer）
在主窗口析构函数 `~MainWindow()` 中：
1. 若过早执行 `delete ui;`，且未将 `ui` 置为 `nullptr`（悬空指针）；
2. 随后清理其他独立顶层窗口/悬浮窗（如 `m_FloatingIconWindow->hideFloatingWindow(); delete m_FloatingIconWindow;`）时，Windows 原生窗口隐藏与销毁会触发激活态与焦点转移（`QEvent::WindowDeactivate`, `QEvent::ActivationChange`, `QEvent::Hide` 等）；
3. 事件分发系统再次调用 `this->eventFilter(object, event)`；
4. 过滤器入口的 `if (ui && ...)` 检测通过（因为 `ui` 地址非零，指向 `0xDDDD...`），进而调用 `ui->mainTableSplitter->handle(1)`，立即引发非法内存访问崩溃。

## 3. 防护标准模式

### 3.1 规则 1：事件类型优先短路（Event Type First）
在 `eventFilter` 中拦截特定事件时，**必须将事件类型判断作为最外层第一条件**：
```cpp
// 安全高效：99.999% 的非目标事件在第一条分支即安全短路，零多余解引用
if (event && event->type() == QEvent::MouseButtonDblClick) {
    if (m_mainTableSplitterHandle && object == m_mainTableSplitterHandle.data()) {
        ...
    }
}
```

### 3.2 规则 2：使用 `QPointer` 弱引用观察子控件
不要在事件循环中每次动态从上层容器检索子控件指针。应使用 Qt 的安全受保护弱引用指针：
```cpp
// 头文件
QPointer<QSplitterHandle> m_mainTableSplitterHandle = Q_NULLPTR;

// 初始化
if (QSplitterHandle *handle = ui->mainTableSplitter->handle(1)) {
    m_mainTableSplitterHandle = handle;
    handle->installEventFilter(this);
}
```
当 `QSplitterHandle` 随父级销毁时，`QPointer` 会被 Qt 自动重置为 `nullptr`，绝不会产生野指针。

### 3.3 规则 3：析构函数首部解除过滤器，尾部销毁 UI 并置空
析构函数中应遵循标准的清理顺序：
```cpp
MainWindow::~MainWindow()
{
    // 1. 最早阶段：立即卸载注册在外部及子控件上的事件过滤器
    if (qApp) {
        qApp->removeEventFilter(this);
    }
    if (m_mainTableSplitterHandle) {
        m_mainTableSplitterHandle->removeEventFilter(this);
        m_mainTableSplitterHandle = Q_NULLPTR;
    }

    // 2. 中间阶段：清理与销毁独立对话框、浮动面板、系统托盘等
    if (m_FloatingIconWindow) {
        delete m_FloatingIconWindow;
        m_FloatingIconWindow = Q_NULLPTR;
    }

    // 3. 最后阶段：销毁主 UI 树并立即显式置空
    delete ui;
    ui = Q_NULLPTR;
}
```
