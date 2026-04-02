#include "qkeymapper.h"
#include "colorpickerwidget.h"
#include <QMenu>

using namespace QKeyMapperConstants;

ColorPickerWidget::ColorPickerWidget(QWidget *parent, QString buttonText, int buttonWidth)
    : QWidget(parent)
    , m_buttonText(buttonText)
{
    colorLabel = new QLabel(this);
    colorButton = new QPushButton(buttonText, this);

    colorButton->setFocusPolicy(Qt::NoFocus);
    colorButton->setAutoDefault(false);

    // Set color label style
    colorLabel->setFrameStyle(QFrame::WinPanel | QFrame::Sunken);

    // Set default size for colorLabel and colorButton
    colorLabel->setFixedSize(31, 21);  // Set fixed size for the label
    colorButton->setFixedSize(buttonWidth, 21); // Set fixed size for the button

    // Setup layout and add the button and label to it
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(colorLabel);
    layout->addWidget(colorButton);

    // Connect the button click signal to the slot
    connect(colorButton, &QPushButton::clicked, this, &ColorPickerWidget::onPickColor);

    // Right-click context menu for restoring default color
    colorButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(colorButton, &QPushButton::customContextMenuRequested,
            this, &ColorPickerWidget::onColorButtonContextMenu);
}

ColorPickerWidget::~ColorPickerWidget()
{
    // Automatic cleanup of dynamically allocated objects
}

QColor ColorPickerWidget::getColor()
{
    return m_color;
}

#if 0
void ColorPickerWidget::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    QString translated_ButtonText;
    if (m_buttonText == "TabFontColor") {
        translated_ButtonText = tr("TabFontColor");
    }
    else { /* m_buttonText == "Color" */
        translated_ButtonText = tr("Color");
    }
    colorButton->setText(translated_ButtonText);
}
#endif

void ColorPickerWidget::setColor(const QColor &color)
{
    if (color.isValid()) {
        m_color = color;
        // If the selected color is valid, update the label with the color name
        // and change the label's background color to the selected color
        QPalette palette = colorLabel->palette();
        palette.setColor(colorLabel->backgroundRole(), color);
        colorLabel->setAutoFillBackground(true);
        colorLabel->setPalette(palette);
    }
}

void ColorPickerWidget::setShowAlphaChannel(bool show)
{
    m_showAlphaChannel = show;
}

void ColorPickerWidget::setWindowTitle(QString title)
{
    m_windowTitle = title;
}

void ColorPickerWidget::setButtonText(QString text)
{
    if (!text.isEmpty()) {
        colorButton->setText(text);
    }
}

void ColorPickerWidget::onPickColor()
{
    QColor defaultColor = CROSSHAIR_CROSSHAIRCOLOR_DEFAULT_QCOLOR;
    if (m_color.isValid()) {
        defaultColor = m_color;
    }

    // Open the color picker dialog and allow the user to choose a color
    QString title;
    QColorDialog::ColorDialogOptions options;
    if (m_windowTitle.isEmpty()) {
        title = tr("Select Color");
    }
    else {
        title = m_windowTitle;
    }
    if (m_showAlphaChannel) {
        options |= QColorDialog::ShowAlphaChannel;
    }

    // Save the current focus widget
    QWidget *currentFocusWidget = QApplication::focusWidget();
    QWidget *parentWindow = this->parentWidget();
    while (parentWindow && !parentWindow->isWindow()) {
        parentWindow = parentWindow->parentWidget();
    }

    QColorDialog *selectcolor_dialog = QKeyMapper::getInstance()->m_SelectColorDialog;
    selectcolor_dialog->setCurrentColor(defaultColor);
    selectcolor_dialog->setOptions(options);
    selectcolor_dialog->setWindowTitle(title);

    bool accepted = false;
    if (selectcolor_dialog->exec() == QDialog::Accepted) {
        QColor color = selectcolor_dialog->selectedColor();
        accepted = true;
        Q_UNUSED(accepted);

        // Restore focus to the original window
        if (parentWindow) {
            parentWindow->raise();
            parentWindow->activateWindow();
        }
        if (currentFocusWidget) {
            currentFocusWidget->setFocus();
        }

        if (color.isValid()) {
            m_color = color;
        } else {
            color = m_color;
        }

        QPalette palette = colorLabel->palette();
        palette.setColor(colorLabel->backgroundRole(), color);
        colorLabel->setAutoFillBackground(true);
        colorLabel->setPalette(palette);

        emit colorChanged(color);
        return;
    }

    // Restore focus to the original window even if cancelled
    if (parentWindow) {
        parentWindow->raise();
        parentWindow->activateWindow();
    }
    if (currentFocusWidget) {
        currentFocusWidget->setFocus();
    }
}

void ColorPickerWidget::onColorButtonContextMenu(const QPoint &pos)
{
    // Only show context menu for button texts that have a known default color
    if ("TabFontColor" != m_buttonText
        && "TabBGColor"    != m_buttonText
        && "FW_BGColor"    != m_buttonText
        && "FontColor"     != m_buttonText
        && "BGColor"       != m_buttonText
        && "CenterColor"   != m_buttonText
        && "CrosshairColor" != m_buttonText
        && "VBtn_BGColor"   != m_buttonText
        && "VBtn_BtnColor"  != m_buttonText
        && "VBtn_PressedColor" != m_buttonText
        && "VBtn_LockedColor" != m_buttonText
        && "VBtn_TextColor" != m_buttonText
        && "FloatBtn_BtnColor" != m_buttonText
        && "FloatBtn_PressedColor" != m_buttonText
        && "FloatBtn_LockedColor" != m_buttonText
        && "FloatBtn_TextColor" != m_buttonText) {
        return;
    }

    QMenu menu(this);
    QAction *restoreAction = menu.addAction(tr("Restore Default Color"));
    QAction *selected = menu.exec(colorButton->mapToGlobal(pos));
    if (selected != restoreAction) {
        return;
    }

    // Determine the default color for this button type
    QColor color;
    if ("TabFontColor" == m_buttonText) {
        int setting_select_index = QTableSetupDialog::getInstance()->getSettingSelectIndex();
        if (setting_select_index < 0) {
            return;
        }
        else if (GLOBALSETTING_INDEX == setting_select_index) {
            color = NOTIFICATION_COLOR_GLOBAL_DEFAULT;
        }
        else {
            color = NOTIFICATION_COLOR_NORMAL_DEFAULT;
        }
    }
    else if ("TabBGColor" == m_buttonText) {
        color = NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
    }
    else if ("FW_BGColor" == m_buttonText) {
        color = FLOATINGWINDOW_BACKGROUND_COLOR_DEFAULT;
    }
    else if ("FontColor" == m_buttonText) {
        color = NOTIFICATION_COLOR_NORMAL_DEFAULT;
    }
    else if ("BGColor" == m_buttonText) {
        color = NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
    }
    else if ("CenterColor" == m_buttonText) {
        color = CROSSHAIR_CENTERCOLOR_DEFAULT_QCOLOR;
    }
    else if ("CrosshairColor" == m_buttonText) {
        color = CROSSHAIR_CROSSHAIRCOLOR_DEFAULT_QCOLOR;
    }
    else if ("VBtn_BGColor" == m_buttonText) {
        color = VBTNPANEL_BACKGROUND_COLOR_DEFAULT;
    }
    else if ("VBtn_BtnColor" == m_buttonText) {
        color = VBTNPANEL_BUTTON_COLOR_DEFAULT;
    }
    else if ("VBtn_PressedColor" == m_buttonText) {
        color = VBTNPANEL_PRESSED_COLOR_DEFAULT;
    }
    else if ("VBtn_LockedColor" == m_buttonText) {
        color = VBTNPANEL_LOCKED_COLOR_DEFAULT;
    }
    else if ("VBtn_TextColor" == m_buttonText) {
        color = VBTNPANEL_TEXT_COLOR_DEFAULT;
    }
    else if ("FloatBtn_BtnColor" == m_buttonText) {
        color = FLOATINGBUTTON_BUTTON_COLOR_DEFAULT_QCOLOR;
    }
    else if ("FloatBtn_PressedColor" == m_buttonText) {
        color = FLOATINGBUTTON_PRESSED_COLOR_DEFAULT_QCOLOR;
    }
    else if ("FloatBtn_LockedColor" == m_buttonText) {
        color = FLOATINGBUTTON_LOCKED_COLOR_DEFAULT_QCOLOR;
    }
    else if ("FloatBtn_TextColor" == m_buttonText) {
        color = FLOATINGBUTTON_TEXT_COLOR_DEFAULT_QCOLOR;
    }

    // Update stored color and label
    if (color.isValid()) {
        m_color = color;
    }

    QPalette palette = colorLabel->palette();
    palette.setColor(colorLabel->backgroundRole(), color.isValid() ? color : m_color);
    colorLabel->setAutoFillBackground(true);
    colorLabel->setPalette(palette);

    // Emit signal — Tab/FW types use empty color to signal "use system default"
    if ("TabFontColor" == m_buttonText
        || "TabBGColor" == m_buttonText
        || "FW_BGColor" == m_buttonText) {
        QColor emptyColor;
        emit colorChanged(emptyColor);
    }
    else {
        emit colorChanged(color);
    }
}
