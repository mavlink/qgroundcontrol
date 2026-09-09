#pragma once

#include <QtCore/QString>

#include "GPSReceiverProfile.h"

/// Endpoint and receiver settings retained together across a session's retries.
struct GPSConnectionConfig
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

    GPSReceiverProfile profile() const;
    QString validationError() const;
};
