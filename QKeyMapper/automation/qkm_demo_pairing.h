#ifndef QKM_DEMO_PAIRING_H
#define QKM_DEMO_PAIRING_H

#include "qkm_demo_process.h"
#include <QJsonObject>

namespace QkmAutomation {

struct DemoPairingState;

class DemoPairingSession : public QObject {
    Q_OBJECT
public:
    explicit DemoPairingSession(QObject *parent = nullptr);
    ~DemoPairingSession() override;
    bool start(DemoProcess &owner, const QString &title, const QString &instructions, const QString &retry);
    void cancel();
    bool isRunning() const { return m_thread != nullptr; }
    DemoSetupResult result() const { return m_result; }

#ifdef QKM_AUTOMATION_TESTING
    // Exercise the real native prompt on an owned background desktop without switching input.
    bool startPromptForTest(DemoProcess &owner, const QString &title, const QString &text, DWORD timeout);
    QJsonObject workerForTest() const;
#endif

signals:
    void finished();

private:
    bool startImpl(DemoProcess &owner, const QString &title, const QString &instructions,
        const QString &retry, bool display, DWORD timeout);
    HANDLE m_thread = nullptr;
    QWinEventNotifier m_done;
    std::shared_ptr<DemoPairingState> m_state;
    DemoSetupResult m_result;
};

} // namespace QkmAutomation
#endif
