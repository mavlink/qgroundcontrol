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

ArduRoverFirmwarePlugin::ArduRoverFirmwarePlugin(void)
    : _manualFlightMode         (tr("Manual"))
    , _acroFlightMode           (tr("Acro"))
    , _learningFlightMode       (tr("Learning"))
    , _steeringFlightMode       (tr("Steering"))
    , _holdFlightMode           (tr("Hold"))
    , _loiterFlightMode         (tr("Loiter"))
    , _followFlightMode         (tr("Follow"))
    , _simpleFlightMode         (tr("Simple"))
    , _dockFlightMode           (tr("Dock"))
    , _circleFlightMode         (tr("Circle"))
    , _autoFlightMode           (tr("Auto"))
    , _rtlFlightMode            (tr("RTL"))
    , _smartRtlFlightMode       (tr("Smart RTL"))
    , _guidedFlightMode         (tr("Guided"))
    , _initializingFlightMode   (tr("Initializing"))
{
    _setModeEnumToModeStringMapping({
        {APMRoverMode::MANUAL       , _manualFlightMode      },
        {APMRoverMode::ACRO         , _acroFlightMode        },
        {APMRoverMode::LEARNING     , _learningFlightMode    },
        {APMRoverMode::STEERING     , _steeringFlightMode    },
        {APMRoverMode::HOLD         , _holdFlightMode        },
        {APMRoverMode::LOITER       , _loiterFlightMode      },
        {APMRoverMode::FOLLOW       , _followFlightMode      },
        {APMRoverMode::SIMPLE       , _simpleFlightMode      },
        {APMRoverMode::DOCK         , _dockFlightMode        },
        {APMRoverMode::CIRCLE       , _circleFlightMode      },
        {APMRoverMode::AUTO         , _autoFlightMode        },
        {APMRoverMode::RTL          , _rtlFlightMode         },
        {APMRoverMode::SMART_RTL    , _smartRtlFlightMode    },
        {APMRoverMode::GUIDED       , _guidedFlightMode      },
        {APMRoverMode::INITIALIZING , _initializingFlightMode},
    });

    updateAvailableFlightModes({
        // Mode Name              , SM, Custom Mode                CanBeSet  adv    FW      MR
        { _manualFlightMode       , 0 , APMRoverMode::MANUAL       , true , true , false , true},
        { _acroFlightMode         , 0 , APMRoverMode::ACRO         , true , true , false , true},
        { _learningFlightMode     , 0 , APMRoverMode::LEARNING     , true , true , false , true},
        { _steeringFlightMode     , 0 , APMRoverMode::STEERING     , true , true , false , true},
        { _holdFlightMode         , 0 , APMRoverMode::HOLD         , true , true , false , true},
        { _loiterFlightMode       , 0 , APMRoverMode::LOITER       , true , true , false , true},
        { _followFlightMode       , 0 , APMRoverMode::FOLLOW       , true , true , false , true},
        { _simpleFlightMode       , 0 , APMRoverMode::SIMPLE       , true , true , false , true},
        { _dockFlightMode         , 0 , APMRoverMode::DOCK         , true , true , false , true},
        { _circleFlightMode       , 0 , APMRoverMode::CIRCLE       , true , true , false , true},
        { _autoFlightMode         , 0 , APMRoverMode::AUTO         , true , true , false , true},
        { _rtlFlightMode          , 0 , APMRoverMode::RTL          , true , true , false , true},
        { _smartRtlFlightMode     , 0 , APMRoverMode::SMART_RTL    , true , true , false , true},
        { _guidedFlightMode       , 0 , APMRoverMode::GUIDED       , true , true , false , true},
        { _initializingFlightMode , 0 , APMRoverMode::INITIALIZING , true , true , false , true},
    });

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_5 = _remapParamName[3][5];

        remapV3_5["BATT_ARM_VOLT"] =    QStringLiteral("ARMING_VOLT_MIN");
        remapV3_5["BATT2_ARM_VOLT"] =   QStringLiteral("ARMING_VOLT2_MIN");

        _remapParamNameIntialized = true;
    }
}

int ArduRoverFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.5
    return majorVersionNumber == 3 ? 5 : Vehicle::versionNotSetValue;
}

void ArduRoverFirmwarePlugin::guidedModeChangeAltitude(Vehicle* /*vehicle*/, double /*altitudeChange*/, bool /*pauseVehicle*/)
{
    qgcApp()->showAppMessage(QStringLiteral("Change altitude not supported."));
}

bool ArduRoverFirmwarePlugin::supportsNegativeThrust(Vehicle* /*vehicle*/)
{
    return true;
}

void ArduRoverFirmwarePlugin::updateAvailableFlightModes(FlightModeList modeList)
{
    _availableFlightModeList.clear();
    for(auto mode: modeList){
        mode.fixedWing = false;
        mode.multiRotor = true;
        _updateModeMappings(mode);
    }
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
    }
    return UINT32_MAX;
}
