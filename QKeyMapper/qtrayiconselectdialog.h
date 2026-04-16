#ifndef QTRAYICONSELECTDIALOG_H
#define QTRAYICONSELECTDIALOG_H

#include <QDialog>
#include <orderedmap.h>

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

    void initTrayIconComboBoxes(void);
    void appendCustomTrayIconsFromDir(const QString &dir);
    void setUILanguage(int languageindex);

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
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_addCustomTrayiconsButton_clicked();

private:
    void initSelectTrayIconFileDialog(void);

    static QTrayIconSelectDialog *m_instance;
    QFileDialog *m_SelectTrayIconFileDialog = Q_NULLPTR;
    Ui::QTrayIconSelectDialog *ui;
};

#endif // QTRAYICONSELECTDIALOG_H
