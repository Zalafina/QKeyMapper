#include "qtrayiconselectdialog.h"
#include "ui_qtrayiconselectdialog.h"
#include "qkeymapper_constants.h"

OrderedMap<int, QString> QTrayIconSelectDialog::s_TrayIconColorMap;
QTrayIconSelectDialog *QTrayIconSelectDialog::m_instance = Q_NULLPTR;

QTrayIconSelectDialog::QTrayIconSelectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTrayIconSelectDialog)
{
    m_instance = this;
    ui->setupUi(this);

    initTrayIconColorMap();
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

void QTrayIconSelectDialog::initTrayIconColorMap()
{
    s_TrayIconColorMap.clear();

    s_TrayIconColorMap.insert(TRAYICON_INDEX_BLACK,     "Black.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_BLUE1,     "Blue1.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_BLUE2,     "Blue2.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_CYAN,      "Cyan.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_GRAY1,     "Gray1.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_GRAY2,     "Gray2.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_GREEN1,    "Green1.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_GREEN2,    "Green2.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_ORANGE,    "Orange.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_PINK,      "Pink.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_PURPLE,    "Purple.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_RED,       "Red.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_WHITE,     "White.ico");
    s_TrayIconColorMap.insert(TRAYICON_INDEX_YELLOW,    "Yellow.ico");
}

void QTrayIconSelectDialog::initTrayIconComboBoxes()
{
    ui->idleStateTrayIconSelectComboBox->clear();
    ui->monitoringStateTrayIconSelectComboBox->clear();
    ui->globalStateTrayIconSelectComboBox->clear();
    ui->matchedStateTrayIconSelectComboBox->clear();

    struct IconInfo {
        QString filename;
        QString displayName;
    };

    QList<IconInfo> systrayIconList = {
        {"Black.ico",   tr("Black.ico")},
        {"Blue1.ico",   tr("Blue1.ico")},
        {"Blue2.ico",   tr("Blue2.ico")},
        {"Cyan.ico",    tr("Cyan.ico")},
        {"Gray1.ico",   tr("Gray1.ico")},
        {"Gray2.ico",   tr("Gray2.ico")},
        {"Green1.ico",  tr("Green1.ico")},
        {"Green2.ico",  tr("Green2.ico")},
        {"Orange.ico",  tr("Orange.ico")},
        {"Pink.ico",    tr("Pink.ico")},
        {"Purple.ico",  tr("Purple.ico")},
        {"Red.ico",     tr("Red.ico")},
        {"White.ico",   tr("White.ico")},
        {"Yellow.ico",  tr("Yellow.ico")}
    };

    for (const IconInfo &iconInfo : systrayIconList) {
        QIcon icon(QString(":/") + iconInfo.filename);
        ui->idleStateTrayIconSelectComboBox->addItem(icon, iconInfo.displayName);
        ui->monitoringStateTrayIconSelectComboBox->addItem(icon, iconInfo.displayName);
        ui->globalStateTrayIconSelectComboBox->addItem(icon, iconInfo.displayName);
        ui->matchedStateTrayIconSelectComboBox->addItem(icon, iconInfo.displayName);
    }
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
    return ui->idleStateTrayIconSelectComboBox->currentIndex();
}

int QTrayIconSelectDialog::getTrayIcon_MonitoringStateIcon()
{
    return ui->monitoringStateTrayIconSelectComboBox->currentIndex();
}

int QTrayIconSelectDialog::getTrayIcon_GlobalStateIcon()
{
    return ui->globalStateTrayIconSelectComboBox->currentIndex();
}

int QTrayIconSelectDialog::getTrayIcon_MatchedStateIcon()
{
    return ui->matchedStateTrayIconSelectComboBox->currentIndex();
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

QIcon QTrayIconSelectDialog::getIdleStateQIcon()
{
    return QIcon(QString(":/%1").arg(s_TrayIconColorMap.value(getTrayIcon_IdleStateIcon())));
}

QIcon QTrayIconSelectDialog::getMonitoringStateQIcon()
{
    return QIcon(QString(":/%1").arg(s_TrayIconColorMap.value(getTrayIcon_MonitoringStateIcon())));
}

QIcon QTrayIconSelectDialog::getGlobalStateQIcon()
{
    return QIcon(QString(":/%1").arg(s_TrayIconColorMap.value(getTrayIcon_GlobalStateIcon())));
}

QIcon QTrayIconSelectDialog::getMatchedStateQIcon()
{
    return QIcon(QString(":/%1").arg(s_TrayIconColorMap.value(getTrayIcon_MatchedStateIcon())));
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
