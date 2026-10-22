#ifndef QKEYMAPPER_AUTOMATION_INSTANCE_DISCOVERY_H
#define QKEYMAPPER_AUTOMATION_INSTANCE_DISCOVERY_H

#include "qkm_local_identity.h"

namespace QkmAutomation {

enum class DiscoveryError { None, NotFound, InvalidData, UnsafeStorage, IoError, IdentityRejected, Busy };

struct InstanceDescriptor {
    QString instanceId;
    QString pipeName;
    quint32 processId = 0;
    quint64 creationTime = 0;
    quint32 sessionId = 0;
    QString executable;
};

QString instanceDiscoveryPath(const QString &directory);
bool publishInstanceDiscovery(const QString &directory, const QString &instanceId,
                              const QString &pipeName, DiscoveryError &error);
// This is a hint: the connecting client must still verify the actual pipe peer and proof.
bool readInstanceDiscovery(const QString &directory, const QString &expectedExecutable,
                           InstanceDescriptor &descriptor, DiscoveryError &error);
bool removeInstanceDiscovery(const QString &directory, const QString &instanceId, DiscoveryError &error);

} // namespace QkmAutomation
#endif
