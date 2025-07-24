/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ArduRoverFirmwarePlugin.h"
#include "QGCApplication.h"
#include "Vehicle.h"

bool ArduRoverFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduRoverFirmwarePlugin::_remapParamName;

ArduRoverFirmwarePlugin::ArduRoverFirmwarePlugin(QObject *parent)
    : APMFirmwarePlugin(parent)
{
    _setModeEnumToModeStringMapping({
        { APMRoverMode::MANUAL       , _manualFlightMode       },
        { APMRoverMode::ACRO         , _acroFlightMode         },
        { APMRoverMode::LEARNING     , _learningFlightMode     },
        { APMRoverMode::STEERING     , _steeringFlightMode     },
        { APMRoverMode::HOLD         , _holdFlightMode         },
        { APMRoverMode::LOITER       , _loiterFlightMode       },
        { APMRoverMode::FOLLOW       , _followFlightMode       },
        { APMRoverMode::SIMPLE       , _simpleFlightMode       },
        { APMRoverMode::DOCK         , _dockFlightMode         },
        { APMRoverMode::CIRCLE       , _circleFlightMode       },
        { APMRoverMode::AUTO         , _autoFlightMode         },
        { APMRoverMode::RTL          , _rtlFlightMode          },
        { APMRoverMode::SMART_RTL    , _smartRtlFlightMode     },
        { APMRoverMode::GUIDED       , _guidedFlightMode       },
        { APMRoverMode::INITIALIZING , _initializingFlightMode },
    });

    static FlightModeList availableFlightModes = {
        // Mode Name              , Custom Mode                CanBeSet  adv
        { _manualFlightMode       , APMRoverMode::MANUAL       , true , true},
        { _acroFlightMode         , APMRoverMode::ACRO         , true , true},
        { _learningFlightMode     , APMRoverMode::LEARNING     , true , true},
        { _steeringFlightMode     , APMRoverMode::STEERING     , true , true},
        { _holdFlightMode         , APMRoverMode::HOLD         , true , true},
        { _loiterFlightMode       , APMRoverMode::LOITER       , true , true},
        { _followFlightMode       , APMRoverMode::FOLLOW       , true , true},
        { _simpleFlightMode       , APMRoverMode::SIMPLE       , true , true},
        { _dockFlightMode         , APMRoverMode::DOCK         , true , true},
        { _circleFlightMode       , APMRoverMode::CIRCLE       , true , true},
        { _autoFlightMode         , APMRoverMode::AUTO         , true , true},
        { _rtlFlightMode          , APMRoverMode::RTL          , true , true},
        { _smartRtlFlightMode     , APMRoverMode::SMART_RTL    , true , true},
        { _guidedFlightMode       , APMRoverMode::GUIDED       , true , true},
        { _initializingFlightMode , APMRoverMode::INITIALIZING , true , true},
    };
    updateAvailableFlightModes(availableFlightModes);

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_5 = _remapParamName[3][5];

        remapV3_5["BATT_ARM_VOLT"] =    QStringLiteral("ARMING_VOLT_MIN");
        remapV3_5["BATT2_ARM_VOLT"] =   QStringLiteral("ARMING_VOLT2_MIN");

        _remapParamNameIntialized = true;
    }
}

ArduRoverFirmwarePlugin::~ArduRoverFirmwarePlugin()
{

}

int ArduRoverFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.5
    return ((majorVersionNumber == 3) ? 5 : Vehicle::versionNotSetValue);
}

void ArduRoverFirmwarePlugin::guidedModeChangeAltitude(Vehicle* /*vehicle*/, double /*altitudeChange*/, bool /*pauseVehicle*/)
{
    qgcApp()->showAppMessage(QStringLiteral("Change altitude not supported."));
}

QString ArduRoverFirmwarePlugin::stabilizedFlightMode() const
{
    return _modeEnumToString.value(APMRoverMode::MANUAL, _manualFlightMode);
}

void ArduRoverFirmwarePlugin::updateAvailableFlightModes(FlightModeList &modeList)
{
    for (FirmwareFlightMode &mode: modeList) {
        mode.fixedWing = false;
        mode.multiRotor = true;
    }

    _updateFlightModeList(modeList);
}

uint32_t ArduRoverFirmwarePlugin::_convertToCustomFlightModeEnum(uint32_t val) const
{
    switch (val) {
    case APMCustomMode::AUTO:
        return APMRoverMode::AUTO;
    case APMCustomMode::GUIDED:
        return APMRoverMode::GUIDED;
    case APMCustomMode::RTL:
        return APMRoverMode::RTL;
    case APMCustomMode::SMART_RTL:
        return APMRoverMode::SMART_RTL;
    default:
        return UINT32_MAX;
    }
}
