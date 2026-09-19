#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

struct NTRIPConnectionConfig;

struct NTRIPHttpRequest
{
    QByteArray bytes;
    QString error;
    /// Credentials are present and the channel is not TLS — caller must warn.
    bool credentialsInClear = false;

    [[nodiscard]] static NTRIPHttpRequest build(const NTRIPConnectionConfig& config);
};
