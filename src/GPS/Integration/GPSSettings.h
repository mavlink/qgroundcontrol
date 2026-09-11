#pragma once

#include "GPSReceiverProfile.h"

class AutoConnectSettings;
class RTKSettings;

namespace GPSSettings {

/// Snapshot of saved connection intent; runtime controllers consume only the canonical profile.
struct Connection
{
    GPSReceiverProfile profile;
    bool automatic = false;
    QString validationError;
};

Connection nmea(AutoConnectSettings& settings);
Connection receiver(RTKSettings& settings, AutoConnectSettings& autoConnect);

}  // namespace GPSSettings
