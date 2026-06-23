#include "VehicleSupports.h"

#include "FirmwarePlugin/FirmwarePlugin.h"
#include "Vehicle.h"

VehicleSupports::VehicleSupports(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
{
    connect(_vehicle, &Vehicle::firmwareTypeChanged, this, &VehicleSupports::terrainFrameChanged);
}

bool VehicleSupports::throttleModeCenterZero() const
{
    return _vehicle->firmwarePlugin()->supportsThrottleModeCenterZero();
}

bool VehicleSupports::negativeThrust() const
{
    return _vehicle->firmwarePlugin()->supportsNegativeThrust(_vehicle);
}

bool VehicleSupports::jsButton() const
{
    return _vehicle->firmwarePlugin()->supportsJSButton();
}

bool VehicleSupports::radio() const
{
    return _vehicle->firmwarePlugin()->supportsRadio();
}

bool VehicleSupports::motorInterference() const
{
    return _vehicle->firmwarePlugin()->supportsMotorInterference();
}

bool VehicleSupports::smartRTL() const
{
    return _vehicle->firmwarePlugin()->supportsSmartRTL();
}

bool VehicleSupports::terrainFrame() const
{
    return !_vehicle->px4Firmware();
}

bool VehicleSupports::guidedMode() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::GuidedModeCapability);
}

bool VehicleSupports::pauseVehicle() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::PauseVehicleCapability);
}

bool VehicleSupports::orbitMode() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::OrbitModeCapability);
}

bool VehicleSupports::roiMode() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::ROIModeCapability);
}

bool VehicleSupports::takeoffMissionCommand() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::TakeoffVehicleCapability);
}

bool VehicleSupports::guidedTakeoffWithAltitude() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::GuidedTakeoffCapability);
}

bool VehicleSupports::guidedTakeoffWithoutAltitude() const
{
    auto* fp = _vehicle->firmwarePlugin();
    return fp->isCapable(_vehicle, FirmwarePlugin::TakeoffVehicleCapability)
        && !fp->isCapable(_vehicle, FirmwarePlugin::GuidedTakeoffCapability);
}

bool VehicleSupports::changeHeading() const
{
    return _vehicle->firmwarePlugin()->isCapable(_vehicle, FirmwarePlugin::ChangeHeadingCapability);
}
