#pragma once

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

struct NTRIPUdpForwardConfig
{
    bool enabled = false;
    QString address;
    quint16 port = 0;
    bool operator==(const NTRIPUdpForwardConfig&) const = default;
};

struct NTRIPConfiguration
{
    NTRIPConnectionConfig connection;
    NTRIPRtcmFilterConfig filter;
    NTRIPUdpForwardConfig udpForward;
    bool operator==(const NTRIPConfiguration&) const = default;
};
