#ifndef QTRAYICONSELECTDIALOG_H
#define QTRAYICONSELECTDIALOG_H

#include <QDialog>
#include <QHash>
#include <QIcon>
#include <orderedmap.h>
#include "automation/qkm_safe_image_asset.h"

class QComboBox;
class QFileDialog;

namespace Ui {
class QTrayIconSelectDialog;
}

class QTrayIconSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QTrayIconSelectDialog(QWidget *parent = nullptr);
    ~QTrayIconSelectDialog();

    static QTrayIconSelectDialog *getInstance()
    {
        return m_instance;
    }

    struct TrayIconSelections {
        QString idle;
        QString monitoring;
        QString global;
        QString matched;
    };

    static const QStringList &builtinTrayIcons(void);
    static bool isBuiltinTrayIcon(const QString &trayIconPath);
    QkmAutomation::SafeTrayIconScanResult cachedCustomTrayIconScan(void) const;
    void invalidateCustomTrayIconCache(void);
    QStringList availableCustomTrayIcons(void) const;
    QStringList availableTrayIcons(void) const;

    void initTrayIconComboBoxes(void);
    void appendCustomTrayIconsFromDir(const QString &dir);
    void setUILanguage(int languageindex);

    TrayIconSelections getTrayIconSelections(void) const;
    bool applyVerifiedTrayIconSelections(
        const TrayIconSelections &selections,
        const QHash<QString, QIcon> &verifiedIcons,
        bool deferTrayDisplayRefresh = false);
    bool applyTrayIconSelections(const TrayIconSelections &selections);

    QString getTrayIcon_IdleStateIcon(void);
    QString getTrayIcon_MonitoringStateIcon(void);
    QString getTrayIcon_GlobalStateIcon(void);
    QString getTrayIcon_MatchedStateIcon(void);

    void setTrayIcon_IdleStateIcon(const QString &trayicon_filename);
    void setTrayIcon_MonitoringStateIcon(const QString &trayicon_filename);
    void setTrayIcon_GlobalStateIcon(const QString &trayicon_filename);
    void setTrayIcon_MatchedStateIcon(const QString &trayicon_filename);

    QIcon getIdleStateQIcon(void);
    QIcon getMonitoringStateQIcon(void);
    QIcon getGlobalStateQIcon(void);
    QIcon getMatchedStateQIcon(void);

protected:
    // bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_addCustomTrayiconsButton_clicked();

private:
    void initSelectTrayIconFileDialog(void);
    void refreshTrayIconComboBoxesWithSelections(void);
    QString getTrayIconPathOrDefault(const QString &trayiconpath, const QString &defaultTrayiconpath) const;
    QIcon getVerifiedStateQIcon(const QString &trayIconPath, const char *defaultFile) const;
    void ensureTrayIconItemPresent(const QString &trayIconPath, const QIcon &icon);
    void setTrayIconStateSelection(QComboBox *combo, const QString &trayIconFilename, const char *defaultFile);

    static QTrayIconSelectDialog *m_instance;
    QFileDialog *m_SelectTrayIconFileDialog = Q_NULLPTR;
    Ui::QTrayIconSelectDialog *ui;
    mutable QkmAutomation::SafeTrayIconCatalogCache m_CustomTrayIconCache;
    mutable QHash<QString, QIcon> m_VerifiedCustomIcons;
};

#endif // QTRAYICONSELECTDIALOG_H
