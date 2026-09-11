#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QHttpHeaders>

struct NTRIPTransportConfig;

/// Shared caster addressing and HTTP fields for both streaming and source-table discovery.
namespace NTRIPRequest {
struct Request
{
    QUrl url;
    QHttpHeaders headers;
    QByteArray bytes;
    bool credentialsInClear = false;
};

QString validationError(const NTRIPTransportConfig& config);
QUrl casterUrl(const NTRIPTransportConfig& config);
Request build(const NTRIPTransportConfig& config, bool sourceTable = false);
}  // namespace NTRIPRequest
