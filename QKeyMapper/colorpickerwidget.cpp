#include "qkeymapper.h"
#include "colorpickerwidget.h"

bool ColorPickerWidget::s_isColorSelecting = false;

ColorPickerWidget::ColorPickerWidget(QWidget *parent, QString buttonText, int buttonWidth)
    : QWidget(parent)
    , m_color()
    , colorButton(Q_NULLPTR)
    , colorLabel(Q_NULLPTR)
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

void ColorPickerWidget::setColor(QColor &color)
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
    if (text.isEmpty() != true) {
        colorButton->setText(text);
    }
}

void ColorPickerWidget::onPickColor()
{
    QKeyMapper::getInstance()->initSelectColorDialog();

    bool clear_to_default = false;
    if ("TabFontColor" == m_buttonText
        || "FontColor" == m_buttonText
        || "BGColor" == m_buttonText) {
        if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) {
            clear_to_default = true;
        }
    }

    QColor color;
    if (clear_to_default) {
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
    else {
        QColor defaultColor = CROSSHAIR_CROSSHAIRCOLOR_DEFAULT_QCOLOR;
        if (m_color.isValid()) {
            defaultColor = m_color;
        }
        s_isColorSelecting = true;
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
        color = QKeyMapper::getInstance()->m_SelectColorDialog->getColor(defaultColor, this, title, options);
        s_isColorSelecting = false;
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
        QColor emptyColor;
        emit colorChanged(emptyColor);
    }
    else {
        emit colorChanged(color);
    }
}
