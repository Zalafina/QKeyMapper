#include "qkm_message_box.h"
#include "qtrayiconselectdialog.h"
#include "ui_qtrayiconselectdialog.h"
#include "qkeymapper.h"
#include "qstyle_singletons.h"

#include <QComboBox>

using namespace QKeyMapperConstants;

namespace {

bool traySelectionsEqual(
    const QTrayIconSelectDialog::TrayIconSelections &a,
    const QTrayIconSelectDialog::TrayIconSelections &b)
{
    return a.idle == b.idle
        && a.monitoring == b.monitoring
        && a.global == b.global
        && a.matched == b.matched;
}

} // namespace

QTrayIconSelectDialog *QTrayIconSelectDialog::m_instance = Q_NULLPTR;

QTrayIconSelectDialog::QTrayIconSelectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTrayIconSelectDialog)
{
    m_instance = this;
    ui->setupUi(this);

    if (QStyle *windowsStyle = QKeyMapperStyle::windowsStyle()) {
        ui->trayIconGroupBox->setStyle(windowsStyle);
    }

    if (QStyle *fusionStyle = QKeyMapperStyle::fusionStyle()) {
        const auto childWidgets = findChildren<QWidget*>();
        for (QWidget *child : childWidgets) {
            if (!qobject_cast<QGroupBox*>(child)) {
                child->setStyle(fusionStyle);
            }
        }
    }

    initSelectTrayIconFileDialog();
    initTrayIconComboBoxes();
    ui->idleStateTrayIconSelectComboBox->setCurrentText(TRAYICON_IDLE_DEFAULT_FILE);
    ui->monitoringStateTrayIconSelectComboBox->setCurrentText(TRAYICON_MONITORING_DEFAULT_FILE);
    ui->globalStateTrayIconSelectComboBox->setCurrentText(TRAYICON_GLOBAL_DEFAULT_FILE);
    ui->matchedStateTrayIconSelectComboBox->setCurrentText(TRAYICON_MATCHED_DEFAULT_FILE);

    auto notifyDirty = []() {
        if (QKeyMapper *keyMapper = QKeyMapper::getInstance()) {
            keyMapper->requestSaveSettingDirty();
        }
    };

    QObject::connect(ui->idleStateTrayIconSelectComboBox, QOverload<int>::of(&QComboBox::activated),
                     this, [notifyDirty](int) { notifyDirty(); });
    QObject::connect(ui->monitoringStateTrayIconSelectComboBox, QOverload<int>::of(&QComboBox::activated),
                     this, [notifyDirty](int) { notifyDirty(); });
    QObject::connect(ui->globalStateTrayIconSelectComboBox, QOverload<int>::of(&QComboBox::activated),
                     this, [notifyDirty](int) { notifyDirty(); });
    QObject::connect(ui->matchedStateTrayIconSelectComboBox, QOverload<int>::of(&QComboBox::activated),
                     this, [notifyDirty](int) { notifyDirty(); });
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

const QStringList &QTrayIconSelectDialog::builtinTrayIcons()
{
    static const QStringList list{
        ":/Black.ico",
        ":/Blue1.ico",
        ":/Blue2.ico",
        ":/Cyan.ico",
        ":/Gray1.ico",
        ":/Gray2.ico",
        ":/Green1.ico",
        ":/Green2.ico",
        ":/Orange.ico",
        ":/Pink.ico",
        ":/Purple.ico",
        ":/Red.ico",
        ":/White.ico",
        ":/Yellow.ico",
        ":/Tray_Q1_01_Blue.ico",
        ":/Tray_Q1_02_SkyBlue.ico",
        ":/Tray_Q1_03_Cyan.ico",
        ":/Tray_Q1_04_Purple.ico",
        ":/Tray_Q1_05_Green.ico",
        ":/Tray_Q1_06_Orange.ico",
        ":/Tray_Q1_07_Pink.ico",
        ":/Tray_Q1_08_Red.ico",
        ":/Tray_Q1_09_DarkGray.ico",
        ":/Tray_Q1_10_White.ico",
        ":/Tray_Q1_11_Yellow.ico",
        ":/Tray_Q1_12_Lime.ico",
        ":/Tray_Q1_13_Indigo.ico",
        ":/Tray_Q1_14_SilverGray.ico",
        ":/Tray_Q2_01_Blue.ico",
        ":/Tray_Q2_02_SkyBlue.ico",
        ":/Tray_Q2_03_Cyan.ico",
        ":/Tray_Q2_04_Purple.ico",
        ":/Tray_Q2_05_Green.ico",
        ":/Tray_Q2_06_Orange.ico",
        ":/Tray_Q2_07_Pink.ico",
        ":/Tray_Q2_08_Red.ico",
        ":/Tray_Q2_09_DarkGray.ico",
        ":/Tray_Q2_10_White.ico",
        ":/Tray_Q2_11_Yellow.ico",
        ":/Tray_Q2_12_Lime.ico",
        ":/Tray_Q2_13_Indigo.ico",
        ":/Tray_Q2_14_SilverGray.ico",
        ":/Tray_Q3_01_Blue.ico",
        ":/Tray_Q3_02_SkyBlue.ico",
        ":/Tray_Q3_03_Cyan.ico",
        ":/Tray_Q3_04_Purple.ico",
        ":/Tray_Q3_05_Green.ico",
        ":/Tray_Q3_06_Orange.ico",
        ":/Tray_Q3_06b_Orange_BlackQ.ico",
        ":/Tray_Q3_07_Pink.ico",
        ":/Tray_Q3_08_Red.ico",
        ":/Tray_Q3_09_DarkGray.ico",
        ":/Tray_Q3_10_White.ico",
        ":/Tray_Q3_11_Yellow.ico",
        ":/Tray_Q3_12_Lime.ico",
        ":/Tray_Q3_13_Indigo.ico",
        ":/Tray_Q3_14_SilverGray.ico"
    };
    return list;
}

bool QTrayIconSelectDialog::isBuiltinTrayIcon(const QString &trayIconPath)
{
    return builtinTrayIcons().contains(trayIconPath);
}

QkmAutomation::SafeTrayIconScanResult QTrayIconSelectDialog::cachedCustomTrayIconScan() const
{
    return m_CustomTrayIconCache.snapshot();
}

void QTrayIconSelectDialog::invalidateCustomTrayIconCache()
{
    m_CustomTrayIconCache.invalidate();
}

QStringList QTrayIconSelectDialog::availableCustomTrayIcons() const
{
    return cachedCustomTrayIconScan().customIcons;
}

QStringList QTrayIconSelectDialog::availableTrayIcons() const
{
    QStringList icons = builtinTrayIcons();
    icons.append(cachedCustomTrayIconScan().customIcons);
    return icons;
}

QTrayIconSelectDialog::TrayIconSelections QTrayIconSelectDialog::getTrayIconSelections() const
{
    return {
        ui->idleStateTrayIconSelectComboBox->currentText(),
        ui->monitoringStateTrayIconSelectComboBox->currentText(),
        ui->globalStateTrayIconSelectComboBox->currentText(),
        ui->matchedStateTrayIconSelectComboBox->currentText()
    };
}

void QTrayIconSelectDialog::ensureTrayIconItemPresent(const QString &trayIconPath, const QIcon &icon)
{
    if (trayIconPath.isEmpty() || icon.isNull()) {
        return;
    }
    const QList<QComboBox *> combos{
        ui->idleStateTrayIconSelectComboBox,
        ui->monitoringStateTrayIconSelectComboBox,
        ui->globalStateTrayIconSelectComboBox,
        ui->matchedStateTrayIconSelectComboBox
    };
    for (QComboBox *combo : combos) {
        if (combo->findText(trayIconPath) < 0) {
            combo->addItem(icon, trayIconPath);
        }
    }
}

bool QTrayIconSelectDialog::applyVerifiedTrayIconSelections(
    const TrayIconSelections &selections,
    const QHash<QString, QIcon> &verifiedIcons,
    bool deferTrayDisplayRefresh)
{
    auto resolveVerified = [&](const QString &path, QIcon &iconOut) -> bool {
        if (isBuiltinTrayIcon(path)) {
            iconOut = QIcon(path);
            return !iconOut.isNull();
        }
        const auto it = verifiedIcons.constFind(path);
        if (it == verifiedIcons.constEnd() || it.value().isNull()) {
            return false;
        }
        iconOut = it.value();
        return true;
    };

    QIcon idleIcon;
    QIcon monitoringIcon;
    QIcon globalIcon;
    QIcon matchedIcon;
    if (!resolveVerified(selections.idle, idleIcon)
        || !resolveVerified(selections.monitoring, monitoringIcon)
        || !resolveVerified(selections.global, globalIcon)
        || !resolveVerified(selections.matched, matchedIcon)) {
        return false;
    }

    const TrayIconSelections before = getTrayIconSelections();
    if (traySelectionsEqual(before, selections)) {
        return false;
    }

    auto applyOne = [&](QComboBox *combo, const QString &target, const QIcon &icon) {
        if (!isBuiltinTrayIcon(target)) {
            m_VerifiedCustomIcons.insert(target, icon);
        }
        ensureTrayIconItemPresent(target, icon);
        combo->setCurrentText(target);
    };
    applyOne(ui->idleStateTrayIconSelectComboBox, selections.idle, idleIcon);
    applyOne(ui->monitoringStateTrayIconSelectComboBox, selections.monitoring, monitoringIcon);
    applyOne(ui->globalStateTrayIconSelectComboBox, selections.global, globalIcon);
    applyOne(ui->matchedStateTrayIconSelectComboBox, selections.matched, matchedIcon);

    if (QKeyMapper *keyMapper = QKeyMapper::getInstance()) {
        keyMapper->requestSaveSettingDirty();
        if (!deferTrayDisplayRefresh) {
            keyMapper->updateSystemTrayDisplay();
        }
    }
    return true;
}

bool QTrayIconSelectDialog::applyTrayIconSelections(const TrayIconSelections &selections)
{
    QHash<QString, QIcon> verifiedIcons;
    TrayIconSelections normalized = selections;
    QString *targets[4]{&normalized.idle, &normalized.monitoring, &normalized.global, &normalized.matched};
    for (QString *targetPtr : targets) {
        if (isBuiltinTrayIcon(*targetPtr)) {
            continue;
        }
        const QkmAutomation::SafeImageAssetResult asset = QkmAutomation::loadSafeCustomTrayIconAsset(*targetPtr);
        if (!asset.ok || QString::compare(asset.relativePath, *targetPtr, Qt::CaseInsensitive) != 0) {
            return false;
        }
        *targetPtr = asset.relativePath;
        verifiedIcons.insert(asset.relativePath, asset.icon);
    }
    return applyVerifiedTrayIconSelections(normalized, verifiedIcons, false);
}

void QTrayIconSelectDialog::initTrayIconComboBoxes()
{
    m_VerifiedCustomIcons.clear();
    ui->idleStateTrayIconSelectComboBox->clear();
    ui->monitoringStateTrayIconSelectComboBox->clear();
    ui->globalStateTrayIconSelectComboBox->clear();
    ui->matchedStateTrayIconSelectComboBox->clear();

    const QStringList &builtinTrayIconsList = builtinTrayIcons();
    for (const QString &iconfile : builtinTrayIconsList) {
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
    if (dir != QLatin1String(CUSTOM_TRAYICONS_DIR)) {
        return;
    }

    const QkmAutomation::SafeTrayIconScanResult scan = QkmAutomation::scanSafeCustomTrayIcons();
    for (const QkmAutomation::SafeTrayIconScanEntry &entry : scan.entries) {
        if (entry.icon.isNull()) {
            continue;
        }
        m_VerifiedCustomIcons.insert(entry.relativePath, entry.icon);
        ui->idleStateTrayIconSelectComboBox->addItem(entry.icon, entry.relativePath);
        ui->monitoringStateTrayIconSelectComboBox->addItem(entry.icon, entry.relativePath);
        ui->globalStateTrayIconSelectComboBox->addItem(entry.icon, entry.relativePath);
        ui->matchedStateTrayIconSelectComboBox->addItem(entry.icon, entry.relativePath);
    }
}

void QTrayIconSelectDialog::setUILanguage(int languageindex)
{
    Q_UNUSED(languageindex);
    setWindowTitle(tr("Select Systemtray Icon"));

    ui->trayIconGroupBox->setTitle(tr("State Icons"));

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

void QTrayIconSelectDialog::setTrayIconStateSelection(QComboBox *combo, const QString &trayIconFilename, const char *defaultFile)
{
    if (combo == Q_NULLPTR) {
        return;
    }

    if (isBuiltinTrayIcon(trayIconFilename)) {
        combo->setCurrentText(trayIconFilename);
        return;
    }

    if (!trayIconFilename.isEmpty()) {
        const auto cachedIt = m_VerifiedCustomIcons.constFind(trayIconFilename);
        if (cachedIt != m_VerifiedCustomIcons.constEnd() && !cachedIt.value().isNull()) {
            ensureTrayIconItemPresent(trayIconFilename, cachedIt.value());
            combo->setCurrentText(trayIconFilename);
            return;
        }

        const QkmAutomation::SafeImageAssetResult asset = QkmAutomation::loadSafeCustomTrayIconAsset(trayIconFilename);
        if (asset.ok) {
            m_VerifiedCustomIcons.insert(asset.relativePath, asset.icon);
            ensureTrayIconItemPresent(asset.relativePath, asset.icon);
            combo->setCurrentText(asset.relativePath);
            return;
        }
    }

    combo->setCurrentText(QLatin1String(defaultFile));
}

void QTrayIconSelectDialog::setTrayIcon_IdleStateIcon(const QString &trayicon_filename)
{
    setTrayIconStateSelection(ui->idleStateTrayIconSelectComboBox, trayicon_filename, TRAYICON_IDLE_DEFAULT_FILE);
}

void QTrayIconSelectDialog::setTrayIcon_MonitoringStateIcon(const QString &trayicon_filename)
{
    setTrayIconStateSelection(ui->monitoringStateTrayIconSelectComboBox, trayicon_filename, TRAYICON_MONITORING_DEFAULT_FILE);
}

void QTrayIconSelectDialog::setTrayIcon_GlobalStateIcon(const QString &trayicon_filename)
{
    setTrayIconStateSelection(ui->globalStateTrayIconSelectComboBox, trayicon_filename, TRAYICON_GLOBAL_DEFAULT_FILE);
}

void QTrayIconSelectDialog::setTrayIcon_MatchedStateIcon(const QString &trayicon_filename)
{
    setTrayIconStateSelection(ui->matchedStateTrayIconSelectComboBox, trayicon_filename, TRAYICON_MATCHED_DEFAULT_FILE);
}

QString QTrayIconSelectDialog::getTrayIconPathOrDefault(const QString &trayiconpath, const QString &defaultTrayiconpath) const
{
    if (trayiconpath.isEmpty()) {
        return defaultTrayiconpath;
    }

    if (isBuiltinTrayIcon(trayiconpath)) {
        const QIcon trayicon(trayiconpath);
        return !trayicon.isNull() ? trayiconpath : defaultTrayiconpath;
    }

    const QkmAutomation::SafeImageAssetResult asset = QkmAutomation::loadSafeCustomTrayIconAsset(trayiconpath);
    if (asset.ok) {
        m_VerifiedCustomIcons.insert(asset.relativePath, asset.icon);
        return asset.relativePath;
    }

    return defaultTrayiconpath;
}

QIcon QTrayIconSelectDialog::getVerifiedStateQIcon(const QString &trayIconPath, const char *defaultFile) const
{
    if (isBuiltinTrayIcon(trayIconPath)) {
        const QIcon trayIcon(trayIconPath);
        if (!trayIcon.isNull()) {
            return trayIcon;
        }
    }
    else if (!trayIconPath.isEmpty()) {
        const auto cachedIt = m_VerifiedCustomIcons.constFind(trayIconPath);
        if (cachedIt != m_VerifiedCustomIcons.constEnd() && !cachedIt.value().isNull()) {
            return cachedIt.value();
        }
        const QkmAutomation::SafeImageAssetResult asset = QkmAutomation::loadSafeCustomTrayIconAsset(trayIconPath);
        if (asset.ok) {
            m_VerifiedCustomIcons.insert(asset.relativePath, asset.icon);
            return asset.icon;
        }
    }

    return QIcon(QLatin1String(defaultFile));
}

void QTrayIconSelectDialog::refreshTrayIconComboBoxesWithSelections()
{
    const QString idleStateTrayIcon = getTrayIcon_IdleStateIcon();
    const QString monitoringStateTrayIcon = getTrayIcon_MonitoringStateIcon();
    const QString globalStateTrayIcon = getTrayIcon_GlobalStateIcon();
    const QString matchedStateTrayIcon = getTrayIcon_MatchedStateIcon();

    invalidateCustomTrayIconCache();
    initTrayIconComboBoxes();

    const QString resolvedIdle = getTrayIconPathOrDefault(idleStateTrayIcon, TRAYICON_IDLE_DEFAULT_FILE);
    const QString resolvedMonitoring = getTrayIconPathOrDefault(monitoringStateTrayIcon, TRAYICON_MONITORING_DEFAULT_FILE);
    const QString resolvedGlobal = getTrayIconPathOrDefault(globalStateTrayIcon, TRAYICON_GLOBAL_DEFAULT_FILE);
    const QString resolvedMatched = getTrayIconPathOrDefault(matchedStateTrayIcon, TRAYICON_MATCHED_DEFAULT_FILE);

    if (!isBuiltinTrayIcon(resolvedIdle)) {
        ensureTrayIconItemPresent(resolvedIdle, m_VerifiedCustomIcons.value(resolvedIdle));
    }
    if (!isBuiltinTrayIcon(resolvedMonitoring)) {
        ensureTrayIconItemPresent(resolvedMonitoring, m_VerifiedCustomIcons.value(resolvedMonitoring));
    }
    if (!isBuiltinTrayIcon(resolvedGlobal)) {
        ensureTrayIconItemPresent(resolvedGlobal, m_VerifiedCustomIcons.value(resolvedGlobal));
    }
    if (!isBuiltinTrayIcon(resolvedMatched)) {
        ensureTrayIconItemPresent(resolvedMatched, m_VerifiedCustomIcons.value(resolvedMatched));
    }

    setTrayIcon_IdleStateIcon(resolvedIdle);
    setTrayIcon_MonitoringStateIcon(resolvedMonitoring);
    setTrayIcon_GlobalStateIcon(resolvedGlobal);
    setTrayIcon_MatchedStateIcon(resolvedMatched);
}

QIcon QTrayIconSelectDialog::getIdleStateQIcon()
{
    return getVerifiedStateQIcon(getTrayIcon_IdleStateIcon(), TRAYICON_IDLE_DEFAULT_FILE);
}

QIcon QTrayIconSelectDialog::getMonitoringStateQIcon()
{
    return getVerifiedStateQIcon(getTrayIcon_MonitoringStateIcon(), TRAYICON_MONITORING_DEFAULT_FILE);
}

QIcon QTrayIconSelectDialog::getGlobalStateQIcon()
{
    return getVerifiedStateQIcon(getTrayIcon_GlobalStateIcon(), TRAYICON_GLOBAL_DEFAULT_FILE);
}

QIcon QTrayIconSelectDialog::getMatchedStateQIcon()
{
    return getVerifiedStateQIcon(getTrayIcon_MatchedStateIcon(), TRAYICON_MATCHED_DEFAULT_FILE);
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
        QkmMessageBox::warning(this, PROGRAM_NAME,
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

        QMessageBox::StandardButton reply = QkmMessageBox::question(this, PROGRAM_NAME, message,
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
