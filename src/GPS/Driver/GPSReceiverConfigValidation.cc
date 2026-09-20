#include "GPSReceiverConfigValidation.h"

#include <QtCore/QCoreApplication>

namespace {
QString diagnostic(GPSReceiverConfigError error)
{
    switch (error) {
        case GPSReceiverConfigError::None:
            return {};
        case GPSReceiverConfigError::UnknownReceiver:
            return QCoreApplication::translate("GPSReceiverConfig", "Unsupported GPS receiver type");
        case GPSReceiverConfigError::InvalidRole:
            return QCoreApplication::translate("GPSReceiverConfig", "Unsupported GPS receiver role");
        case GPSReceiverConfigError::UnsupportedRole:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This receiver does not support the requested role");
        case GPSReceiverConfigError::InvalidFixedBase:
            return QCoreApplication::translate("GPSReceiverConfig", "Enter a valid fixed base position and accuracy");
        case GPSReceiverConfigError::InvalidSurveyIn:
            return QCoreApplication::translate("GPSReceiverConfig", "Enter a valid survey-in accuracy and duration");
        case GPSReceiverConfigError::UnsupportedBaseMode:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This receiver does not support the selected base mode");
        case GPSReceiverConfigError::InvalidReceiverAveraging:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "Enter a receiver averaging duration between 1 and 3600 seconds");
        case GPSReceiverConfigError::InvalidBaudRate:
            return QCoreApplication::translate(
                "GPSReceiverConfig", "Select a valid serial baud rate; passive input requires an explicit rate");
        case GPSReceiverConfigError::UnsupportedPersistentConfiguration:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This driver does not support persistent receiver configuration");
        case GPSReceiverConfigError::UnsupportedConstellations:
            return QCoreApplication::translate("GPSReceiverConfig", "This receiver cannot configure constellations");
        case GPSReceiverConfigError::InvalidConstellations:
            return QCoreApplication::translate("GPSReceiverConfig", "Unsupported constellation selection");
        case GPSReceiverConfigError::UnsupportedDynamicModel:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This receiver role cannot configure a dynamic model");
        case GPSReceiverConfigError::InvalidDynamicModel:
            return QCoreApplication::translate("GPSReceiverConfig", "Unsupported receiver dynamic model");
        case GPSReceiverConfigError::UnsupportedHeadingOffset:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "This receiver role cannot configure a heading offset");
        case GPSReceiverConfigError::InvalidHeadingOffset:
            return QCoreApplication::translate("GPSReceiverConfig",
                                               "Enter a finite heading offset between -pi and pi radians");
    }
    return QCoreApplication::translate("GPSReceiverConfig", "Invalid GPS receiver configuration");
}
}  // namespace

QString gpsBaseStationConfigError(const GPSBaseStationConfig& config)
{
    return diagnostic(gpsValidateBaseStationConfig(config));
}

QString gpsReceiverConfigError(GPSType type, const GPSReceiverConfig& config)
{
    return diagnostic(gpsValidateReceiverConfig(type, config));
}
