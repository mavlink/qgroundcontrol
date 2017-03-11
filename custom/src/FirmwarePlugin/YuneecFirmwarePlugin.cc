/*!
 * @file
 *   @brief Yuneec Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "YuneecFirmwarePlugin.h"
#include "YuneecAutoPilotPlugin.h"
#include "CameraMetaData.h"

const char* YuneecFirmwarePlugin::_simpleFlightMode =   "Smart";
const char* YuneecFirmwarePlugin::_posCtlFlightMode =   "Angle";

QVariantList YuneecFirmwarePlugin::_cameraList;

YuneecFirmwarePlugin::YuneecFirmwarePlugin(void)
{
    //  The following flight modes are renamed:
    //      Simple -> Smart
    //      POSCTL -> Angle

    // Only the following flight modes are user selectable:
    //      Manual
    //      Stablized
    //      Simple
    //      Angle
    //      Mission

    // FIXME: Need clarification between Manual and Stabilized

    for (int i=0; i<_flightModeInfoList.count(); i++) {
        FlightModeInfo_t& info = _flightModeInfoList[i];
        if (info.name == PX4FirmwarePlugin::_simpleFlightMode) {
            info.name = YuneecFirmwarePlugin::_simpleFlightMode;
        } else if (info.name == PX4FirmwarePlugin::_posCtlFlightMode) {
            info.name = YuneecFirmwarePlugin::_posCtlFlightMode;
        } else if (info.name != PX4FirmwarePlugin::_manualFlightMode &&
                   info.name != PX4FirmwarePlugin::_stabilizedFlightMode &&
                   info.name != PX4FirmwarePlugin::_missionFlightMode) {
            // Not a user selectable flight mode
            info.canBeSet = false;
        }
    }
}

AutoPilotPlugin* YuneecFirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new YuneecAutoPilotPlugin(vehicle, vehicle);
}

QString
YuneecFirmwarePlugin::brandImage(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/YuneecBrandImage.png");
}

QString YuneecFirmwarePlugin::vehicleImageOpaque(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/VehicleIndicator.svg");
}

QString YuneecFirmwarePlugin::vehicleImageOutline(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/VehicleIndicator.svg");
}

QString YuneecFirmwarePlugin::vehicleImageCompass(const Vehicle* vehicle) const
{
    Q_UNUSED(vehicle);
    return QStringLiteral("/typhoonh/CompassIndicator.svg");
}

const QVariantList& YuneecFirmwarePlugin::toolBarIndicators(const Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    if(_toolBarIndicators.size() == 0) {
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/MessageIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/typhoonh/YGPSIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/TelemetryRSSIIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/RCRSSIIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/BatteryIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/typhoonh/CameraIndicator.qml")));
        _toolBarIndicators.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/toolbar/ModeIndicator.qml")));
    }
    return _toolBarIndicators;
}

const QVariantList& YuneecFirmwarePlugin::cameraList(const Vehicle* vehicle)
{
    Q_UNUSED(vehicle);

    if (_cameraList.size() == 0) {
        CameraMetaData* metaData;

        metaData = new CameraMetaData(tr("Typhoon H CGO3+"),   // Camera name
                                      6.264,                   // sensorWidth
                                      4.698,                   // sensorHeight
                                      4000,                    // imageWidth
                                      3000,                    // imageHeight
                                      14,                      // focalLength
                                      this);                   // parent
        _cameraList.append(QVariant::fromValue(metaData));
    }

    return _cameraList;
}

bool YuneecFirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    if (vehicle->flightMode() == _posCtlFlightMode) {
        // PosCtl/Angle is supported for Guided
        return true;
    } else {
        return PX4FirmwarePlugin::isGuidedMode(vehicle);
    }
}
