#pragma once

#include <QtCore/QString>

class AutoConnectSettings;

/// Values for one NMEA session. Only the selected transport contributes fields.
struct NMEAConnectionConfig
{
    enum Source : int
    {
        Disabled,
        Udp,
        Serial,
        Tcp
    };

    enum ReceiverMode : int
    {
        Passive,
        Ublox
    };

    Source source = Disabled;
    QString host;
    int port = 0;
    QString device;
    int baud = 0;
    ReceiverMode receiverMode = Passive;

    bool operator==(const NMEAConnectionConfig&) const = default;
    QString validationError() const;
    static NMEAConnectionConfig fromSettings(AutoConnectSettings& settings);
};
