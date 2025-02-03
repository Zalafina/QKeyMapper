#include "qkeymapper.h"
#include "colorpickerwidget.h"

ColorPickerWidget::ColorPickerWidget(QWidget *parent)
    : QWidget(parent)
    , m_color()
    , colorButton(Q_NULLPTR)
    , colorLabel(Q_NULLPTR)
{
    colorLabel = new QLabel(this);
    colorButton = new QPushButton(tr("Color"), this);

    colorButton->setFocusPolicy(Qt::NoFocus);
    colorButton->setAutoDefault(false);

    // Set color label style
    colorLabel->setFrameStyle(QFrame::WinPanel | QFrame::Sunken);

    // Set default size for colorLabel and colorButton
    colorLabel->setFixedSize(31, 21);  // Set fixed size for the label
    colorButton->setFixedSize(51, 21); // Set fixed size for the button

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

void ColorPickerWidget::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    colorButton->setText(tr("Color"));
}

void ColorPickerWidget::setColor(QColor &color)
{
    if (color.isValid() && (color != m_color)) {
        m_color = color;
        // If the selected color is valid, update the label with the color name
        // and change the label's background color to the selected color
        QPalette palette = colorLabel->palette();
        palette.setColor(colorLabel->backgroundRole(), color);
        colorLabel->setAutoFillBackground(true);
        colorLabel->setPalette(palette);
    }
}

void ColorPickerWidget::onPickColor()
{
    QKeyMapper::getInstance()->initSelectColorDialog();

    // Open the color picker dialog and allow the user to choose a color
    QColor color = QKeyMapper::getInstance()->m_SelectColorDialog->getColor(Qt::white, this);

    if (color.isValid() && (color != m_color)) {
        // If the selected color is valid, update the label with the color name
        // and change the label's background color to the selected color
        QPalette palette = colorLabel->palette();
        palette.setColor(colorLabel->backgroundRole(), color);
        colorLabel->setAutoFillBackground(true);
        colorLabel->setPalette(palette);

        emit colorChanged(color);
    }
}
