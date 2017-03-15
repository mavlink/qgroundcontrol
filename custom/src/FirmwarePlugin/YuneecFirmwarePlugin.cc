/*!
 * @file
 *   @brief Yuneec Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "YuneecFirmwarePlugin.h"
#include "YuneecAutoPilotPlugin.h"
#include "CameraMetaData.h"

QVariantList YuneecFirmwarePlugin::_cameraList;

YuneecFirmwarePlugin::YuneecFirmwarePlugin(void)
{
    //  The following flight modes are renamed:
    //      Simple -> Smart
    //      Position -> Angle
    //      Return -> RTL

    _posCtlFlightMode = tr("Angle");
    _rtlFlightMode = tr("RTL");
    _simpleFlightMode = tr("Smart");

    // Only the following flight modes are user selectable:
    //      Simple
    //      Position
    //      Return
    //      Mission

    for (int i=0; i<_flightModeInfoList.count(); i++) {
        FlightModeInfo_t& info = _flightModeInfoList[i];

        if (info.name != _simpleFlightMode &&
                info.name != _posCtlFlightMode &&
                info.name != _rtlFlightMode &&
                info.name != _missionFlightMode) {
            // No other flight modes can be set
            info.canBeSet = false;
        }
    }
}

AutoPilotPlugin* YuneecFirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new YuneecAutoPilotPlugin(vehicle, vehicle);
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

        metaData = new CameraMetaData(tr("Typhoon H CGO3+"),    // Camera name
                                      6.264,                    // sensorWidth
                                      4.698,                    // sensorHeight
                                      4000,                     // imageWidth
                                      3000,                     // imageHeight
                                      14,                       // focalLength
                                      true,                     // true: landscape orientation
                                      true,                     // true: camera is fixed orientation
                                      this);                    // parent
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
