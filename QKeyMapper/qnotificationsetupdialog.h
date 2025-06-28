#ifndef QNOTIFICATIONSETUPDIALOG_H
#define QNOTIFICATIONSETUPDIALOG_H

#include <QDialog>

namespace Ui {
class QNotificationSetupDialog;
}

class QNotificationSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QNotificationSetupDialog(QWidget *parent = nullptr);
    ~QNotificationSetupDialog();

    static QNotificationSetupDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

protected:
    bool event(QEvent *event) override;

private:
    static QNotificationSetupDialog *m_instance;
    Ui::QNotificationSetupDialog *ui;
};

#endif // QNOTIFICATIONSETUPDIALOG_H
