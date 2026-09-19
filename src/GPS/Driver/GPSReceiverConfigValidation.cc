#include "GPSReceiverConfigValidation.h"

#include <QtCore/QCoreApplication>

namespace {
QString diagnostic(GPSReceiverConfigError error)
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSReceiverConfig", text); };
    switch (error) {
        case GPSReceiverConfigError::None:
            return {};
        case GPSReceiverConfigError::UnknownReceiver:
            return tr("Unsupported GPS receiver type");
        case GPSReceiverConfigError::InvalidRole:
            return tr("Unsupported GPS receiver role");
        case GPSReceiverConfigError::InvalidFixedBase:
            return tr("Enter a valid fixed base position and accuracy");
        case GPSReceiverConfigError::InvalidSurveyIn:
            return tr("Enter a valid survey-in accuracy and duration");
        case GPSReceiverConfigError::UnsupportedConstellations:
            return tr("This receiver cannot configure constellations");
        case GPSReceiverConfigError::InvalidConstellations:
            return tr("Unsupported constellation selection");
        case GPSReceiverConfigError::UnsupportedDynamicModel:
            return tr("This receiver role cannot configure a dynamic model");
        case GPSReceiverConfigError::InvalidDynamicModel:
            return tr("Unsupported receiver dynamic model");
        case GPSReceiverConfigError::UnsupportedHeadingOffset:
            return tr("This receiver role cannot configure a heading offset");
        case GPSReceiverConfigError::InvalidHeadingOffset:
            return tr("Enter a finite heading offset between -pi and pi radians");
    }
    return tr("Invalid GPS receiver configuration");
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
