#include "qkeymapper.h"
#include "qnotificationsetupdialog.h"
#include "ui_qnotificationsetupdialog.h"

using namespace QKeyMapperConstants;

QNotificationSetupDialog *QNotificationSetupDialog::m_instance = Q_NULLPTR;

QNotificationSetupDialog::QNotificationSetupDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QNotificationSetupDialog)
    , m_FontFamily()
    , m_FontColorPicker(new ColorPickerWidget(this, "FontColor", COLORPICKER_BUTTON_WIDTH_NOTIFICATION_FONTCOLOR))
    , m_BackgroundColorPicker(new ColorPickerWidget(this, "BGColor", COLORPICKER_BUTTON_WIDTH_NOTIFICATION_BGCOLOR))
{
    m_instance = this;
    ui->setupUi(this);

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->fontGroupBox->setStyle(windowsStyle);
        ui->durationGroupBox->setStyle(windowsStyle);
        ui->borderGroupBox->setStyle(windowsStyle);
        ui->offsetGroupBox->setStyle(windowsStyle);
    }

    int x_offset = 8;
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

    QStringList fontWeightList;
    fontWeightList.append(tr("Light"));
    fontWeightList.append(tr("Normal"));
    fontWeightList.append(tr("Bold"));
    ui->fontWeightComboBox->addItems(fontWeightList);
    ui->fontWeightComboBox->setCurrentIndex(NOTIFICATION_FONT_WEIGHT_DEFAULT);

    m_FontColorPicker->setColor(NOTIFICATION_COLOR_NORMAL_DEFAULT);
    m_BackgroundColorPicker->setColor(NOTIFICATION_BACKGROUND_COLOR_DEFAULT);

    ui->opacitySpinBox->setDecimals(NOTIFICATION_OPACITY_DECIMALS);
    ui->opacitySpinBox->setSingleStep(NOTIFICATION_OPACITY_SINGLESTEP);

    ui->fontSizeSpinBox->setRange(NOTIFICATION_FONT_SIZE_MIN, NOTIFICATION_FONT_SIZE_MAX);
    ui->displayDurationSpinBox->setRange(NOTIFICATION_DURATION_MIN, NOTIFICATION_DURATION_MAX);
    ui->fadeinDurationSpinBox->setRange(NOTIFICATION_DURATION_MIN, NOTIFICATION_DURATION_MAX);
    ui->fadeoutDurationSpinBox->setRange(NOTIFICATION_DURATION_MIN, NOTIFICATION_DURATION_MAX);
    ui->borderRadiusSpinBox->setRange(NOTIFICATION_BORDER_RADIUS_MIN, NOTIFICATION_BORDER_RADIUS_MAX);
    ui->paddingSpinBox->setRange(NOTIFICATION_PADDING_MIN, NOTIFICATION_PADDING_MAX);
    ui->opacitySpinBox->setRange(NOTIFICATION_OPACITY_MIN, NOTIFICATION_OPACITY_MAX);
    ui->x_offsetSpinBox->setRange(NOTIFICATION_OFFSET_MIN, NOTIFICATION_OFFSET_MAX);
    ui->y_offsetSpinBox->setRange(NOTIFICATION_OFFSET_MIN, NOTIFICATION_OFFSET_MAX);

    ui->fontSizeSpinBox->setValue(NOTIFICATION_FONT_SIZE_DEFAULT);
    ui->fontItalicCheckBox->setChecked(NOTIFICATION_FONT_ITALIC_DEFAULT);
    ui->displayDurationSpinBox->setValue(NOTIFICATION_DISPLAY_DURATION_DEFAULT);
    ui->fadeinDurationSpinBox->setValue(NOTIFICATION_FADEIN_DURATION_DEFAULT);
    ui->fadeoutDurationSpinBox->setValue(NOTIFICATION_FADEOUT_DURATION_DEFAULT);
    ui->borderRadiusSpinBox->setValue(NOTIFICATION_BORDER_RADIUS_DEFAULT);
    ui->paddingSpinBox->setValue(NOTIFICATION_PADDING_DEFAULT);
    ui->opacitySpinBox->setValue(NOTIFICATION_OPACITY_DEFAULT);
    ui->x_offsetSpinBox->setValue(NOTIFICATION_X_OFFSET_DEFAULT);
    ui->y_offsetSpinBox->setValue(NOTIFICATION_X_OFFSET_DEFAULT);

    QObject::connect(m_FontColorPicker, &ColorPickerWidget::colorChanged, this, &QNotificationSetupDialog::onFontColorChanged);
    QObject::connect(m_BackgroundColorPicker, &ColorPickerWidget::colorChanged, this, &QNotificationSetupDialog::onBackgroundColorChanged);
    QObject::connect(ui->fontFamilyComboBox, &QFontComboBox::currentFontChanged, this,
                     [this](const QFont &font) {
                         m_FontFamily = font.family().trimmed();
                         ui->fontFamilyDefaultButton->setEnabled(!m_FontFamily.isEmpty());
                     });
    QObject::connect(ui->fontFamilyDefaultButton, &QPushButton::clicked, this,
                     [this]() {
                         m_FontFamily.clear();
                         syncFontFamilyControls();
                     });

    ui->fontFamilyDefaultButton->setAutoDefault(false);
    ui->fontFamilyDefaultButton->setDefault(false);
    syncFontFamilyControls();
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
    ui->fontGroupBox->setTitle(tr("Font"));
    m_FontColorPicker->setButtonText(tr("FontColor"));
    m_FontColorPicker->setWindowTitle(tr("Select Notification Font Color"));
    ui->fontSizeLabel->setText(tr("Size"));
    ui->fontWeightLabel->setText(tr("Weight"));
    ui->fontItalicCheckBox->setText(tr("Italic"));
    ui->fontFamilyLabel->setText(tr("Family"));
    ui->fontFamilyDefaultButton->setText(tr("Default"));
    ui->fontFamilyDefaultButton->setToolTip(tr("Use application default font"));
    ui->fontWeightComboBox->setItemText(NOTIFICATION_FONT_WEIGHT_LIGHT,     tr("Light"));
    ui->fontWeightComboBox->setItemText(NOTIFICATION_FONT_WEIGHT_NORMAL,    tr("Normal"));
    ui->fontWeightComboBox->setItemText(NOTIFICATION_FONT_WEIGHT_BOLD,      tr("Bold"));

    /* Duration Group */
    ui->durationGroupBox->setTitle(tr("Duration"));
    ui->displayDurationSpinBox->setSpecialValueText(tr("Undisappear"));
    ui->durationLabel->setText(tr("DisplayDuration"));
    ui->fadeinDurationLabel->setText(tr("FadeIn"));
    ui->fadeoutDurationLabel->setText(tr("FadeOut"));

    /* Border Group */
    ui->borderGroupBox->setTitle(tr("Border"));
    m_BackgroundColorPicker->setButtonText(tr("BGColor"));
    m_BackgroundColorPicker->setWindowTitle(tr("Select Notification Background Color"));
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
    if (m_FontColorPicker == Q_NULLPTR) {
        return NOTIFICATION_COLOR_NORMAL_DEFAULT;
    }
    else {
        return m_FontColorPicker->getColor();
    }
}

QColor QNotificationSetupDialog::getNotification_BackgroundColor()
{
    if (m_BackgroundColorPicker == Q_NULLPTR) {
        return NOTIFICATION_BACKGROUND_COLOR_DEFAULT;
    }
    else {
        return m_BackgroundColorPicker->getColor();
    }
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

QString QNotificationSetupDialog::getNotification_FontFamily() const
{
    return m_FontFamily.trimmed();
}

int QNotificationSetupDialog::getNotification_DisplayDuration()
{
    return ui->displayDurationSpinBox->value();
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

NotificationStyleSettings QNotificationSetupDialog::getNotificationSettings()
{
    NotificationStyleSettings settings;
    settings.fontColor = getNotification_FontColor();
    settings.backgroundColor = getNotification_BackgroundColor();
    settings.fontFamily = getNotification_FontFamily();
    settings.fontSize = ui->fontSizeSpinBox->value();
    settings.fontWeight = ui->fontWeightComboBox->currentIndex();
    settings.fontItalic = ui->fontItalicCheckBox->isChecked();
    settings.displayDuration = ui->displayDurationSpinBox->value();
    settings.fadeInDuration = ui->fadeinDurationSpinBox->value();
    settings.fadeOutDuration = ui->fadeoutDurationSpinBox->value();
    settings.borderRadius = ui->borderRadiusSpinBox->value();
    settings.padding = ui->paddingSpinBox->value();
    settings.opacity = ui->opacitySpinBox->value();
    settings.xOffset = ui->x_offsetSpinBox->value();
    settings.yOffset = ui->y_offsetSpinBox->value();
    return settings;
}

void QNotificationSetupDialog::setNotification_FontColor(const QColor &color)
{
    if (color.isValid()) {
        m_FontColorPicker->setColor(color);
    }
}

void QNotificationSetupDialog::setNotification_BackgroundColor(const QColor &color)
{
    if (color.isValid()) {
        m_BackgroundColorPicker->setColor(color);
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

void QNotificationSetupDialog::setNotification_FontFamily(const QString &family)
{
    m_FontFamily = family.trimmed();
    syncFontFamilyControls();
}

void QNotificationSetupDialog::setNotification_DisplayDuration(int duration)
{
    ui->displayDurationSpinBox->setValue(duration);
}

void QNotificationSetupDialog::setNotification_FadeInDuration(int duration)
{
    ui->fadeinDurationSpinBox->setValue(duration);
}

void QNotificationSetupDialog::setNotification_FadeOutDuration(int duration)
{
    ui->fadeoutDurationSpinBox->setValue(duration);
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

void QNotificationSetupDialog::loadNotificationSettings(const NotificationStyleSettings &settings)
{
    setNotification_FontColor(settings.fontColor);
    setNotification_BackgroundColor(settings.backgroundColor);
    setNotification_FontFamily(settings.fontFamily);
    setNotification_FontSize(settings.fontSize);
    setNotification_FontWeight(qBound(NOTIFICATION_FONT_WEIGHT_MIN, settings.fontWeight, NOTIFICATION_FONT_WEIGHT_MAX));
    setNotification_FontIsItalic(settings.fontItalic);
    setNotification_DisplayDuration(qBound(NOTIFICATION_DURATION_MIN, settings.displayDuration, NOTIFICATION_DURATION_MAX));
    setNotification_FadeInDuration(qBound(NOTIFICATION_DURATION_MIN, settings.fadeInDuration, NOTIFICATION_DURATION_MAX));
    setNotification_FadeOutDuration(qBound(NOTIFICATION_DURATION_MIN, settings.fadeOutDuration, NOTIFICATION_DURATION_MAX));
    setNotification_BorderRadius(qBound(NOTIFICATION_BORDER_RADIUS_MIN, settings.borderRadius, NOTIFICATION_BORDER_RADIUS_MAX));
    setNotification_Padding(qBound(NOTIFICATION_PADDING_MIN, settings.padding, NOTIFICATION_PADDING_MAX));
    setNotification_Opacity(qBound(NOTIFICATION_OPACITY_MIN, settings.opacity, NOTIFICATION_OPACITY_MAX));
    setNotification_X_Offset(qBound(NOTIFICATION_OFFSET_MIN, settings.xOffset, NOTIFICATION_OFFSET_MAX));
    setNotification_Y_Offset(qBound(NOTIFICATION_OFFSET_MIN, settings.yOffset, NOTIFICATION_OFFSET_MAX));
}

void QNotificationSetupDialog::syncFontFamilyControls()
{
    const QString previewFamily = QKeyMapper::resolveConfiguredFontFamily(m_FontFamily);

    const QSignalBlocker blocker(ui->fontFamilyComboBox);
    if (!previewFamily.isEmpty()) {
        ui->fontFamilyComboBox->setCurrentFont(QFont(previewFamily));
    }

    ui->fontFamilyDefaultButton->setEnabled(!m_FontFamily.isEmpty());
}

bool QNotificationSetupDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            if (QKeyMapper::isSelectColorDialogVisible()) {
            }
            else {
                close();
            }
        }
    }
    return QDialog::event(event);
}

void QNotificationSetupDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}

void QNotificationSetupDialog::onFontColorChanged(QColor &color)
{
    Q_UNUSED(color);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote() << "[QNotificationSetupDialog::onFontColorChanged] Notification Font Color -> " << color.name();
#endif
}

void QNotificationSetupDialog::onBackgroundColorChanged(QColor &color)
{
    Q_UNUSED(color);
#ifdef DEBUG_LOGOUT_ON
    qDebug().nospace().noquote()
        << "[QNotificationSetupDialog::onBackgroundColorChanged] Notification Background Color -> " << color.name(QColor::HexArgb)
        << ", Alpha: " << color.alpha();
#endif
}
