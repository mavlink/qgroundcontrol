#include "ArduRoverFirmwarePlugin.h"
#include "AppMessages.h"
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
        { _learningFlightMode     , APMRoverMode::LEARNING     , false, true},
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
        { _initializingFlightMode , APMRoverMode::INITIALIZING , false, true},
    };
    updateAvailableFlightModes(availableFlightModes);

    if (!_remapParamNameIntialized) {
        // ArduPilot 4.7: parameter renames and SI unit conversion
        FirmwarePlugin::remapParamNameMap_t &remapV4_7 = _remapParamName[4][7];

        // EKF
        remapV4_7["EK3_FLOW_MAX"]    = QStringLiteral("EK3_MAX_FLOW");

        // Common
        remapV4_7["ARMING_SKIPCHK"]  = QStringLiteral("ARMING_CHECK");

        _remapParamNameIntialized = true;
    }
}

ArduRoverFirmwarePlugin::~ArduRoverFirmwarePlugin()
{

}

int ArduRoverFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    return ((majorVersionNumber == 4) ? 7 : Vehicle::versionNotSetValue);
}

void ArduRoverFirmwarePlugin::guidedModeChangeAltitude(Vehicle* /*vehicle*/, double /*altitudeChange*/, bool /*pauseVehicle*/)
{
    QGC::showAppMessage(QStringLiteral("Change altitude not supported."));
}

QString ArduRoverFirmwarePlugin::stabilizedFlightMode() const
{
    return _modeEnumToString.value(APMRoverMode::MANUAL, _manualFlightMode);
}

QString ArduRoverFirmwarePlugin::pauseFlightMode() const
{
    return _modeEnumToString.value(APMRoverMode::HOLD, _holdFlightMode);
}

QString ArduRoverFirmwarePlugin::followFlightMode() const
{
    return _modeEnumToString.value(APMRoverMode::FOLLOW, _followFlightMode);
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
