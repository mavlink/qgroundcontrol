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

    bool operator==(const NTRIPTransportConfig& other) const {
        return host == other.host && port == other.port &&
               username == other.username && password == other.password &&
               mountpoint == other.mountpoint && whitelist == other.whitelist &&
               useTls == other.useTls;
    }
    bool operator!=(const NTRIPTransportConfig& other) const { return !(*this == other); }

    bool isValid() const { return !host.isEmpty() && port > 0 && port <= 65535; }

    static NTRIPTransportConfig fromSettings(NTRIPSettings &settings);
};
