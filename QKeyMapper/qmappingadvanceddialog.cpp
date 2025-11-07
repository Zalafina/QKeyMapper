#include "qkeymapper_worker.h"
#include "qmappingadvanceddialog.h"
#include "ui_qmappingadvanceddialog.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QMappingAdvancedDialog *QMappingAdvancedDialog::m_instance = Q_NULLPTR;

QMappingAdvancedDialog::QMappingAdvancedDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QMappingAdvancedDialog)
{
    m_instance = this;
    ui->setupUi(this);

    QStyle* windowsStyle = QStyleFactory::create("windows");
    ui->mouseGroupBox->setStyle(windowsStyle);
    ui->gamepadGroupBox->setStyle(windowsStyle);

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

    ui->LT_Threshold_PressSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RT_Threshold_PressSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->LT_Threshold_ReleaseSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RT_Threshold_ReleaseSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->LS_Threshold_PushSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RS_Threshold_PushSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->LS_Threshold_ReleaseSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);
    ui->RS_Threshold_ReleaseSpinBox->setRange(GAMEPAD_THRESHOLD_PERCENT_MIN, GAMEPAD_THRESHOLD_PERCENT_MAX);

    ui->LT_Threshold_PressSpinBox->setValue(GAMEPAD_TRIGGER_PRESS_THRESHOLD_PERCENT_DEFAULT);
    ui->RT_Threshold_PressSpinBox->setValue(GAMEPAD_TRIGGER_PRESS_THRESHOLD_PERCENT_DEFAULT);
    ui->LT_Threshold_ReleaseSpinBox->setValue(GAMEPAD_TRIGGER_RELEASE_THRESHOLD_PERCENT_DEFAULT);
    ui->RT_Threshold_ReleaseSpinBox->setValue(GAMEPAD_TRIGGER_RELEASE_THRESHOLD_PERCENT_DEFAULT);
    ui->LS_Threshold_PushSpinBox->setValue(GAMEPAD_JOYSTICK_PUSH_THRESHOLD_PERCENT_DEFAULT);
    ui->RS_Threshold_PushSpinBox->setValue(GAMEPAD_JOYSTICK_PUSH_THRESHOLD_PERCENT_DEFAULT);
    ui->LS_Threshold_ReleaseSpinBox->setValue(GAMEPAD_JOYSTICK_RELEASE_THRESHOLD_PERCENT_DEFAULT);
    ui->RS_Threshold_ReleaseSpinBox->setValue(GAMEPAD_JOYSTICK_RELEASE_THRESHOLD_PERCENT_DEFAULT);

    ui->LT_Threshold_PressSpinBox->setSuffix("%");
    ui->RT_Threshold_PressSpinBox->setSuffix("%");
    ui->LT_Threshold_ReleaseSpinBox->setSuffix("%");
    ui->RT_Threshold_ReleaseSpinBox->setSuffix("%");
    ui->LS_Threshold_PushSpinBox->setSuffix("%");
    ui->RS_Threshold_PushSpinBox->setSuffix("%");
    ui->LS_Threshold_ReleaseSpinBox->setSuffix("%");
    ui->RS_Threshold_ReleaseSpinBox->setSuffix("%");
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

    ui->showWindowPointKeyLabel->setText(tr("ShowWindowPoint"));
    ui->showScreenPointKeyLabel->setText(tr("ShowScreenPoint"));

    ui->gamepadGroupBox->setTitle(tr("Gamepad"));
    ui->LT_ThresholdLabel->setText(tr("LT Threshold"));
    ui->RT_ThresholdLabel->setText(tr("RT Threshold"));
    ui->LS_ThresholdLabel->setText(tr("LS Threshold"));
    ui->RS_ThresholdLabel->setText(tr("RS Threshold"));
    ui->LT_Threshold_PressLabel->setText(tr("Press"));
    ui->RT_Threshold_PressLabel->setText(tr("Press"));
    ui->LS_Threshold_PushLabel->setText(tr("Push"));
    ui->RS_Threshold_PushLabel->setText(tr("Push"));
    ui->LT_Threshold_ReleaseLabel->setText(tr("Release"));
    ui->RT_Threshold_ReleaseLabel->setText(tr("Release"));
    ui->LS_Threshold_ReleaseLabel->setText(tr("Recenter"));
    ui->RS_Threshold_ReleaseLabel->setText(tr("Recenter"));
}

void QMappingAdvancedDialog::initGamepadThresholdSpinBoxes()
{

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

void QMappingAdvancedDialog::setProcessIconAsTrayIconEnabled(bool enabled)
{
    ui->ProcessIconAsTrayIconCheckBox->setEnabled(enabled);
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
