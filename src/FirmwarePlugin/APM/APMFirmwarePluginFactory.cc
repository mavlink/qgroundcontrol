/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMFirmwarePluginFactory.h"
#include "ArduCopterFirmwarePlugin.h"
#include "ArduPlaneFirmwarePlugin.h"
#include "ArduRoverFirmwarePlugin.h"
#include "ArduSubFirmwarePlugin.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(APMFirmwarePluginFactoryLog, "qgc.firmwareplugin.apmfirmwarepluginfactory");

APMFirmwarePluginFactory APMFirmwarePluginFactory(nullptr);

APMFirmwarePluginFactory::APMFirmwarePluginFactory(QObject *parent)
{
    // qCDebug(FirmwarePluginFactoryLog) << Q_FUNC_INFO << this;
}

APMFirmwarePluginFactory::~APMFirmwarePluginFactory()
{
    // qCDebug(FirmwarePluginFactoryLog) << Q_FUNC_INFO << this;
}

QList<QGCMAVLink::FirmwareClass_t> APMFirmwarePluginFactory::supportedFirmwareClasses() const
{
    QList<QGCMAVLink::FirmwareClass_t> list;
    list.append(QGCMAVLink::FirmwareClassArduPilot);
    return list;
}

FirmwarePlugin *APMFirmwarePluginFactory::firmwarePluginForAutopilot(MAV_AUTOPILOT autopilotType, MAV_TYPE vehicleType)
{
    if (autopilotType == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        switch (vehicleType) {
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
            if (!_arduCopterPluginInstance) {
                _arduCopterPluginInstance = new ArduCopterFirmwarePlugin(this);
            }
            return _arduCopterPluginInstance;
        case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
        case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_FIXEDROTOR:
        case MAV_TYPE_VTOL_TAILSITTER:
        case MAV_TYPE_VTOL_TILTWING:
        case MAV_TYPE_VTOL_RESERVED5:
        case MAV_TYPE_FIXED_WING:
            if (!_arduPlanePluginInstance) {
                _arduPlanePluginInstance = new ArduPlaneFirmwarePlugin;
            }
            return _arduPlanePluginInstance;
        case MAV_TYPE_GROUND_ROVER:
        case MAV_TYPE_SURFACE_BOAT:
            if (!_arduRoverPluginInstance) {
                _arduRoverPluginInstance = new ArduRoverFirmwarePlugin;
            }
            return _arduRoverPluginInstance;
        case MAV_TYPE_SUBMARINE:
            if (!_arduSubPluginInstance) {
                _arduSubPluginInstance = new ArduSubFirmwarePlugin;
            }
            return _arduSubPluginInstance;
        default:
            break;
        }
    }

    return nullptr;
}
