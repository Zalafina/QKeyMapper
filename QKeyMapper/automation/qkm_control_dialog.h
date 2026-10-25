#ifndef QKEYMAPPER_AUTOMATION_CONTROL_DIALOG_H
#define QKEYMAPPER_AUTOMATION_CONTROL_DIALOG_H

#include "qkm_session_server.h"
#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QGroupBox;

namespace QkmAutomation {

class ControlDialog : public QDialog {
    Q_OBJECT
public:
    explicit ControlDialog(QWidget *parent = nullptr);
    ControlDialog(QString storeDirectory, QByteArray applicationBinding, QWidget *parent = nullptr);
    void retranslateUi();
    static QString title();
    AutomationSessionServer &server() { return m_server; }

protected:
    void done(int result) override;
    void showEvent(QShowEvent *event) override;

private:
    void refresh();
    void showPending();
    void finishPending();
    void setEnabledByUser(bool enabled);

    QString m_storeDirectory;
    AutomationSessionServer m_server;
    PairingRequest m_pending;
    QString m_peerExecutable;
    bool m_failed = false;
    QCheckBox *m_enabled;
    QLabel *m_notice;
    QLabel *m_status;
    QLineEdit *m_connection;
    QListWidget *m_pairings;
    QPushButton *m_revoke;
    QGroupBox *m_approval;
    QLabel *m_request;
    QLineEdit *m_executable;
    QCheckBox *m_read;
    QCheckBox *m_configure;
    QCheckBox *m_run;
    QPushButton *m_approve;
    QPushButton *m_reject;
    QPushButton *m_close;
};

} // namespace QkmAutomation
#endif
