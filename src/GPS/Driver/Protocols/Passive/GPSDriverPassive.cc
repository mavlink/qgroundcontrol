#include "GPSDriverPassive.h"

int GPSNativePassive::configure(unsigned& baud, const GPSConfig& config)
{
    _configured = false;
    resetIOError();
    resetStream();
    if (config.output_mode != OutputMode::GPS || config.gnss_systems != GNSSSystemsMask::RECEIVER_DEFAULTS ||
        config.dynamicModel != 0 || config.allowPersistentChanges || config.base != GPSBaseStationConfig{} ||
        baud < 1200 || baud > 4000000) {
        log(GPSProtocolLogLevel::Warning, "Passive input requires an explicit baud rate and no receiver configuration");
        return -1;
    }
    if (setBaudrate(baud) < 0) {
        if (ioError() != ReadCancelled) {
            log(GPSProtocolLogLevel::Warning, "Could not set the passive input baud rate");
        }
        return -1;
    }
    setRTCMEnabled(true);
    _configured = true;
    return 0;
}
