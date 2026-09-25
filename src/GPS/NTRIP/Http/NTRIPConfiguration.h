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
    /// "host:port|sha256-hex" of the self-signed certificate trusted on first use; other endpoints ignore it.
    QString pinnedCertificate;

    bool operator==(const NTRIPConnectionConfig&) const = default;
    [[nodiscard]] QString validationError() const;
    [[nodiscard]] QString streamValidationError() const;

    [[nodiscard]] bool isValid() const { return validationError().isEmpty(); }
};

struct NTRIPRTCMFilterConfig
{
    QString whitelist;
    bool operator==(const NTRIPRTCMFilterConfig&) const = default;
    [[nodiscard]] QVector<int> messageIds() const;
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
