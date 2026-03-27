#pragma once

#include <QtCore/QString>

class NTRIPSettings;

struct NTRIPTransportConfig {
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

    bool isValid() const { return !host.isEmpty() && port > 0 && port <= 65535; }

    static NTRIPTransportConfig fromSettings(NTRIPSettings &settings);
};
