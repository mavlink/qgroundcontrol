/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ArduPlaneFirmwarePlugin.h"
#include "Vehicle.h"

bool ArduPlaneFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduPlaneFirmwarePlugin::_remapParamName;

ArduPlaneFirmwarePlugin::ArduPlaneFirmwarePlugin(QObject *parent)
    : APMFirmwarePlugin(parent)
{
    _setModeEnumToModeStringMapping({
        { APMPlaneMode::MANUAL        , _manualFlightMode       },
        { APMPlaneMode::CIRCLE        , _circleFlightMode       },
        { APMPlaneMode::STABILIZE     , _stabilizeFlightMode    },
        { APMPlaneMode::TRAINING      , _trainingFlightMode     },
        { APMPlaneMode::ACRO          , _acroFlightMode         },
        { APMPlaneMode::FLY_BY_WIRE_A , _flyByWireAFlightMode   },
        { APMPlaneMode::FLY_BY_WIRE_B , _flyByWireBFlightMode   },
        { APMPlaneMode::CRUISE        , _cruiseFlightMode       },
        { APMPlaneMode::AUTOTUNE      , _autoTuneFlightMode     },
        { APMPlaneMode::AUTO          , _autoFlightMode         },
        { APMPlaneMode::RTL           , _rtlFlightMode          },
        { APMPlaneMode::LOITER        , _loiterFlightMode       },
        { APMPlaneMode::TAKEOFF       , _takeoffFlightMode      },
        { APMPlaneMode::AVOID_ADSB    , _avoidADSBFlightMode    },
        { APMPlaneMode::GUIDED        , _guidedFlightMode       },
        { APMPlaneMode::INITIALIZING  , _initializingFlightMode },
        { APMPlaneMode::QSTABILIZE    , _qStabilizeFlightMode   },
        { APMPlaneMode::QHOVER        , _qHoverFlightMode       },
        { APMPlaneMode::QLOITER       , _qLoiterFlightMode      },
        { APMPlaneMode::QLAND         , _qLandFlightMode        },
        { APMPlaneMode::QRTL          , _qRTLFlightMode         },
        { APMPlaneMode::QAUTOTUNE     , _qAutotuneFlightMode    },
        { APMPlaneMode::QACRO         , _qAcroFlightMode        },
        { APMPlaneMode::THERMAL       , _thermalFlightMode      },
        { APMPlaneMode::LOITER2QLAND  , _loiter2qlandFlightMode },
        { APMPlaneMode::AUTOLAND      , _autolandFlightMode     },
        
    });

    static FlightModeList availableFlightModes = {
         // Mode Name              , Custom Mode                CanBeSet  adv
        { _manualFlightMode       , APMPlaneMode::MANUAL        , true , true },
        { _circleFlightMode       , APMPlaneMode::CIRCLE        , true , true },
        { _stabilizeFlightMode    , APMPlaneMode::STABILIZE     , true , true },
        { _trainingFlightMode     , APMPlaneMode::TRAINING      , true , true },
        { _acroFlightMode         , APMPlaneMode::ACRO          , true , true },
        { _flyByWireAFlightMode   , APMPlaneMode::FLY_BY_WIRE_A , true , true },
        { _flyByWireBFlightMode   , APMPlaneMode::FLY_BY_WIRE_B , true , true },
        { _cruiseFlightMode       , APMPlaneMode::CRUISE        , true , true },
        { _autoTuneFlightMode     , APMPlaneMode::AUTOTUNE      , true , true },
        { _autoFlightMode         , APMPlaneMode::AUTO          , true , true },
        { _rtlFlightMode          , APMPlaneMode::RTL           , true , true },
        { _loiterFlightMode       , APMPlaneMode::LOITER        , true , true },
        { _takeoffFlightMode      , APMPlaneMode::TAKEOFF       , true , true },
        { _avoidADSBFlightMode    , APMPlaneMode::AVOID_ADSB    , true , true },
        { _guidedFlightMode       , APMPlaneMode::GUIDED        , true , true },
        { _initializingFlightMode , APMPlaneMode::INITIALIZING  , true , true },
        { _qStabilizeFlightMode   , APMPlaneMode::QSTABILIZE    , true , true },
        { _qHoverFlightMode       , APMPlaneMode::QHOVER        , true , true },
        { _qLoiterFlightMode      , APMPlaneMode::QLOITER       , true , true },
        { _qLandFlightMode        , APMPlaneMode::QLAND         , true , true },
        { _qRTLFlightMode         , APMPlaneMode::QRTL          , true , true },
        { _qAutotuneFlightMode    , APMPlaneMode::QAUTOTUNE     , true , true },
        { _qAcroFlightMode        , APMPlaneMode::QACRO         , true , true },
        { _thermalFlightMode      , APMPlaneMode::THERMAL       , true , true },
        { _loiter2qlandFlightMode , APMPlaneMode::LOITER2QLAND  , true , true },
        { _autolandFlightMode     , APMPlaneMode::AUTOLAND      , true , true },
    };
    updateAvailableFlightModes(availableFlightModes);

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t &remapV3_10 = _remapParamName[3][10];

        remapV3_10["BATT_ARM_VOLT"] = QStringLiteral("ARMING_VOLT_MIN");
        remapV3_10["BATT2_ARM_VOLT"] = QStringLiteral("ARMING_VOLT2_MIN");

        FirmwarePlugin::remapParamNameMap_t &remapV4_5 = _remapParamName[4][5];

        remapV4_5["AIRSPEED_MIN"] = QStringLiteral("ARSPD_FBW_MIN");
        remapV4_5["AIRSPEED_MAX"] = QStringLiteral("ARSPD_FBW_MAX");

        _remapParamNameIntialized = true;
    }
}

ArduPlaneFirmwarePlugin::~ArduPlaneFirmwarePlugin()
{

}

int ArduPlaneFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 4.5
    return ((majorVersionNumber == 4) ? 5 : Vehicle::versionNotSetValue);
}

QString ArduPlaneFirmwarePlugin::stabilizedFlightMode() const
{
    return _modeEnumToString.value(APMPlaneMode::STABILIZE, _stabilizeFlightMode);
}

void ArduPlaneFirmwarePlugin::updateAvailableFlightModes(FlightModeList &modeList)
{
    for (FirmwareFlightMode &mode: modeList) {
        mode.fixedWing = true;
        mode.multiRotor = true;
    }

    _updateFlightModeList(modeList);
}

uint32_t ArduPlaneFirmwarePlugin::_convertToCustomFlightModeEnum(uint32_t val) const
{
    switch (val) {
    case APMCustomMode::AUTO:
        return APMPlaneMode::AUTO;
    case APMCustomMode::GUIDED:
        return APMPlaneMode::GUIDED;
    case APMCustomMode::RTL:
        return APMPlaneMode::RTL;
    case APMCustomMode::SMART_RTL:
        return APMPlaneMode::RTL;
    default:
        return UINT32_MAX;
    }
}
