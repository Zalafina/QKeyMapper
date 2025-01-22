#include "qkeymapper.h"
#include "colorpickerwidget.h"

ColorPickerWidget::ColorPickerWidget(QWidget *parent)
    : QWidget(parent),
    colorButton(Q_NULLPTR),
    colorLabel(Q_NULLPTR)
{
    colorLabel = new QLabel(this);
    QString button_string;
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        button_string = "Color";
    }
    else if (LANGUAGE_CHINESE == QKeyMapper::getLanguageIndex()) {
        button_string = "颜色设定";
    }
    colorButton = new QPushButton(button_string, this);

    colorButton->setFocusPolicy(Qt::NoFocus);
    colorButton->setAutoDefault(false);

    colorLabel->setFrameStyle(QFrame::WinPanel | QFrame::Sunken);

    // Set up layout and add the button and label to it
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

void ColorPickerWidget::onPickColor()
{
    // Open the color picker dialog and allow the user to choose a color
    QString dialog_title;
    if (LANGUAGE_ENGLISH == QKeyMapper::getLanguageIndex()) {
        dialog_title = "Select Color";
    }
    else if (LANGUAGE_CHINESE == QKeyMapper::getLanguageIndex()) {
        dialog_title = "颜色选择";
    }

    QKeyMapper::getInstance()->initSelectColorDialog();

    // Set dialog title dynamically
    QKeyMapper::getInstance()->m_SelectColorDialog->setWindowTitle(dialog_title);

    // Open the color picker dialog and allow the user to choose a color
    QColor color = QKeyMapper::getInstance()->m_SelectColorDialog->getColor(Qt::white, this);

    if (color.isValid()) {
        // If the selected color is valid, update the label with the color name
        // and change the label's background color to the selected color
        QPalette palette = colorLabel->palette();
        palette.setColor(colorLabel->backgroundRole(), color);
        colorLabel->setAutoFillBackground(true);
        colorLabel->setPalette(palette);
    }
}
