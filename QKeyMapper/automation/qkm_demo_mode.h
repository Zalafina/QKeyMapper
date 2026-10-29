#ifndef QKM_DEMO_MODE_H
#define QKM_DEMO_MODE_H

#include <QJsonObject>
#include <QStringList>
#include <QtGlobal>

namespace QkmAutomation {

enum class StartupPurpose { Standard, ConfigurationOnly, Invalid };
StartupPurpose parseStartupPurpose(const QStringList &arguments);
bool initializeStartupPurpose(StartupPurpose purpose);
bool configurationOnlyDemo();
bool currentThreadOnPrivateDesktop();
quint32 startupAllowedScopes();
QJsonObject configurationOnlyPolicy();
bool denyDemoEffect();
#if defined(QKM_AUTOMATION_TESTING) || defined(QKM_LIFECYCLE_TESTING)
quint64 demoEffectDenialsForTest();
#endif

} // namespace QkmAutomation
#endif
