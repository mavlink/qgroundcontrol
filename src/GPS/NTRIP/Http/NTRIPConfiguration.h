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

struct NTRIPRTCMFilterConfig
{
    QString whitelist;
    bool operator==(const NTRIPRTCMFilterConfig&) const = default;
    QVector<int> messageIds() const;
};

struct NTRIPConfiguration
{
    NTRIPConnectionConfig connection;
    NTRIPRTCMFilterConfig filter;
    bool operator==(const NTRIPConfiguration&) const = default;
};

QDebug operator<<(QDebug debug, const NTRIPConnectionConfig& configuration);
QDebug operator<<(QDebug debug, const NTRIPRTCMFilterConfig& configuration);
QDebug operator<<(QDebug debug, const NTRIPConfiguration& configuration);
