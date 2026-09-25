#pragma once

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QVector>

struct NTRIPConnectionConfig
{
    QString host;
    int port = 2101;
    QString username;
    QString password;
    QString mountpoint;
    bool useTls = false;
    bool allowSelfSignedCerts = false;

    bool operator==(const NTRIPConnectionConfig&) const = default;
    QString validationError() const;
    QString streamValidationError() const;

    bool isValid() const { return validationError().isEmpty(); }
};

struct NTRIPRtcmFilterConfig
{
    QString whitelist;
    bool operator==(const NTRIPRtcmFilterConfig&) const = default;
    QVector<int> messageIds() const;
};

struct NTRIPConfiguration
{
    NTRIPConnectionConfig connection;
    NTRIPRtcmFilterConfig filter;
    bool operator==(const NTRIPConfiguration&) const = default;
};

QDebug operator<<(QDebug debug, const NTRIPConnectionConfig& configuration);
QDebug operator<<(QDebug debug, const NTRIPRtcmFilterConfig& configuration);
QDebug operator<<(QDebug debug, const NTRIPConfiguration& configuration);
