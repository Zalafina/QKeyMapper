#ifndef QTRAYICONSELECTDIALOG_H
#define QTRAYICONSELECTDIALOG_H

#include <QDialog>

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

    void setUILanguage(int languageindex);

    static int getTrayIcon_IdleStateIcon(void);
    static int getTrayIcon_MonitoringStateIcon(void);
    static int getTrayIcon_GlobalStateIcon(void);
    static int getTrayIcon_MatchedStateIcon(void);

    void setTrayIcon_IdleStateIcon(int trayicon_index);
    void setTrayIcon_MonitoringStateIcon(int trayicon_index);
    void setTrayIcon_GlobalStateIcon(int trayicon_index);
    void setTrayIcon_MatchedStateIcon(int trayicon_index);

protected:
    bool event(QEvent *event) override;

private:
    static QTrayIconSelectDialog *m_instance;
    Ui::QTrayIconSelectDialog *ui;
};

#endif // QTRAYICONSELECTDIALOG_H
