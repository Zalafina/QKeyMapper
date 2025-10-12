#ifndef QMAPPINGADVANCEDDIALOG_H
#define QMAPPINGADVANCEDDIALOG_H

#include <QDialog>
#include <QMouseEvent>
#include <QStyleFactory>

namespace Ui {
class QMappingAdvancedDialog;
}

class QMappingAdvancedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QMappingAdvancedDialog(QWidget *parent = nullptr);
    ~QMappingAdvancedDialog();

    static QMappingAdvancedDialog *getInstance()
    {
        return m_instance;
    }

    void setUILanguage(int languageindex);

    int getMouseXSpeed(void);
    int getMouseYSpeed(void);
    int getMousePollingInterval(void);
    bool getProcessIconAsTrayIcon(void);
    bool getAcceptVirtualGamepadInput(void);
    QString getShowWindowPointKey(void);
    QString getShowScreenPointKey(void);

    void setMouseXSpeed(int speed);
    void setMouseYSpeed(int speed);
    void setMousePollingInterval(int interval);
    void setProcessIconAsTrayIcon(bool checked);
    void setAcceptVirtualGamepadInput(bool checked);
    void setShowWindowPointKey(const QString &keyname);
    void setShowScreenPointKey(const QString &keyname);

    void setProcessIconAsTrayIconEnabled(bool enabled);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    static QMappingAdvancedDialog *m_instance;
    Ui::QMappingAdvancedDialog *ui;
};

#endif // QMAPPINGADVANCEDDIALOG_H
