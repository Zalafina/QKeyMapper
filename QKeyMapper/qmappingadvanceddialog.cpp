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

    ui->mouseXSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);
    ui->mouseYSpeedSpinBox->setRange(MOUSE_SPEED_MIN, MOUSE_SPEED_MAX);
    ui->mouseXSpeedSpinBox->setValue(MOUSE_SPEED_DEFAULT);
    ui->mouseYSpeedSpinBox->setValue(MOUSE_SPEED_DEFAULT);

    ui->mousePollingIntervalSpinBox->setRange(MOUSE_POLLING_INTERNAL_MIN, MOUSE_POLLING_INTERNAL_MAX);
    ui->mousePollingIntervalSpinBox->setValue(MOUSE_POLLING_INTERNAL_DEFAULT);
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
}

int QMappingAdvancedDialog::getMouseXSpeed()
{
    return ui->mouseXSpeedSpinBox->value();
}

int QMappingAdvancedDialog::getMouseYSpeed()
{
    return ui->mouseXSpeedSpinBox->value();
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
