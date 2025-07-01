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
    m_BackgroundColorPicker->setShowAlphaChannel(true);

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
    if (font_weight_index < 0) {
        font_weight_index = NOTIFICATION_FONT_WEIGHT_DEFAULT;
    }
    ui->fontWeightComboBox->clear();
    QStringList fontWeightList;
    fontWeightList.append(tr("Light"));
    fontWeightList.append(tr("Normal"));
    fontWeightList.append(tr("Bold"));
    ui->fontWeightComboBox->addItems(fontWeightList);
    ui->fontWeightComboBox->setCurrentIndex(font_weight_index);

    /* Duration Group */
    ui->durationGroupBox->setTitle(tr("Duration"));
    ui->durationLabel->setText(tr("DisplayDuration"));
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

QColor QNotificationSetupDialog::getNotification_FontColor()
{
    return m_NotificationFontColor;
}

QColor QNotificationSetupDialog::getNotification_BackgroundColor()
{
    return m_NotificationBackgroundColor;
}

int QNotificationSetupDialog::getNotification_FontSize()
{
    return ui->fontSizeSpinBox->value();
}

int QNotificationSetupDialog::getNotification_FontWeight()
{
    return ui->fontWeightComboBox->currentIndex();
}

bool QNotificationSetupDialog::getNotification_FontIsItalic()
{
    return ui->fontItalicCheckBox->isChecked();
}

int QNotificationSetupDialog::getNotification_DisplayDuration()
{
    return ui->durationSpinBox->value();
}

int QNotificationSetupDialog::getNotification_FadeInDuration()
{
    return ui->fadeinDurationSpinBox->value();
}

int QNotificationSetupDialog::getNotification_FadeOutDuration()
{
    return ui->fadeoutDurationSpinBox->value();
}

int QNotificationSetupDialog::getNotification_BorderRadius()
{
    return ui->borderRadiusSpinBox->value();
}

int QNotificationSetupDialog::getNotification_Padding()
{
    return ui->paddingSpinBox->value();
}

double QNotificationSetupDialog::getNotification_Opacity()
{
    return ui->opacitySpinBox->value();
}

int QNotificationSetupDialog::getNotification_X_Offset()
{
    return ui->x_offsetSpinBox->value();
}

int QNotificationSetupDialog::getNotification_Y_Offset()
{
    return ui->y_offsetSpinBox->value();
}

void QNotificationSetupDialog::setNotification_FontColor(const QColor &color)
{
    if (color.isValid()) {
        m_NotificationFontColor = color;
        m_FontColorPicker->setColor(m_NotificationFontColor);
    }
}

void QNotificationSetupDialog::setNotification_BackgroundColor(const QColor &color)
{
    if (color.isValid()) {
        m_NotificationBackgroundColor = color;
        m_BackgroundColorPicker->setColor(m_NotificationBackgroundColor);
    }
}

void QNotificationSetupDialog::setNotification_FontSize(int size)
{
    ui->fontSizeSpinBox->setValue(size);
}

void QNotificationSetupDialog::setNotification_FontWeight(int weight)
{
    ui->fontWeightComboBox->setCurrentIndex(weight);
}

void QNotificationSetupDialog::setNotification_FontIsItalic(bool italic)
{
    ui->fontItalicCheckBox->setChecked(italic);
}

void QNotificationSetupDialog::setNotification_Duration(int duration)
{
    ui->durationSpinBox->setValue(duration);
}

void QNotificationSetupDialog::setNotification_FadeInDuration(int fadein_duration)
{
    ui->fadeinDurationSpinBox->setValue(fadein_duration);
}

void QNotificationSetupDialog::setNotification_FadeOutDuration(int fadeout_duration)
{
    ui->fadeoutDurationSpinBox->setValue(fadeout_duration);
}

void QNotificationSetupDialog::setNotification_BorderRadius(int radius)
{
    ui->borderRadiusSpinBox->setValue(radius);
}

void QNotificationSetupDialog::setNotification_Padding(int padding)
{
    ui->paddingSpinBox->setValue(padding);
}

void QNotificationSetupDialog::setNotification_Opacity(double opacity)
{
    ui->opacitySpinBox->setValue(opacity);
}

void QNotificationSetupDialog::setNotification_X_Offset(int offset)
{
    ui->x_offsetSpinBox->setValue(offset);
}

void QNotificationSetupDialog::setNotification_Y_Offset(int offset)
{
    ui->y_offsetSpinBox->setValue(offset);
}

bool QNotificationSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            if (ColorPickerWidget::s_isColorSelecting) {
            }
            else {
                close();
            }
        }
    }
    return QDialog::event(event);
}

void QNotificationSetupDialog::showEvent(QShowEvent *event)
{
    // Load Notification Font Color
    if (m_NotificationFontColor.isValid() != true) {
        m_NotificationFontColor = NOTIFICATION_COLOR_NORMAL_DEFAULT;
    }
    m_FontColorPicker->setColor(m_NotificationFontColor);

    // Load Notification Background Color
    if (m_NotificationBackgroundColor.isValid() != true) {
        m_NotificationBackgroundColor = NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
    }
    m_BackgroundColorPicker->setColor(m_NotificationBackgroundColor);

    QDialog::showEvent(event);
}

void QNotificationSetupDialog::onFontColorChanged(QColor &color)
{
    if (color != m_NotificationFontColor) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote() << "[QNotificationSetupDialog::onFontColorChanged] Notification Font Color -> " << color.name();
#endif
        if (color.isValid()) {
            m_NotificationFontColor = color;
        }
        else {
            m_NotificationFontColor = NOTIFICATION_COLOR_NORMAL_DEFAULT;
        }
    }
}

void QNotificationSetupDialog::onBackgroundColorChanged(QColor &color)
{
    if (color != m_NotificationBackgroundColor) {
#ifdef DEBUG_LOGOUT_ON
        qDebug().nospace().noquote()
            << "[QNotificationSetupDialog::onBackgroundColorChanged] Notification Background Color -> " << color.name()
            << ", Alpha: " << color.alpha();
#endif
        if (color.isValid()) {
            m_NotificationBackgroundColor = color;
        }
        else {
            m_NotificationBackgroundColor = NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
        }
    }
}
