#include "qnotificationsetupdialog.h"
#include "ui_qnotificationsetupdialog.h"

QNotificationSetupDialog *QNotificationSetupDialog::m_instance = Q_NULLPTR;

QNotificationSetupDialog::QNotificationSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QNotificationSetupDialog)
    , m_FontColorPicker(new ColorPickerWidget(this, "FontColor", 61))
    , m_BackgroundColorPicker(new ColorPickerWidget(this, "BGColor", 51))
{
    m_instance = this;
    ui->setupUi(this);

    QStyle* windowsStyle = QStyleFactory::create("windows");
    ui->fontGroupBox->setStyle(windowsStyle);
    ui->durationGroupBox->setStyle(windowsStyle);
    ui->borderGroupBox->setStyle(windowsStyle);
    ui->offsetGroupBox->setStyle(windowsStyle);

    int x_offset = 4;
    int y_offset = 15;
    QRect fontWidgetGeometry = ui->fontGroupBox->geometry();
    int font_color_x = fontWidgetGeometry.x();
    int font_color_y = fontWidgetGeometry.y();
    font_color_x += x_offset;
    font_color_y += y_offset;
    // Set position for the font color picker
    m_FontColorPicker->move(font_color_x, font_color_y);

    x_offset = 19;
    y_offset = 15;
    QRect borderWidgetGeometry = ui->borderGroupBox->geometry();
    int background_color_x = borderWidgetGeometry.x();
    int background_color_y = borderWidgetGeometry.y();
    background_color_x += x_offset;
    background_color_y += y_offset;
    // Set position for the background color picker
    m_BackgroundColorPicker->move(background_color_x, background_color_y);

    m_FontColorPicker->raise();
    m_BackgroundColorPicker->raise();

    QObject::connect(m_FontColorPicker, &ColorPickerWidget::colorChanged, this, &QNotificationSetupDialog::onFontColorChanged);
    QObject::connect(m_BackgroundColorPicker, &ColorPickerWidget::colorChanged, this, &QNotificationSetupDialog::onBackgroundColorChanged);
}

QNotificationSetupDialog::~QNotificationSetupDialog()
{
    delete ui;
}

void QNotificationSetupDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Notification Advanced Setting"));

    /* Font Group */
    m_FontColorPicker->setButtonText(tr("FontColor"));
    m_FontColorPicker->setWindowTitle(tr("Select Notification Font Color"));
    ui->fontGroupBox->setTitle(tr("Font"));
    ui->fontSizeLabel->setText(tr("Size"));
    ui->fontWeightLabel->setText(tr("Weight"));
    ui->fontItalicCheckBox->setText(tr("Italic"));
    int font_weight_index = ui->fontWeightComboBox->currentIndex();
    ui->fontWeightComboBox->clear();
    QStringList fontWeightList;
    fontWeightList.append(tr("Light"));
    fontWeightList.append(tr("Normal"));
    fontWeightList.append(tr("Bold"));
    ui->fontWeightComboBox->addItems(fontWeightList);
    ui->fontWeightComboBox->setCurrentIndex(font_weight_index);

    /* Duration Group */
    ui->durationGroupBox->setTitle(tr("Duration"));
    ui->durationLabel->setText(tr("Duration"));
    ui->fadeinDurationLabel->setText(tr("FadeIn"));
    ui->fadeoutDurationLabel->setText(tr("FadeOut"));

    /* Border Group */
    m_BackgroundColorPicker->setButtonText(tr("BGColor"));
    m_BackgroundColorPicker->setWindowTitle(tr("Select Notification Background Color"));
    ui->borderGroupBox->setTitle(tr("Border"));
    ui->borderRadiusLabel->setText(tr("Radius"));
    ui->paddingLabel->setText(tr("Padding"));
    ui->opacityLabel->setText(tr("Opacity"));

    /* Offset Group */
    ui->offsetGroupBox->setTitle(tr("Offset"));
    ui->x_offsetLabel->setText(tr("X-Offset"));
    ui->y_offsetLabel->setText(tr("Y-Offset"));
}

bool QNotificationSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}

void QNotificationSetupDialog::onFontColorChanged(QColor &color)
{

}

void QNotificationSetupDialog::onBackgroundColorChanged(QColor &color)
{

}
