#include <cstdlib>

#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"

#if defined(QT_CORE_LIB) || defined(QT_VERSION)
#error The receiver configuration consumer must not inherit Qt dependencies.
#endif

int main()
{
    using Error = GPSReceiverConfigError;
    using Role = GPSReceiverConfig::Role;

    if (gpsValidateBaseStationConfig({}) != Error::InvalidSurveyIn ||
        gpsValidateReceiverConfig(GPSType::ublox, {}) != Error::InvalidSurveyIn) {
        return EXIT_FAILURE;
    }

    GPSReceiverConfig config{.role = Role::Position, .dynamicModel = 0};
    if (!gpsReceiverCapabilities(GPSType::ublox, config.role).dynamicModel ||
        gpsValidateReceiverConfig(GPSType::ublox, config) != Error::None) {
        return EXIT_FAILURE;
    }

    config.headingOffsetRadians = 0.0f;
    return gpsValidateReceiverConfig(GPSType::ublox, config) == Error::UnsupportedHeadingOffset ? EXIT_SUCCESS
                                                                                                : EXIT_FAILURE;
}
