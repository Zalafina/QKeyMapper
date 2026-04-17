#include "qtrayiconselectdialog.h"
#include "ui_qtrayiconselectdialog.h"
#include "qkeymapper.h"

using namespace QKeyMapperConstants;

QTrayIconSelectDialog *QTrayIconSelectDialog::m_instance = Q_NULLPTR;

QTrayIconSelectDialog::QTrayIconSelectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTrayIconSelectDialog)
{
    m_instance = this;
    ui->setupUi(this);

    initSelectTrayIconFileDialog();
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

void QTrayIconSelectDialog::initSelectTrayIconFileDialog()
{
    m_SelectTrayIconFileDialog = new QFileDialog(this);
    m_SelectTrayIconFileDialog->setFileMode(QFileDialog::ExistingFiles);
    // m_SelectTrayIconFileDialog->setDirectory(QDir());
}

void QTrayIconSelectDialog::initTrayIconComboBoxes()
{
    ui->idleStateTrayIconSelectComboBox->clear();
    ui->monitoringStateTrayIconSelectComboBox->clear();
    ui->globalStateTrayIconSelectComboBox->clear();
    ui->matchedStateTrayIconSelectComboBox->clear();

    QStringList builtinTrayIconsList = QStringList() \
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

    for (const QString &iconfile : std::as_const(builtinTrayIconsList)) {
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

    ui->addCustomTrayiconsButton->setText(tr("Add custom trayicons"));
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

QString QTrayIconSelectDialog::getTrayIconPathOrDefault(const QString &trayiconpath, const QString &defaultTrayiconpath) const
{
    if (false == trayiconpath.isEmpty()) {
        QIcon trayicon(trayiconpath);
        if (!trayicon.isNull()) {
            if (true == trayiconpath.startsWith(":/")) {
                return trayiconpath;
            }

            if (true == QFileInfo::exists(trayiconpath)) {
                return trayiconpath;
            }
        }
    }

    return defaultTrayiconpath;
}

void QTrayIconSelectDialog::refreshTrayIconComboBoxesWithSelections()
{
    const QString idleStateTrayIcon = getTrayIcon_IdleStateIcon();
    const QString monitoringStateTrayIcon = getTrayIcon_MonitoringStateIcon();
    const QString globalStateTrayIcon = getTrayIcon_GlobalStateIcon();
    const QString matchedStateTrayIcon = getTrayIcon_MatchedStateIcon();

    initTrayIconComboBoxes();

    setTrayIcon_IdleStateIcon(getTrayIconPathOrDefault(idleStateTrayIcon, TRAYICON_IDLE_DEFAULT_FILE));
    setTrayIcon_MonitoringStateIcon(getTrayIconPathOrDefault(monitoringStateTrayIcon, TRAYICON_MONITORING_DEFAULT_FILE));
    setTrayIcon_GlobalStateIcon(getTrayIconPathOrDefault(globalStateTrayIcon, TRAYICON_GLOBAL_DEFAULT_FILE));
    setTrayIcon_MatchedStateIcon(getTrayIconPathOrDefault(matchedStateTrayIcon, TRAYICON_MATCHED_DEFAULT_FILE));
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

void QTrayIconSelectDialog::showEvent(QShowEvent *event)
{
    refreshTrayIconComboBoxesWithSelections();

    QDialog::showEvent(event);
}

#if 0
bool QTrayIconSelectDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            close();
        }
    }
    return QDialog::event(event);
}
#endif

void QTrayIconSelectDialog::closeEvent(QCloseEvent *event)
{
    QKeyMapper::getInstance()->updateSystemTrayDisplay();

    QDialog::closeEvent(event);
}

void QTrayIconSelectDialog::on_addCustomTrayiconsButton_clicked()
{
    if (m_SelectTrayIconFileDialog == Q_NULLPTR) {
        initSelectTrayIconFileDialog();
    }

    m_SelectTrayIconFileDialog->setWindowTitle(tr("Select Custom Tray Icon Files"));
    m_SelectTrayIconFileDialog->setNameFilter(tr("ICO Files (*.ico)"));

    QString caption_string = tr("Select Custom Tray Icon Files");
    QString filter = tr("ICO Files (*.ico)");

    const QStringList selectedFiles = m_SelectTrayIconFileDialog->getOpenFileNames(this,
                                                           caption_string,
                                                           QString(),
                                                           filter);

    if (selectedFiles.isEmpty()) {
        return;
    }

    QDir appDir(QCoreApplication::applicationDirPath());
    const QString targetDirPath = appDir.filePath(CUSTOM_TRAYICONS_DIR);
    QDir targetDir(targetDirPath);

    if (!targetDir.exists() && !appDir.mkpath(CUSTOM_TRAYICONS_DIR)) {
        QMessageBox::warning(this, PROGRAM_NAME,
                             tr("Failed to create the custom tray icon directory.\n\nPath: %1")
                             .arg(QDir::toNativeSeparators(targetDirPath)));
        return;
    }

    QStringList duplicateNames;
    for (const QString &sourceFilePath : selectedFiles) {
        QFileInfo sourceFileInfo(sourceFilePath);
        if (!sourceFileInfo.exists() || !sourceFileInfo.isFile()) {
            continue;
        }

        if (QFile::exists(targetDir.filePath(sourceFileInfo.fileName()))
            && !duplicateNames.contains(sourceFileInfo.fileName(), Qt::CaseInsensitive)) {
            duplicateNames.append(sourceFileInfo.fileName());
        }
    }

    bool overwriteDuplicates = false;
    if (!duplicateNames.isEmpty()) {
        QString message;
        if (duplicateNames.size() == 1) {
            message = tr("Custom tray icon file \"%1\" already exists.\n\nDo you want to overwrite it?")
                      .arg(duplicateNames.first());
        }
        else {
            message = tr("Custom tray icon file \"%1\" and %2 other(s) already exist.\n\nDo you want to overwrite them?")
                      .arg(duplicateNames.first())
                      .arg(duplicateNames.size() - 1);
        }

        QMessageBox::StandardButton reply = QMessageBox::question(this, PROGRAM_NAME, message,
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No);
        overwriteDuplicates = (reply == QMessageBox::Yes);
    }

    int copiedCount = 0;
    int skippedCount = 0;
    int failedCount = 0;

    for (const QString &sourceFilePath : selectedFiles) {
        QFileInfo sourceFileInfo(sourceFilePath);
        if (!sourceFileInfo.exists() || !sourceFileInfo.isFile()) {
            ++failedCount;
            continue;
        }

        const QString sourceAbsolutePath = sourceFileInfo.absoluteFilePath();
        const QString targetFilePath = targetDir.filePath(sourceFileInfo.fileName());

        if (QString::compare(QDir::cleanPath(sourceAbsolutePath),
                             QDir::cleanPath(QFileInfo(targetFilePath).absoluteFilePath()),
                             Qt::CaseInsensitive) == 0) {
            ++skippedCount;
            continue;
        }

        if (QFile::exists(targetFilePath)) {
            if (!overwriteDuplicates) {
                ++skippedCount;
                continue;
            }

            if (!QFile::remove(targetFilePath)) {
                ++failedCount;
                continue;
            }
        }

        if (QFile::copy(sourceAbsolutePath, targetFilePath)) {
            ++copiedCount;
        }
        else {
            ++failedCount;
        }
    }

    if (copiedCount > 0) {
        refreshTrayIconComboBoxesWithSelections();
    }

    QString popupMessage;
    QString popupMessageColor = SUCCESS_COLOR;
    if (copiedCount > 0) {
        popupMessage = tr("Added %1 custom tray icon file(s).").arg(copiedCount);
        if (failedCount > 0) {
            popupMessage += tr(" %1 file(s) failed to copy.").arg(failedCount);
            popupMessageColor = FAILURE_COLOR;
        }
        if (skippedCount > 0) {
            popupMessage += tr(" %1 file(s) were skipped.").arg(skippedCount);
        }
    }
    else if (failedCount > 0) {
        popupMessage = tr("Failed to add the selected custom tray icon file(s).");
        popupMessageColor = FAILURE_COLOR;
    }
    else if (skippedCount > 0) {
        popupMessage = tr("No new custom tray icon files were added.");
        popupMessageColor = FAILURE_COLOR;
    }

    if (!popupMessage.isEmpty()) {
        int popupMessageDisplayTime = 3000;
        emit QKeyMapper::getInstance()->showPopupMessage_Signal(popupMessage, popupMessageColor, popupMessageDisplayTime);
    }
}
