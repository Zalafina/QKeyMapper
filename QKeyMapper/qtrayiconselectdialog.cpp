#include "qtrayiconselectdialog.h"
#include "ui_qtrayiconselectdialog.h"
#include "qkeymapper.h"
#include "qkeymapper_constants.h"

// OrderedMap<int, QString> QTrayIconSelectDialog::s_TrayIconColorMap;
QTrayIconSelectDialog *QTrayIconSelectDialog::m_instance = Q_NULLPTR;

QTrayIconSelectDialog::QTrayIconSelectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTrayIconSelectDialog)
{
    m_instance = this;
    ui->setupUi(this);

    // initTrayIconColorMap();
    initTrayIconComboBoxes();
    ui->idleStateTrayIconSelectComboBox->setCurrentText(TRAYICON_IDLE_DEFAULT_FILE);
    ui->monitoringStateTrayIconSelectComboBox->setCurrentText(TRAYICON_MONITORING_DEFAULT_FILE);
    ui->globalStateTrayIconSelectComboBox->setCurrentText(TRAYICON_GLOBAL_DEFAULT_FILE);
    ui->matchedStateTrayIconSelectComboBox->setCurrentText(TRAYICON_MATCHED_DEFAULT_FILE);
}

QTrayIconSelectDialog::~QTrayIconSelectDialog()
{
    delete ui;
}

#if 0
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
#endif

void QTrayIconSelectDialog::initTrayIconComboBoxes()
{
    ui->idleStateTrayIconSelectComboBox->clear();
    ui->monitoringStateTrayIconSelectComboBox->clear();
    ui->globalStateTrayIconSelectComboBox->clear();
    ui->matchedStateTrayIconSelectComboBox->clear();

#if 0
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
#endif

    QStringList builtinSystrayIconList = QStringList() \
        << ":/Black.ico"
        << ":/Blue1.ico"
        << ":/Blue2.ico"
        << ":/Cyan.ico"
        << ":/Gray1.ico"
        << ":/Gray2.ico"
        << ":/Green1.ico"
        << ":/Green2.ico"
        << ":/Orange.ico"
        << ":/Pink.ico"
        << ":/Purple.ico"
        << ":/Red.ico"
        << ":/White.ico"
        << ":/Yellow.ico"
        ;

    for (const QString &iconfile : std::as_const(builtinSystrayIconList)) {
        QIcon icon(iconfile);
        ui->idleStateTrayIconSelectComboBox->addItem(icon, iconfile);
        ui->monitoringStateTrayIconSelectComboBox->addItem(icon, iconfile);
        ui->globalStateTrayIconSelectComboBox->addItem(icon, iconfile);
        ui->matchedStateTrayIconSelectComboBox->addItem(icon, iconfile);
    }

    appendCustomTrayIconsFromDir(CUSTOM_TRAYICONS_DIR);
}

void QTrayIconSelectDialog::appendCustomTrayIconsFromDir(const QString &dir)
{

}

void QTrayIconSelectDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Select Systemtray Icon"));

    ui->idleStateTrayIconSelectLabel->setText(tr("Idle"));
    ui->monitoringStateTrayIconSelectLabel->setText(tr("Monitoring"));
    ui->globalStateTrayIconSelectLabel->setText(tr("Global"));
    ui->matchedStateTrayIconSelectLabel->setText(tr("Matched"));

#if 0
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
#endif
}

QString QTrayIconSelectDialog::getTrayIcon_IdleStateIcon()
{
    return ui->idleStateTrayIconSelectComboBox->currentText();
}

QString QTrayIconSelectDialog::getTrayIcon_MonitoringStateIcon()
{
    return ui->monitoringStateTrayIconSelectComboBox->currentText();
}

QString QTrayIconSelectDialog::getTrayIcon_GlobalStateIcon()
{
    return ui->globalStateTrayIconSelectComboBox->currentText();
}

QString QTrayIconSelectDialog::getTrayIcon_MatchedStateIcon()
{
    return ui->matchedStateTrayIconSelectComboBox->currentText();
}

void QTrayIconSelectDialog::setTrayIcon_IdleStateIcon(const QString &trayicon_filename)
{
    ui->idleStateTrayIconSelectComboBox->setCurrentText(trayicon_filename);
}

void QTrayIconSelectDialog::setTrayIcon_MonitoringStateIcon(const QString &trayicon_filename)
{
    ui->monitoringStateTrayIconSelectComboBox->setCurrentText(trayicon_filename);
}

void QTrayIconSelectDialog::setTrayIcon_GlobalStateIcon(const QString &trayicon_filename)
{
    ui->globalStateTrayIconSelectComboBox->setCurrentText(trayicon_filename);
}

void QTrayIconSelectDialog::setTrayIcon_MatchedStateIcon(const QString &trayicon_filename)
{
    ui->matchedStateTrayIconSelectComboBox->setCurrentText(trayicon_filename);
}

QIcon QTrayIconSelectDialog::getIdleStateQIcon()
{
    QIcon tray_icon = QIcon(getTrayIcon_IdleStateIcon());
    if (tray_icon.isNull()) {
        tray_icon = QIcon(TRAYICON_IDLE_DEFAULT_FILE);
    }
    return tray_icon;
}

QIcon QTrayIconSelectDialog::getMonitoringStateQIcon()
{
    QIcon tray_icon = QIcon(getTrayIcon_MonitoringStateIcon());
    if (tray_icon.isNull()) {
        tray_icon = QIcon(TRAYICON_MONITORING_DEFAULT_FILE);
    }
    return tray_icon;
}

QIcon QTrayIconSelectDialog::getGlobalStateQIcon()
{
    QIcon tray_icon = QIcon(getTrayIcon_GlobalStateIcon());
    if (tray_icon.isNull()) {
        tray_icon = QIcon(TRAYICON_GLOBAL_DEFAULT_FILE);
    }
    return tray_icon;
}

QIcon QTrayIconSelectDialog::getMatchedStateQIcon()
{
    QIcon tray_icon = QIcon(getTrayIcon_MatchedStateIcon());
    if (tray_icon.isNull()) {
        tray_icon = QIcon(TRAYICON_MATCHED_DEFAULT_FILE);
    }
    return tray_icon;
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

void QTrayIconSelectDialog::closeEvent(QCloseEvent *event)
{
    QKeyMapper::getInstance()->updateSystemTrayDisplay();

    QDialog::closeEvent(event);
}
