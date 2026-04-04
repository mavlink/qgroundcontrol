/****************************************************************************
 *
 * USV Firmware Plugin Factory Implementation - 无人船固件插件工厂实现
 *
 * 同时支持 ArduPilot 和 PX4 固件
 *
 ****************************************************************************/

#include "USVFirmwarePluginFactory.h"
#include "USVFirmwarePlugin.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(USVFirmwarePluginLog)

USVFirmwarePluginFactory::USVFirmwarePluginFactory(QObject *parent)
    : FirmwarePluginFactory(parent)
{
    qCDebug(USVFirmwarePluginLog) << "USVFirmwarePluginFactory created - 支持 ArduPilot 和 PX4";
}

USVFirmwarePluginFactory::~USVFirmwarePluginFactory()
{
}

QList<QGCMAVLink::FirmwareClass_t> USVFirmwarePluginFactory::supportedFirmwareClasses() const
{
    // 同时支持 ArduPilot 和 PX4 固件
    return QList<QGCMAVLink::FirmwareClass_t>({
        QGCMAVLink::FirmwareClassArduPilot,
        QGCMAVLink::FirmwareClassPX4
    });
}

QList<QGCMAVLink::VehicleClass_t> USVFirmwarePluginFactory::supportedVehicleClasses() const
{
    // 仅支持 Rover/Boat 类型
    // 这是 USV 定制构建的核心限制
    return QList<QGCMAVLink::VehicleClass_t>({
        QGCMAVLink::VehicleClassRoverBoat
    });
}

FirmwarePlugin *USVFirmwarePluginFactory::firmwarePluginForAutopilot(
    MAV_AUTOPILOT autopilotType,
    MAV_TYPE vehicleType)
{
    // 仅为 Rover/Boat 类型创建 USV 固件插件
    if (!QGCMAVLink::isRoverBoat(vehicleType)) {
        qCWarning(USVFirmwarePluginLog) << "Unsupported vehicle type:" << vehicleType
                                         << "- USV build only supports Rover/Boat";
        return nullptr;
    }

    // 根据固件类型返回对应的插件
    switch (autopilotType) {
    case MAV_AUTOPILOT_ARDUPILOTMEGA:
        // ArduPilot (ArduRover/ArduBoat)
        if (!_arduPilotPlugin) {
            _arduPilotPlugin = new USVArduPilotFirmwarePlugin(this);
        }
        qCDebug(USVFirmwarePluginLog) << "Using USVArduPilotFirmwarePlugin for ArduPilot Rover/Boat";
        return _arduPilotPlugin;

    case MAV_AUTOPILOT_PX4:
        // PX4 Rover
        if (!_px4Plugin) {
            _px4Plugin = new USVPX4FirmwarePlugin(this);
        }
        qCDebug(USVFirmwarePluginLog) << "Using USVPX4FirmwarePlugin for PX4 Rover";
        return _px4Plugin;

    default:
        // 其他固件类型，尝试使用 ArduPilot 插件作为后备
        qCWarning(USVFirmwarePluginLog) << "Unknown autopilot type:" << autopilotType
                                         << "- falling back to ArduPilot plugin";
        if (!_arduPilotPlugin) {
            _arduPilotPlugin = new USVArduPilotFirmwarePlugin(this);
        }
        return _arduPilotPlugin;
    }
}
