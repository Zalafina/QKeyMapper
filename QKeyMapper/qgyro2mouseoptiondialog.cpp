#include "qgyro2mouseoptiondialog.h"
#include "ui_qgyro2mouseoptiondialog.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QGyro2MouseOptionDialog *QGyro2MouseOptionDialog::m_instance = Q_NULLPTR;

QGyro2MouseOptionDialog::QGyro2MouseOptionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QGyro2MouseOptionDialog)
{
    m_instance = this;
    ui->setupUi(this);

    QStringList mouseInputSourceList;
    mouseInputSourceList.append(tr("Gyro X"));
    mouseInputSourceList.append(tr("Gyro Y"));
    mouseInputSourceList.append(tr("Gyro Z"));
    ui->mouseXInputSourceComboBox->addItems(mouseInputSourceList);
    ui->mouseYInputSourceComboBox->addItems(mouseInputSourceList);
    ui->mouseXInputSourceComboBox->setCurrentIndex(GYRO2MOUSE_MOUSE_X_INPUT_SOURCE_DEFAULT);
    ui->mouseYInputSourceComboBox->setCurrentIndex(GYRO2MOUSE_MOUSE_Y_INPUT_SOURCE_DEFAULT);
}

QGyro2MouseOptionDialog::~QGyro2MouseOptionDialog()
{
    delete ui;
}

void QGyro2MouseOptionDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Gyro2Mouse Advanced Setting"));

    ui->mouseXInputSourceLabel->setText(tr("X Source"));
    ui->mouseYInputSourceLabel->setText(tr("Y Source"));

    ui->mouseXRevertCheckBox->setText(tr("X Revert"));
    ui->mouseYRevertCheckBox->setText(tr("Y Revert"));

    ui->mouseXInputSourceComboBox->setItemText(GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_X, tr("Gyro X"));
    ui->mouseXInputSourceComboBox->setItemText(GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Y, tr("Gyro Y"));
    ui->mouseXInputSourceComboBox->setItemText(GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Z, tr("Gyro Z"));
    ui->mouseYInputSourceComboBox->setItemText(GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_X, tr("Gyro X"));
    ui->mouseYInputSourceComboBox->setItemText(GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Y, tr("Gyro Y"));
    ui->mouseYInputSourceComboBox->setItemText(GYRO2MOUSE_MOUSE_INPUT_SOURCE_GYRO_Z, tr("Gyro Z"));
}

int QGyro2MouseOptionDialog::getGyro2Mouse_MouseXSource()
{
    return getInstance()->ui->mouseXInputSourceComboBox->currentIndex();
}

int QGyro2MouseOptionDialog::getGyro2Mouse_MouseYSource()
{
    return getInstance()->ui->mouseYInputSourceComboBox->currentIndex();
}

bool QGyro2MouseOptionDialog::getGyro2Mouse_MouseXRevert()
{
    return getInstance()->ui->mouseXRevertCheckBox->isChecked();
}

bool QGyro2MouseOptionDialog::getGyro2Mouse_MouseYRevert()
{
    return getInstance()->ui->mouseYRevertCheckBox->isChecked();
}

void QGyro2MouseOptionDialog::setGyro2Mouse_MouseXSource(int source_index)
{

    ui->mouseXInputSourceComboBox->setCurrentIndex(source_index);
}

void QGyro2MouseOptionDialog::setGyro2Mouse_MouseYSource(int source_index)
{
    ui->mouseYInputSourceComboBox->setCurrentIndex(source_index);
}

void QGyro2MouseOptionDialog::setGyro2Mouse_MouseXRevert(bool revert)
{
    ui->mouseXRevertCheckBox->setChecked(revert);
}

void QGyro2MouseOptionDialog::setGyro2Mouse_MouseYRevert(bool revert)
{
    ui->mouseYRevertCheckBox->setChecked(revert);
}

bool QGyro2MouseOptionDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}

void QGyro2MouseOptionDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}
