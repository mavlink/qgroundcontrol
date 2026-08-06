#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

class NTRIPSettings;

struct NTRIPTransportConfig
{
    QString host;
    int port = 2101;
    QString username;
    QString password;
    QString mountpoint;
    QString whitelist;
    bool useTls = false;
    bool allowSelfSignedCerts = false;

    // UDP forwarding
    bool udpForwardEnabled = false;
    QString udpTargetAddress;
    quint16 udpTargetPort = 0;

    bool operator==(const NTRIPTransportConfig&) const = default;

    /// Returns a user-facing error string if the config is unusable, or an empty
    /// string if valid. Covers required fields plus control-character / RFC 7617
    /// injection guards so the manager can reject before any socket is allocated.
    QString validationError() const;

    bool isValid() const { return validationError().isEmpty(); }

    // The three differ-checks below must jointly cover every field (union == operator!=);
    // a new field assigned to none silently breaks reconnect.

    /// Hot fields: a change forces a transport reconnect (TCP/TLS handshake,
    /// HTTP GET line, auth). Excludes whitelist (parser-only) and UDP sink.
    bool transportDiffers(const NTRIPTransportConfig& other) const;

    /// Warm fields: the UDP sidecar can be reconfigured without tearing down
    /// the caster connection.
    bool udpForwardDiffers(const NTRIPTransportConfig& other) const;

    /// Cold field: RTCM whitelist is applied in the parser and does not
    /// require a reconnect or any sink reconfiguration.
    bool whitelistDiffers(const NTRIPTransportConfig& other) const { return whitelist != other.whitelist; }

    /// Identity of the caster a source table belongs to: host + port + credentials
    /// + TLS. Excludes mountpoint/whitelist because the source table is shared
    /// across mountpoints on the same caster. Used as the source-table cache key
    /// so it cannot drift from this config's own notion of "same caster".
    QString casterIdentity() const;

    static NTRIPTransportConfig fromSettings(NTRIPSettings& settings);

    /// Parse a comma-separated RTCM message-id list (e.g. "1005,1077,1087")
    /// into the QVector<int> expected by RTCMParser::setWhitelist. Empty or
    /// non-numeric tokens are ignored.
    static QVector<int> parseWhitelist(const QString& csv);
};
