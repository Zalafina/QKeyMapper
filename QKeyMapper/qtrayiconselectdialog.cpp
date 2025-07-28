#include "qtrayiconselectdialog.h"
#include "ui_qtrayiconselectdialog.h"
#include "qkeymapper.h"
#include "qkeymapper_constants.h"

QTrayIconSelectDialog *QTrayIconSelectDialog::m_instance = Q_NULLPTR;

QTrayIconSelectDialog::QTrayIconSelectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTrayIconSelectDialog)
{
    m_instance = this;
    ui->setupUi(this);

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

void QTrayIconSelectDialog::initTrayIconComboBoxes()
{
    ui->idleStateTrayIconSelectComboBox->clear();
    ui->monitoringStateTrayIconSelectComboBox->clear();
    ui->globalStateTrayIconSelectComboBox->clear();
    ui->matchedStateTrayIconSelectComboBox->clear();

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
    // Check if the directory exists
    QDir iconDir(dir);
    if (!iconDir.exists()) {
        return;
    }

    // Filter for .ico files
    QStringList iconFilters;
    iconFilters << "*.ico";
    iconDir.setNameFilters(iconFilters);
    iconDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    // Get all .ico files in the directory
    QFileInfoList iconFiles = iconDir.entryInfoList();

    for (const QFileInfo &fileInfo : std::as_const(iconFiles)) {
        QString absolutePath = fileInfo.absoluteFilePath();
        QString relativePath = dir + "/" + fileInfo.fileName();

        // Try to create QIcon and check if it's valid
        QIcon icon(absolutePath);
        if (!icon.isNull()) {
            // Add the valid icon to all four comboboxes
            ui->idleStateTrayIconSelectComboBox->addItem(icon, relativePath);
            ui->monitoringStateTrayIconSelectComboBox->addItem(icon, relativePath);
            ui->globalStateTrayIconSelectComboBox->addItem(icon, relativePath);
            ui->matchedStateTrayIconSelectComboBox->addItem(icon, relativePath);
        }
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
