/*!
 * @file
 *   @brief Auterion Firmware Plugin (PX4)
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "AuterionFirmwarePlugin.h"
#include "AuterionAutoPilotPlugin.h"
#include "AuterionCameraManager.h"
#include "AuterionCameraControl.h"

//-----------------------------------------------------------------------------
AuterionFirmwarePlugin::AuterionFirmwarePlugin()
{
}

//-----------------------------------------------------------------------------
AutoPilotPlugin* AuterionFirmwarePlugin::autopilotPlugin(Vehicle* vehicle)
{
    return new AuterionAutoPilotPlugin(vehicle, vehicle);
}

//-----------------------------------------------------------------------------
QGCCameraManager*
AuterionFirmwarePlugin::createCameraManager(Vehicle *vehicle)
{
    return new AuterionCameraManager(vehicle);
}

//-----------------------------------------------------------------------------
QGCCameraControl*
AuterionFirmwarePlugin::createCameraControl(const mavlink_camera_information_t* info, Vehicle *vehicle, int compID, QObject* parent)
{
    return new AuterionCameraControl(info, vehicle, compID, parent);
}

//-----------------------------------------------------------------------------
const QVariantList&
AuterionFirmwarePlugin::toolBarIndicators(const Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    if(_toolBarIndicatorList.size() == 0) {
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/auterion/AuterionRCRSSIIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/auterion/AuterionGPSIndicator.qml")));
        _toolBarIndicatorList.append(QVariant::fromValue(QUrl::fromUserInput("qrc:/auterion/AuterionBatteryIndicator.qml")));
    }
    return _toolBarIndicatorList;
}

