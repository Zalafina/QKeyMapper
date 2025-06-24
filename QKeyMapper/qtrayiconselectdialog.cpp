#include "qtrayiconselectdialog.h"
#include "ui_qtrayiconselectdialog.h"
#include "qkeymapper_constants.h"

QTrayIconSelectDialog *QTrayIconSelectDialog::m_instance = Q_NULLPTR;

QTrayIconSelectDialog::QTrayIconSelectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTrayIconSelectDialog)
{
    m_instance = this;
    ui->setupUi(this);

    initTrayIconComboBoxes();
    ui->idleStateTrayIconSelectComboBox->setCurrentIndex(TRAYICON_IDLE_DEFAULT);
    ui->monitoringStateTrayIconSelectComboBox->setCurrentIndex(TRAYICON_MONITORING_DEFAULT);
    ui->globalStateTrayIconSelectComboBox->setCurrentIndex(TRAYICON_GLOBAL_DEFAULT);
    ui->matchedStateTrayIconSelectComboBox->setCurrentIndex(TRAYICON_MATCHED_DEFAULT);
}

QTrayIconSelectDialog::~QTrayIconSelectDialog()
{
    delete ui;
}

void QTrayIconSelectDialog::initTrayIconComboBoxes()
{
    ui->idleStateTrayIconSelectComboBox->clear();
    ui->monitoringStateTrayIconSelectComboBox->clear();
    ui->globalStateTrayIconSelectComboBox->clear();
    ui->matchedStateTrayIconSelectComboBox->clear();

    QStringList systrayIconList;
    systrayIconList.append(tr("Black.ico"));
    systrayIconList.append(tr("Blue1.ico"));
    systrayIconList.append(tr("Blue2.ico"));
    systrayIconList.append(tr("Gray1.ico"));
    systrayIconList.append(tr("Gray2.ico"));
    systrayIconList.append(tr("Green1.ico"));
    systrayIconList.append(tr("Green2.ico"));
    systrayIconList.append(tr("Orange.ico"));
    systrayIconList.append(tr("Pink.ico"));
    systrayIconList.append(tr("Purple.ico"));
    systrayIconList.append(tr("Red.ico"));
    systrayIconList.append(tr("White.ico"));
    systrayIconList.append(tr("Yellow.ico"));

    ui->idleStateTrayIconSelectComboBox->addItems(systrayIconList);
    ui->monitoringStateTrayIconSelectComboBox->addItems(systrayIconList);
    ui->globalStateTrayIconSelectComboBox->addItems(systrayIconList);
    ui->matchedStateTrayIconSelectComboBox->addItems(systrayIconList);
}

void QTrayIconSelectDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Select Systemtray Icon"));

    ui->idleStateTrayIconSelectLabel->setText(tr("Idle"));
    ui->monitoringStateTrayIconSelectLabel->setText(tr("Monitoring"));
    ui->globalStateTrayIconSelectLabel->setText(tr("Global"));
    ui->matchedStateTrayIconSelectLabel->setText(tr("Matched"));

    // Backup current selections
    int idle_icon_index = ui->idleStateTrayIconSelectComboBox->currentIndex();
    int monitoring_icon_index = ui->monitoringStateTrayIconSelectComboBox->currentIndex();
    int global_icon_index = ui->globalStateTrayIconSelectComboBox->currentIndex();
    int matched_icon_index = ui->matchedStateTrayIconSelectComboBox->currentIndex();

    initTrayIconComboBoxes();
    // Restore the previous selections
    ui->idleStateTrayIconSelectComboBox->setCurrentIndex(idle_icon_index);
    ui->monitoringStateTrayIconSelectComboBox->setCurrentIndex(monitoring_icon_index);
    ui->globalStateTrayIconSelectComboBox->setCurrentIndex(global_icon_index);
    ui->matchedStateTrayIconSelectComboBox->setCurrentIndex(matched_icon_index);
}

int QTrayIconSelectDialog::getTrayIcon_IdleStateIcon()
{
    return getInstance()->ui->idleStateTrayIconSelectComboBox->currentIndex();
}

int QTrayIconSelectDialog::getTrayIcon_MonitoringStateIcon()
{
    return getInstance()->ui->monitoringStateTrayIconSelectComboBox->currentIndex();
}

int QTrayIconSelectDialog::getTrayIcon_GlobalStateIcon()
{
    return getInstance()->ui->globalStateTrayIconSelectComboBox->currentIndex();
}

int QTrayIconSelectDialog::getTrayIcon_MatchedStateIcon()
{
    return getInstance()->ui->matchedStateTrayIconSelectComboBox->currentIndex();
}

void QTrayIconSelectDialog::setTrayIcon_IdleStateIcon(int trayicon_index)
{
    ui->idleStateTrayIconSelectComboBox->setCurrentIndex(trayicon_index);
}

void QTrayIconSelectDialog::setTrayIcon_MonitoringStateIcon(int trayicon_index)
{
    ui->monitoringStateTrayIconSelectComboBox->setCurrentIndex(trayicon_index);
}

void QTrayIconSelectDialog::setTrayIcon_GlobalStateIcon(int trayicon_index)
{
    ui->globalStateTrayIconSelectComboBox->setCurrentIndex(trayicon_index);
}

void QTrayIconSelectDialog::setTrayIcon_MatchedStateIcon(int trayicon_index)
{
    ui->matchedStateTrayIconSelectComboBox->setCurrentIndex(trayicon_index);
}

bool QTrayIconSelectDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}
