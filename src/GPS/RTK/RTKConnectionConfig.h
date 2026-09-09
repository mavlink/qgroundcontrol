#pragma once

#include <QtCore/QString>

#include "GPSDriver.h"

class RTKSettings;

/// Endpoint and receiver settings retained together across a session's retries.
struct RTKConnectionConfig
{
    enum Transport : int
    {
        Serial,
        Tcp,
        Udp
    };

    Transport transport = Serial;
    QString device;
    QString receiverName;
    GPSType receiverType = GPSType::u_blox;
    QString host;
    int port = 0;
    int localPort = 0;
    int baseMode = 0;
    GPSReceiverConfig receiver{.base = {.surveyInAccMeters = 2.0, .surveyInDurationSecs = 180}};

    QString validationError() const;
    static RTKConnectionConfig fromSettings(RTKSettings& settings);
};
