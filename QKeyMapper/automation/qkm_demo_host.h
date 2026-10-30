#ifndef QKM_DEMO_HOST_H
#define QKM_DEMO_HOST_H

#include "qkm_demo_process.h"
#include "qkm_demo_pairing.h"
#include <QDialog>
#include <QTimer>

class QLabel;
class QPushButton;

namespace QkmAutomation {

class DemoHostDialog : public QDialog {
    Q_OBJECT
public:
    explicit DemoHostDialog(QWidget *parent = nullptr);
    DemoProcess &process() { return m_process; }
    const DemoProcess &process() const { return m_process; }
    void reject() override { close(); }
#ifdef QKM_AUTOMATION_TESTING
    DemoPairingSession &pairingForTest() { return m_pairing; }
#endif

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void refreshText();
    DemoProcess m_process;
    DemoPairingSession m_pairing;
    DemoSetupResult m_pairingResult;
    QLabel *m_status;
    QPushButton *m_close, *m_setup;
    QTimer m_poll;
    bool m_started = false, m_closeFailed = false;
    bool m_closeAfterPairing = false;
};

int runDemoHost(int &argc, char **argv);

} // namespace QkmAutomation
#endif
