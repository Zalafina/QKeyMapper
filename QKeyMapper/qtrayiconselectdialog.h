#ifndef QTRAYICONSELECTDIALOG_H
#define QTRAYICONSELECTDIALOG_H

#include <QDialog>
#include <orderedmap.h>

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

    void initTrayIconColorMap(void);
    void initTrayIconComboBoxes(void);
    void setUILanguage(int languageindex);

    int getTrayIcon_IdleStateIcon(void);
    int getTrayIcon_MonitoringStateIcon(void);
    int getTrayIcon_GlobalStateIcon(void);
    int getTrayIcon_MatchedStateIcon(void);

    void setTrayIcon_IdleStateIcon(int trayicon_index);
    void setTrayIcon_MonitoringStateIcon(int trayicon_index);
    void setTrayIcon_GlobalStateIcon(int trayicon_index);
    void setTrayIcon_MatchedStateIcon(int trayicon_index);

    QIcon getIdleStateQIcon(void);
    QIcon getMonitoringStateQIcon(void);
    QIcon getGlobalStateQIcon(void);
    QIcon getMatchedStateQIcon(void);

protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

public:
    static OrderedMap<int, QString> s_TrayIconColorMap;

private:
    static QTrayIconSelectDialog *m_instance;
    Ui::QTrayIconSelectDialog *ui;
};

#endif // QTRAYICONSELECTDIALOG_H
