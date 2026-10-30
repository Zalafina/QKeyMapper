#ifndef QKM_DEMO_HOST_H
#define QKM_DEMO_HOST_H

#include "qkm_demo_process.h"
#include <QDialog>
#include <QTimer>

class QLabel;
class QPushButton;

namespace QkmAutomation {

class DemoHostDialog : public QDialog {
    Q_OBJECT
public:
    explicit DemoHostDialog(QWidget *parent = nullptr);
    const DemoProcess &process() const { return m_process; }
    void reject() override { close(); }

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void refreshText();
    DemoProcess m_process;
    QLabel *m_status;
    QPushButton *m_close;
    QTimer m_poll;
    bool m_started = false, m_closeFailed = false;
};

int runDemoHost(int &argc, char **argv);

} // namespace QkmAutomation
#endif
