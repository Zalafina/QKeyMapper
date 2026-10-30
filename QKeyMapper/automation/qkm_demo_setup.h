#ifndef QKM_DEMO_SETUP_H
#define QKM_DEMO_SETUP_H

#include "qkm_local_identity.h"
#include <functional>
#include <optional>

namespace QkmAutomation {

inline constexpr char DemoSetupRequestEnv[] = "QKM_DEMO_SETUP_REQUEST";
inline constexpr char DemoSetupAckEnv[] = "QKM_DEMO_SETUP_ACK";
inline constexpr char DemoSetupParentEnv[] = "QKM_DEMO_SETUP_PARENT";
inline constexpr char DemoSetupCreatedEnv[] = "QKM_DEMO_SETUP_CREATED";

struct DemoSetupResult {
    QString failure;
    std::optional<DWORD> nativeError;
    bool ok() const { return failure.isEmpty(); }
};

// Consume and protect the inherited capability before Qt or diagnostic subprocesses start.
void prepareInheritedDemoSetup();

class DemoSetupEndpoint : public QObject {
public:
    DemoSetupEndpoint(std::function<bool()> showManagement, QObject *parent);
    ~DemoSetupEndpoint() override;
    bool ready() const { return m_request && m_ack; }

private:
    HANDLE m_request = nullptr, m_ack = nullptr;
    QWinEventNotifier m_notifier;
};

} // namespace QkmAutomation
#endif
