#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QHttpHeaders>

struct NTRIPConnectionConfig;

struct NTRIPHttpRequest
{
    enum class Purpose
    {
        Corrections,
        SourceTable,
    };

    QByteArray bytes;
    QString error;
    QUrl url;
    QHttpHeaders headers;
    /// Credentials are present and the channel is not TLS — caller must warn.
    bool credentialsInClear = false;

    [[nodiscard]] static NTRIPHttpRequest build(const NTRIPConnectionConfig& config,
                                                Purpose purpose = Purpose::Corrections);
};
