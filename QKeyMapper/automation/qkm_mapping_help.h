#ifndef QKM_MAPPING_HELP_H
#define QKM_MAPPING_HELP_H

#include <QJsonObject>
#include <QStringList>

namespace QkmAutomation {

struct MappingHelpResult {
    QJsonObject page;
    QString errorCode;
    QString message;
    bool ok() const { return errorCode.isEmpty(); }
};

QJsonObject mappingHelpCapability();
MappingHelpResult queryMappingHelp(const QJsonObject &arguments,
    const QStringList &originalKeys, const QStringList &mappingKeys);

} // namespace QkmAutomation

#endif
