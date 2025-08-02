#include "qkeymapper.h"
#include "colorpickerwidget.h"

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
    bool clear_to_default = false;
    if ("TabFontColor" == m_buttonText
        || "TabBGColor" == m_buttonText
        || "FontColor" == m_buttonText
        || "BGColor" == m_buttonText
        || "CenterColor" == m_buttonText
        || "CrosshairColor" == m_buttonText) {
        if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
            clear_to_default = true;
        }
    }

    QColor color;
    if (clear_to_default) {
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
    }
    else {
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

        // color = QKeyMapper::getInstance()->m_SelectColorDialog->getColor(defaultColor, this, title, options);
        QColorDialog *selectcolor_dialog = QKeyMapper::getInstance()->m_SelectColorDialog;
        selectcolor_dialog->setCurrentColor(defaultColor);
        selectcolor_dialog->setOptions(options);
        selectcolor_dialog->setWindowTitle(title);

        bool accepted = false;
        if (selectcolor_dialog->exec() == QDialog::Accepted) {
            color = selectcolor_dialog->selectedColor();
            accepted = true;
        }

        // Restore focus to the original window
        if (parentWindow) {
            parentWindow->raise();
            parentWindow->activateWindow();
        }
        if (currentFocusWidget) {
            currentFocusWidget->setFocus();
        }

        if (!accepted) {
            return;
        }
    }

    if (color.isValid()) {
        m_color = color;
    }
    else {
        color = m_color;
    }

    QPalette palette = colorLabel->palette();
    palette.setColor(colorLabel->backgroundRole(), color);
    colorLabel->setAutoFillBackground(true);
    colorLabel->setPalette(palette);

    if (clear_to_default) {
        if ("TabFontColor" == m_buttonText
            || "TabBGColor" == m_buttonText) {
            QColor emptyColor;
            emit colorChanged(emptyColor);
        }
        else {
            emit colorChanged(color);
        }
    }
    else {
        emit colorChanged(color);
    }
}
