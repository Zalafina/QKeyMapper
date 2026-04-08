#include "qkeymapper_worker.h"
#include "qmappingadvanceddialog.h"
#include "ui_qmappingadvanceddialog.h"
#include "qkeymapper_constants.h"
#include "qstyle_singletons.h"

using namespace QKeyMapperConstants;

QMappingAdvancedDialog *QMappingAdvancedDialog::m_instance = Q_NULLPTR;

QMappingAdvancedDialog::QMappingAdvancedDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QMappingAdvancedDialog)
{
    m_instance = this;
    ui->setupUi(this);

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->mouseGroupBox->setStyle(windowsStyle);
        ui->gamepadGroupBox->setStyle(windowsStyle);
        ui->customNotificationGroupBox->setStyle(windowsStyle);
    }

    ui->mouseXSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);
    ui->mouseYSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);
    ui->mouseXSpeedSpinBox->setValue(MOUSE_SPEED_DEFAULT);
    ui->mouseYSpeedSpinBox->setValue(MOUSE_SPEED_DEFAULT);

    ui->mousePollingIntervalSpinBox->setRange(MOUSE_POLLING_INTERNAL_MIN, MOUSE_POLLING_INTERNAL_MAX);
    ui->mousePollingIntervalSpinBox->setValue(MOUSE_POLLING_INTERNAL_DEFAULT);

    ui->showWindowPointKeyComboBox->addItem(tr(FUNCTION_KEY_NONE));
    ui->showWindowPointKeyComboBox->addItems(QKeyMapper_Worker::MultiKeyboardInputList);
    ui->showWindowPointKeyComboBox->setCurrentText(SHOW_POINTS_IN_WINDOW_KEY);

    ui->showScreenPointKeyComboBox->addItem(tr(FUNCTION_KEY_NONE));
    ui->showScreenPointKeyComboBox->addItems(QKeyMapper_Worker::MultiKeyboardInputList);
    ui->showScreenPointKeyComboBox->setCurrentText(SHOW_POINTS_IN_SCREEN_KEY);

    ui->customNotificationPositionComboBox->addItem(tr("None"));
    ui->customNotificationPositionComboBox->addItem(tr("Top Left"));
    ui->customNotificationPositionComboBox->addItem(tr("Top Center"));
    ui->customNotificationPositionComboBox->addItem(tr("Top Right"));
    ui->customNotificationPositionComboBox->addItem(tr("Bottom Left"));
    ui->customNotificationPositionComboBox->addItem(tr("Bottom Center"));
    ui->customNotificationPositionComboBox->addItem(tr("Bottom Right"));
    ui->customNotificationPositionComboBox->setCurrentIndex(NOTIFICATION_POSITION_DEFAULT);

    ui->LT_Threshold_PressSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RT_Threshold_PressSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->LT_Threshold_LightPressSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RT_Threshold_LightPressSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->LT_Threshold_ReleaseSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RT_Threshold_ReleaseSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->LS_Threshold_PushSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RS_Threshold_PushSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->LS_Threshold_LightPushSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RS_Threshold_LightPushSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->LS_Threshold_ReleaseSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RS_Threshold_ReleaseSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);

    ui->LT_Threshold_PressSpinBox->setValue(GAMEPAD_TRIGGER_PRESS_THRESHOLD_PERCENT_DEFAULT);
    ui->RT_Threshold_PressSpinBox->setValue(GAMEPAD_TRIGGER_PRESS_THRESHOLD_PERCENT_DEFAULT);
    ui->LT_Threshold_LightPressSpinBox->setValue(GAMEPAD_TRIGGER_LIGHTPRESS_THRESHOLD_PERCENT_DEFAULT);
    ui->RT_Threshold_LightPressSpinBox->setValue(GAMEPAD_TRIGGER_LIGHTPRESS_THRESHOLD_PERCENT_DEFAULT);
    ui->LT_Threshold_ReleaseSpinBox->setValue(GAMEPAD_TRIGGER_RELEASE_THRESHOLD_PERCENT_DEFAULT);
    ui->RT_Threshold_ReleaseSpinBox->setValue(GAMEPAD_TRIGGER_RELEASE_THRESHOLD_PERCENT_DEFAULT);
    ui->LS_Threshold_PushSpinBox->setValue(GAMEPAD_STICK_PUSH_THRESHOLD_PERCENT_DEFAULT);
    ui->RS_Threshold_PushSpinBox->setValue(GAMEPAD_STICK_PUSH_THRESHOLD_PERCENT_DEFAULT);
    ui->LS_Threshold_LightPushSpinBox->setValue(GAMEPAD_STICK_LIGHTPUSH_THRESHOLD_PERCENT_DEFAULT);
    ui->RS_Threshold_LightPushSpinBox->setValue(GAMEPAD_STICK_LIGHTPUSH_THRESHOLD_PERCENT_DEFAULT);
    ui->LS_Threshold_ReleaseSpinBox->setValue(GAMEPAD_STICK_RELEASE_THRESHOLD_PERCENT_DEFAULT);
    ui->RS_Threshold_ReleaseSpinBox->setValue(GAMEPAD_STICK_RELEASE_THRESHOLD_PERCENT_DEFAULT);

    ui->LT_Threshold_PressSpinBox->setSuffix("%");
    ui->RT_Threshold_PressSpinBox->setSuffix("%");
    ui->LT_Threshold_LightPressSpinBox->setSuffix("%");
    ui->RT_Threshold_LightPressSpinBox->setSuffix("%");
    ui->LT_Threshold_ReleaseSpinBox->setSuffix("%");
    ui->RT_Threshold_ReleaseSpinBox->setSuffix("%");
    ui->LS_Threshold_PushSpinBox->setSuffix("%");
    ui->RS_Threshold_PushSpinBox->setSuffix("%");
    ui->LS_Threshold_LightPushSpinBox->setSuffix("%");
    ui->RS_Threshold_LightPushSpinBox->setSuffix("%");
    ui->LS_Threshold_ReleaseSpinBox->setSuffix("%");
    ui->RS_Threshold_ReleaseSpinBox->setSuffix("%");

    QObject::connect(ui->customNotificationEnableCheckBox, &QCheckBox::toggled, this,
                     [this](bool checked) {
                         updateCustomNotificationState();
                         emit customNotificationEnabledChanged(checked);
                     });
    QObject::connect(ui->customNotificationSetupButton, &QPushButton::clicked,
                     this, &QMappingAdvancedDialog::customNotificationSetupRequested);

    updateCustomNotificationState();
}

QMappingAdvancedDialog::~QMappingAdvancedDialog()
{
    delete ui;
}

void QMappingAdvancedDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Mapping Advanced Setting"));

    ui->mouseGroupBox->setTitle(tr("Mouse"));
    ui->mouseXSpeedLabel->setText(tr("X Speed"));
    ui->mouseYSpeedLabel->setText(tr("Y Speed"));
    ui->mousePollingIntervalLabel->setText(tr("Polling"));
    ui->mousePollingIntervalSpinBox->setSuffix(tr(" ms"));

    ui->ProcessIconAsTrayIconCheckBox->setText(tr("ProcessIcon as TrayIcon"));
    ui->acceptVirtualGamepadInputCheckBox->setText(tr("Accept Virtual Gamepad Input"));
    ui->customNotificationGroupBox->setTitle(tr("Custom Notification"));
    ui->customNotificationEnableCheckBox->setText(tr("Enable Custom Notification"));
    ui->customNotificationPositionLabel->setText(tr("Position"));
    ui->customNotificationSetupButton->setText(tr("Custom Notification Setup"));
    ui->customNotificationPositionComboBox->setItemText(NOTIFICATION_POSITION_NONE, tr("None"));
    ui->customNotificationPositionComboBox->setItemText(NOTIFICATION_POSITION_TOP_LEFT, tr("Top Left"));
    ui->customNotificationPositionComboBox->setItemText(NOTIFICATION_POSITION_TOP_CENTER, tr("Top Center"));
    ui->customNotificationPositionComboBox->setItemText(NOTIFICATION_POSITION_TOP_RIGHT, tr("Top Right"));
    ui->customNotificationPositionComboBox->setItemText(NOTIFICATION_POSITION_BOTTOM_LEFT, tr("Bottom Left"));
    ui->customNotificationPositionComboBox->setItemText(NOTIFICATION_POSITION_BOTTOM_CENTER, tr("Bottom Center"));
    ui->customNotificationPositionComboBox->setItemText(NOTIFICATION_POSITION_BOTTOM_RIGHT, tr("Bottom Right"));

    ui->showWindowPointKeyLabel->setText(tr("ShowWindowPoint"));
    ui->showScreenPointKeyLabel->setText(tr("ShowScreenPoint"));

    ui->gamepadGroupBox->setTitle(tr("Gamepad"));
    ui->LT_ThresholdLabel->setText(tr("LT Threshold"));
    ui->RT_ThresholdLabel->setText(tr("RT Threshold"));
    ui->LS_ThresholdLabel->setText(tr("LS Threshold"));
    ui->RS_ThresholdLabel->setText(tr("RS Threshold"));
    ui->LT_Threshold_PressLabel->setText(tr("HPress"));
    ui->RT_Threshold_PressLabel->setText(tr("HPress"));
    ui->LS_Threshold_PushLabel->setText(tr("HPush"));
    ui->RS_Threshold_PushLabel->setText(tr("HPush"));
    ui->LT_Threshold_LightPressLabel->setText(tr("LPress"));
    ui->RT_Threshold_LightPressLabel->setText(tr("LPress"));
    ui->LS_Threshold_LightPushLabel->setText(tr("LPush"));
    ui->RS_Threshold_LightPushLabel->setText(tr("LPush"));
    ui->LT_Threshold_ReleaseLabel->setText(tr("Release"));
    ui->RT_Threshold_ReleaseLabel->setText(tr("Release"));
    ui->LS_Threshold_ReleaseLabel->setText(tr("Recenter"));
    ui->RS_Threshold_ReleaseLabel->setText(tr("Recenter"));
}

void QMappingAdvancedDialog::initGamepadThresholdSpinBoxes()
{
    // Set LT Press / LightPress / Release SpinBox ranges (Release <= LightPress <= Press)
    ui->LT_Threshold_PressSpinBox->setMinimum(ui->LT_Threshold_LightPressSpinBox->value());
    ui->LT_Threshold_LightPressSpinBox->setMinimum(ui->LT_Threshold_ReleaseSpinBox->value());
    ui->LT_Threshold_LightPressSpinBox->setMaximum(ui->LT_Threshold_PressSpinBox->value());
    ui->LT_Threshold_ReleaseSpinBox->setMaximum(ui->LT_Threshold_LightPressSpinBox->value());

    // Set RT Press / LightPress / Release SpinBox ranges (Release <= LightPress <= Press)
    ui->RT_Threshold_PressSpinBox->setMinimum(ui->RT_Threshold_LightPressSpinBox->value());
    ui->RT_Threshold_LightPressSpinBox->setMinimum(ui->RT_Threshold_ReleaseSpinBox->value());
    ui->RT_Threshold_LightPressSpinBox->setMaximum(ui->RT_Threshold_PressSpinBox->value());
    ui->RT_Threshold_ReleaseSpinBox->setMaximum(ui->RT_Threshold_LightPressSpinBox->value());

    // Set LS Push / LightPush / Release SpinBox ranges (Release <= LightPush <= Push)
    ui->LS_Threshold_PushSpinBox->setMinimum(ui->LS_Threshold_LightPushSpinBox->value());
    ui->LS_Threshold_LightPushSpinBox->setMinimum(ui->LS_Threshold_ReleaseSpinBox->value());
    ui->LS_Threshold_LightPushSpinBox->setMaximum(ui->LS_Threshold_PushSpinBox->value());
    ui->LS_Threshold_ReleaseSpinBox->setMaximum(ui->LS_Threshold_LightPushSpinBox->value());

    // Set RS Push / LightPush / Release SpinBox ranges (Release <= LightPush <= Push)
    ui->RS_Threshold_PushSpinBox->setMinimum(ui->RS_Threshold_LightPushSpinBox->value());
    ui->RS_Threshold_LightPushSpinBox->setMinimum(ui->RS_Threshold_ReleaseSpinBox->value());
    ui->RS_Threshold_LightPushSpinBox->setMaximum(ui->RS_Threshold_PushSpinBox->value());
    ui->RS_Threshold_ReleaseSpinBox->setMaximum(ui->RS_Threshold_LightPushSpinBox->value());

    // Connect LT Press / LightPress / Release SpinBox (three-way chain)
    QObject::connect(ui->LT_Threshold_ReleaseSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int releaseValue){
        ui->LT_Threshold_LightPressSpinBox->setMinimum(releaseValue);
    });
    QObject::connect(ui->LT_Threshold_LightPressSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int lightPressValue){
        ui->LT_Threshold_ReleaseSpinBox->setMaximum(lightPressValue);
        ui->LT_Threshold_PressSpinBox->setMinimum(lightPressValue);
    });
    QObject::connect(ui->LT_Threshold_PressSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int pressValue){
        ui->LT_Threshold_LightPressSpinBox->setMaximum(pressValue);
    });

    // Connect RT Press / LightPress / Release SpinBox (three-way chain)
    QObject::connect(ui->RT_Threshold_ReleaseSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int releaseValue){
        ui->RT_Threshold_LightPressSpinBox->setMinimum(releaseValue);
    });
    QObject::connect(ui->RT_Threshold_LightPressSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int lightPressValue){
        ui->RT_Threshold_ReleaseSpinBox->setMaximum(lightPressValue);
        ui->RT_Threshold_PressSpinBox->setMinimum(lightPressValue);
    });
    QObject::connect(ui->RT_Threshold_PressSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int pressValue){
        ui->RT_Threshold_LightPressSpinBox->setMaximum(pressValue);
    });

    // Connect LS Push / LightPush / Release SpinBox (three-way chain)
    QObject::connect(ui->LS_Threshold_ReleaseSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int releaseValue){
        ui->LS_Threshold_LightPushSpinBox->setMinimum(releaseValue);
    });
    QObject::connect(ui->LS_Threshold_LightPushSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int lightPushValue){
        ui->LS_Threshold_ReleaseSpinBox->setMaximum(lightPushValue);
        ui->LS_Threshold_PushSpinBox->setMinimum(lightPushValue);
    });
    QObject::connect(ui->LS_Threshold_PushSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int pushValue){
        ui->LS_Threshold_LightPushSpinBox->setMaximum(pushValue);
    });

    // Connect RS Push / LightPush / Release SpinBox (three-way chain)
    QObject::connect(ui->RS_Threshold_ReleaseSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int releaseValue){
        ui->RS_Threshold_LightPushSpinBox->setMinimum(releaseValue);
    });
    QObject::connect(ui->RS_Threshold_LightPushSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int lightPushValue){
        ui->RS_Threshold_ReleaseSpinBox->setMaximum(lightPushValue);
        ui->RS_Threshold_PushSpinBox->setMinimum(lightPushValue);
    });
    QObject::connect(ui->RS_Threshold_PushSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int pushValue){
        ui->RS_Threshold_LightPushSpinBox->setMaximum(pushValue);
    });
}

int QMappingAdvancedDialog::getMouseXSpeed()
{
    return ui->mouseXSpeedSpinBox->value();
}

int QMappingAdvancedDialog::getMouseYSpeed()
{
    return ui->mouseYSpeedSpinBox->value();
}

int QMappingAdvancedDialog::getMousePollingInterval()
{
    return ui->mousePollingIntervalSpinBox->value();
}

bool QMappingAdvancedDialog::getProcessIconAsTrayIcon()
{
    return ui->ProcessIconAsTrayIconCheckBox->isChecked();
}

bool QMappingAdvancedDialog::getAcceptVirtualGamepadInput()
{
    return ui->acceptVirtualGamepadInputCheckBox->isChecked();
}

QString QMappingAdvancedDialog::getShowWindowPointKey()
{
    return ui->showWindowPointKeyComboBox->currentText();
}

QString QMappingAdvancedDialog::getShowScreenPointKey()
{
    return ui->showScreenPointKeyComboBox->currentText();
}

int QMappingAdvancedDialog::getLeftTriggerPressThreshold()
{
    return ui->LT_Threshold_PressSpinBox->value();
}

int QMappingAdvancedDialog::getLeftTriggerLightPressThreshold()
{
    return ui->LT_Threshold_LightPressSpinBox->value();
}

int QMappingAdvancedDialog::getLeftTriggerReleaseThreshold()
{
    return ui->LT_Threshold_ReleaseSpinBox->value();
}

int QMappingAdvancedDialog::getRightTriggerPressThreshold()
{
    return ui->RT_Threshold_PressSpinBox->value();
}

int QMappingAdvancedDialog::getRightTriggerLightPressThreshold()
{
    return ui->RT_Threshold_LightPressSpinBox->value();
}

int QMappingAdvancedDialog::getRightTriggerReleaseThreshold()
{
    return ui->RT_Threshold_ReleaseSpinBox->value();
}

int QMappingAdvancedDialog::getLeftStickPushThreshold()
{
    return ui->LS_Threshold_PushSpinBox->value();
}

int QMappingAdvancedDialog::getLeftStickLightPushThreshold()
{
    return ui->LS_Threshold_LightPushSpinBox->value();
}

int QMappingAdvancedDialog::getLeftStickReleaseThreshold()
{
    return ui->LS_Threshold_ReleaseSpinBox->value();
}

int QMappingAdvancedDialog::getRightStickPushThreshold()
{
    return ui->RS_Threshold_PushSpinBox->value();
}

int QMappingAdvancedDialog::getRightStickLightPushThreshold()
{
    return ui->RS_Threshold_LightPushSpinBox->value();
}

int QMappingAdvancedDialog::getRightStickReleaseThreshold()
{
    return ui->RS_Threshold_ReleaseSpinBox->value();
}

bool QMappingAdvancedDialog::getCustomNotificationEnabled()
{
    return ui->customNotificationEnableCheckBox->isChecked();
}

int QMappingAdvancedDialog::getCustomNotificationPosition()
{
    return ui->customNotificationPositionComboBox->currentIndex();
}

void QMappingAdvancedDialog::setMouseXSpeed(int speed)
{
    ui->mouseXSpeedSpinBox->setValue(speed);
}

void QMappingAdvancedDialog::setMouseYSpeed(int speed)
{
    ui->mouseYSpeedSpinBox->setValue(speed);
}

void QMappingAdvancedDialog::setMousePollingInterval(int interval)
{
    ui->mousePollingIntervalSpinBox->setValue(interval);
}

void QMappingAdvancedDialog::setProcessIconAsTrayIcon(bool checked)
{
    ui->ProcessIconAsTrayIconCheckBox->setChecked(checked);
}

void QMappingAdvancedDialog::setAcceptVirtualGamepadInput(bool checked)
{
    ui->acceptVirtualGamepadInputCheckBox->setChecked(checked);
}

void QMappingAdvancedDialog::setShowWindowPointKey(const QString &keyname)
{
    if (keyname == FUNCTION_KEY_NONE) {
        ui->showWindowPointKeyComboBox->setCurrentIndex(FUNCTION_KEY_NONE_INDEX);
    }
    else if(QKeyMapper_Worker::MultiKeyboardInputList.contains(keyname)) {
        ui->showWindowPointKeyComboBox->setCurrentText(keyname);
    }
    else {
        ui->showWindowPointKeyComboBox->setCurrentText(SHOW_POINTS_IN_WINDOW_KEY);
    }
}

void QMappingAdvancedDialog::setShowScreenPointKey(const QString &keyname)
{
    if (keyname == FUNCTION_KEY_NONE) {
        ui->showScreenPointKeyComboBox->setCurrentIndex(FUNCTION_KEY_NONE_INDEX);
    }
    else if(QKeyMapper_Worker::MultiKeyboardInputList.contains(keyname)) {
        ui->showScreenPointKeyComboBox->setCurrentText(keyname);
    }
    else {
        ui->showScreenPointKeyComboBox->setCurrentText(SHOW_POINTS_IN_SCREEN_KEY);
    }
}

void QMappingAdvancedDialog::setLeftTriggerPressThreshold(int threshold)
{
    ui->LT_Threshold_PressSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setLeftTriggerLightPressThreshold(int threshold)
{
    ui->LT_Threshold_LightPressSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setLeftTriggerReleaseThreshold(int threshold)
{
    ui->LT_Threshold_ReleaseSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setRightTriggerPressThreshold(int threshold)
{
    ui->RT_Threshold_PressSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setRightTriggerLightPressThreshold(int threshold)
{
    ui->RT_Threshold_LightPressSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setRightTriggerReleaseThreshold(int threshold)
{
    ui->RT_Threshold_ReleaseSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setLeftStickPushThreshold(int threshold)
{
    ui->LS_Threshold_PushSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setLeftStickLightPushThreshold(int threshold)
{
    ui->LS_Threshold_LightPushSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setLeftStickReleaseThreshold(int threshold)
{
    ui->LS_Threshold_ReleaseSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setRightStickPushThreshold(int threshold)
{
    ui->RS_Threshold_PushSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setRightStickLightPushThreshold(int threshold)
{
    ui->RS_Threshold_LightPushSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setRightStickReleaseThreshold(int threshold)
{
    ui->RS_Threshold_ReleaseSpinBox->setValue(threshold);
}

void QMappingAdvancedDialog::setCustomNotificationEnabled(bool enabled)
{
    ui->customNotificationEnableCheckBox->setChecked(enabled);
    updateCustomNotificationState();
}

void QMappingAdvancedDialog::setCustomNotificationPosition(int position)
{
    if (NOTIFICATION_POSITION_NONE <= position && position <= NOTIFICATION_POSITION_BOTTOM_RIGHT) {
        ui->customNotificationPositionComboBox->setCurrentIndex(position);
    }
    else {
        ui->customNotificationPositionComboBox->setCurrentIndex(NOTIFICATION_POSITION_DEFAULT);
    }
}

void QMappingAdvancedDialog::setProcessIconAsTrayIconEnabled(bool enabled)
{
    ui->ProcessIconAsTrayIconCheckBox->setEnabled(enabled);
}

void QMappingAdvancedDialog::updateCustomNotificationState()
{
    const bool enabled = ui->customNotificationEnableCheckBox->isChecked();
    ui->customNotificationPositionLabel->setEnabled(enabled);
    ui->customNotificationPositionComboBox->setEnabled(enabled);
    ui->customNotificationSetupButton->setEnabled(enabled);
}

bool QMappingAdvancedDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}

void QMappingAdvancedDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}
