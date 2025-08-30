#include "qstartuppositiondialog.h"
#include "ui_qstartuppositiondialog.h"
#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

QStartupPositionDialog *QStartupPositionDialog::m_instance = Q_NULLPTR;

QStartupPositionDialog::QStartupPositionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QStartupPositionDialog)
{
    m_instance = this;
    ui->setupUi(this);

    QStringList startupPositionList;
    startupPositionList.append(tr("Default"));
    startupPositionList.append(tr("LastSaved"));
    startupPositionList.append(tr("Specify"));
    ui->startupPositionComboBox->addItems(startupPositionList);
    ui->startupPositionComboBox->setCurrentIndex(STARTUP_POSITION_LASTSAVED);

    ui->startupSpecifyPositionXSpinBox->setRange(STARTUP_SPECIFY_POSITION_MIN_X,
                                                 STARTUP_SPECIFY_POSITION_MAX_X);
    ui->startupSpecifyPositionYSpinBox->setRange(STARTUP_SPECIFY_POSITION_MIN_Y,
                                                 STARTUP_SPECIFY_POSITION_MAX_Y);
    ui->startupSpecifyPositionXSpinBox->setValue(STARTUP_SPECIFY_POSITION_DEFAULT.x());
    ui->startupSpecifyPositionYSpinBox->setValue(STARTUP_SPECIFY_POSITION_DEFAULT.y());
}

QStartupPositionDialog::~QStartupPositionDialog()
{
    delete ui;
}

void QStartupPositionDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Startup Position Setting"));

    ui->startupPositionLabel->setText(tr("Startup Position"));
    ui->startupSpecifyPositionXLabel->setText(tr("Position X"));
    ui->startupSpecifyPositionYLabel->setText(tr("Position Y"));

    ui->startupPositionComboBox->setItemText(STARTUP_POSITION_DEFAULT,      tr("Default"));
    ui->startupPositionComboBox->setItemText(STARTUP_POSITION_LASTSAVED,    tr("LastSaved"));
    ui->startupPositionComboBox->setItemText(STARTUP_POSITION_SPECIFY,      tr("Specify"));
}

int QStartupPositionDialog::getStartupPosition()
{
    return ui->startupPositionComboBox->currentIndex();
}

QPoint QStartupPositionDialog::getSpecifyStartupPosition()
{
    return QPoint(ui->startupSpecifyPositionXSpinBox->value(),
                  ui->startupSpecifyPositionYSpinBox->value());
}

void QStartupPositionDialog::setStartupPosition(int positon)
{
    ui->startupPositionComboBox->setCurrentIndex(positon);
}

void QStartupPositionDialog::setSpecifyStartupPosition(const QPoint &position)
{
    ui->startupSpecifyPositionXSpinBox->setValue(position.x());
    ui->startupSpecifyPositionYSpinBox->setValue(position.y());
}

bool QStartupPositionDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}

void QStartupPositionDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *focused = focusWidget();
        if (focused && focused != this) {
            focused->clearFocus();
        }
    }

    QDialog::mousePressEvent(event);
}
