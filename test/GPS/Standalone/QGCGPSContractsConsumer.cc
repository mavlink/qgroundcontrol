#include <QtCore/QCoreApplication>
#include <QtCore/QVariant>

#include "GPSConnectionError.h"
#include "GPSReceiverConfig.h"
#include "GPSType.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    static_assert(static_cast<int>(GPSType::ublox) == 0);
    static_assert(static_cast<int>(GPSType::trimble) == 1);
    static_assert(static_cast<int>(GPSType::septentrio) == 2);
    static_assert(static_cast<int>(GPSType::femto) == 3);
    GPSReceiverConfig config{.base = {.surveyInAccMeters = 0.5, .surveyInDurationSecs = 60},
                             .constellationMask = 2,
                             .dynamicModel = 4,
                             .outputRateHz = 5,
                             .headingOffsetDeg = 12};
    if (!config.validationError().isEmpty() || QVariant::fromValue(config).value<GPSReceiverConfig>() != config) {
        return 1;
    }
    config.base.surveyInDurationSecs = 0;
    if (config.validationError().isEmpty()) {
        return 2;
    }
    const auto error = QVariant::fromValue(GPSConnectionError::ConfigFailed).value<GPSConnectionError>();
    return error == GPSConnectionError::ConfigFailed && static_cast<int>(error) == 2 ? 0 : 3;
}
