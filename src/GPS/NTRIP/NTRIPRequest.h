#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QUrl>

struct NTRIPTransportConfig;

/// Shared caster addressing and HTTP fields for both streaming and source-table discovery.
namespace NTRIPRequest {
struct Request
{
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray bytes;
    bool credentialsInClear = false;
};

QString validationError(const NTRIPTransportConfig& config);
QUrl casterUrl(const NTRIPTransportConfig& config);
Request build(const NTRIPTransportConfig& config, bool sourceTable = false);
}  // namespace NTRIPRequest
