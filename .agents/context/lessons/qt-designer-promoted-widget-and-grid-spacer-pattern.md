# Qt Promoted Widget 边距归零、高度标准化与多列 GroupBox 网格弹簧对齐规范

## 1. 适用场景与问题背景

在 QKeyMapper 的 Qt Widgets UI 现代化与标准化改造中：
1. **自定义复合控件（如 ColorPickerWidget）嵌入父布局**：
   自定义 QWidget 内部使用 `QHBoxLayout` 封装子控件（如预览色块与按钮）。若未显式调用 `layout->setContentsMargins(0, 0, 0, 0)`，Qt 在 Fusion / Windows 风格下会默认施加 9px 外边距，导致子控件被推向 `y=9`。当外部布局限制行高为 22~24px 时，子控件下部（约 7~8px）会被硬性截断。
2. **多列 GroupBox 并列展示时的跨组对齐**：
   在类似“十字准星设定”、“提示信息高级设定”等具有多个并列 GroupBox 的对话框中，若某些 GroupBox 底部配置了 `verticalSpacer`（垂直弹簧），而其他组遗漏了弹簧：
   - 有弹簧的组所有控件紧贴顶部（`y = topMargin`），行高紧凑；
   - 缺弹簧的组会将窗口多余垂直高度均分拉伸到各行中，导致行高被撑大、行距稀疏，控件（特别是 `AlignCenter` 对齐的色块和复选框）在更高的行中垂直居中，整体 Y 坐标下移数个像素，导致跨 GroupBox 视觉错位。

---

## 2. 核心架构与布局规范

### 2.1 自定义复合控件（Promoted Widget）规范
1. **内部布局边距彻底归零**：
   嵌入式复合控件自身内部布局必须显式清零外边距：
   ```cpp
   QHBoxLayout *layout = new QHBoxLayout(this);
   layout->setContentsMargins(0, 0, 0, 0);
   layout->setSpacing(6);
   ```
2. **高度标准化与自我尺寸锁定**：
   与 QKeyMapper 全局标准输入控件（LineEdit / SpinBox / ComboBox / 小型 PushButton）的 22px 基准高度保持严格一致：
   ```cpp
   const int btnHeight = qMax(22, colorButton->fontMetrics().height() + 4);
   colorLabel->setFixedSize(31, btnHeight);
   colorButton->setFixedHeight(btnHeight);
   setFixedHeight(btnHeight);  // 容器自我锁定高度，防止被外部父布局异常压缩或拉伸
   ```
3. **`.ui` 文件属性对齐**：
   在 `.ui` 文件中，Promoted 控件的 `minimumSize` 高度统一设置为 `<height>22</height>`，确保 Qt Creator 设计器画布与程序运行态 1:1 精确一致。

---

### 2.2 多列 GroupBox 并列网格弹簧规范
1. **并列 GroupBox 必须全员配备底部垂直弹簧**：
   每一个包含多行控件的并列 GroupBox 网格布局，最后一行必须放置一个 `QSpacerItem`（垂直方向，`sizeHint: (20, 0)`），跨越该网格所有列。
2. **对齐效果**：
   - 所有 GroupBox 的控件全部从 `y = topMargin` 紧凑排起；
   - 跨组对应的第 0 行、第 1 行、第 2 行等在同一水平 Y 坐标上实现像素级平齐；
   - 行间距严格受控于 `verticalSpacing`（标准为 6px），行数较少的组不会出现异常拉伸空隙；
   - 窗口伸展或多余的纵向空间全部由底部的弹簧整齐吸收。

---

## 3. 测试与临时文件存放规范

- 所有调试脚本、临时编译物、实验生成的草稿文件（如 `test_*.obj`、`*.tmp` 等）**严禁直接放置在仓库根目录**；
- 必须严格统一放置在根目录下的 [`scratch/`](file:///f:/work/code/mygit_hub/QKeyMapper/scratch/) 目录中；
- `scratch/` 目录已被加入 `.gitignore`，确保仓库根目录结构时刻保持干净整洁。
