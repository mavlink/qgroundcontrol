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

ArduPlaneFirmwarePlugin::ArduPlaneFirmwarePlugin(void)
    : _manualFlightMode      (tr("Manual"))
    , _circleFlightMode      (tr("Circle"))
    , _stabilizeFlightMode   (tr("Stabilize"))
    , _trainingFlightMode    (tr("Training"))
    , _acroFlightMode        (tr("Acro"))
    , _flyByWireAFlightMode  (tr("FBW A"))
    , _flyByWireBFlightMode  (tr("FBW B"))
    , _cruiseFlightMode      (tr("Cruise"))
    , _autoTuneFlightMode    (tr("Autotune"))
    , _autoFlightMode        (tr("Auto"))
    , _rtlFlightMode         (tr("RTL"))
    , _loiterFlightMode      (tr("Loiter"))
    , _takeoffFlightMode     (tr("Takeoff"))
    , _avoidADSBFlightMode   (tr("Avoid ADSB"))
    , _guidedFlightMode      (tr("Guided"))
    , _initializingFlightMode(tr("Initializing"))
    , _qStabilizeFlightMode  (tr("QuadPlane Stabilize"))
    , _qHoverFlightMode      (tr("QuadPlane Hover"))
    , _qLoiterFlightMode     (tr("QuadPlane Loiter"))
    , _qLandFlightMode       (tr("QuadPlane Land"))
    , _qRTLFlightMode        (tr("QuadPlane RTL"))
    , _qAutotuneFlightMode   (tr("QuadPlane AutoTune"))
    , _qAcroFlightMode       (tr("QuadPlane Acro"))
    , _thermalFlightMode     (tr("Thermal"))
{
    _setModeEnumToModeStringMapping({
        {APMPlaneMode::MANUAL        , _manualFlightMode      },
        {APMPlaneMode::CIRCLE        , _circleFlightMode      },
        {APMPlaneMode::STABILIZE     , _stabilizeFlightMode   },
        {APMPlaneMode::TRAINING      , _trainingFlightMode    },
        {APMPlaneMode::ACRO          , _acroFlightMode        },
        {APMPlaneMode::FLY_BY_WIRE_A , _flyByWireAFlightMode  },
        {APMPlaneMode::FLY_BY_WIRE_B , _flyByWireBFlightMode  },
        {APMPlaneMode::CRUISE        , _cruiseFlightMode      },
        {APMPlaneMode::AUTOTUNE      , _autoTuneFlightMode    },
        {APMPlaneMode::AUTO          , _autoFlightMode        },
        {APMPlaneMode::RTL           , _rtlFlightMode         },
        {APMPlaneMode::LOITER        , _loiterFlightMode      },
        {APMPlaneMode::TAKEOFF       , _takeoffFlightMode     },
        {APMPlaneMode::AVOID_ADSB    , _avoidADSBFlightMode   },
        {APMPlaneMode::GUIDED        , _guidedFlightMode      },
        {APMPlaneMode::INITIALIZING  , _initializingFlightMode},
        {APMPlaneMode::QSTABILIZE    , _qStabilizeFlightMode  },
        {APMPlaneMode::QHOVER        , _qHoverFlightMode      },
        {APMPlaneMode::QLOITER       , _qLoiterFlightMode     },
        {APMPlaneMode::QLAND         , _qLandFlightMode       },
        {APMPlaneMode::QRTL          , _qRTLFlightMode        },
        {APMPlaneMode::QAUTOTUNE     , _qAutotuneFlightMode   },
        {APMPlaneMode::QACRO         , _qAcroFlightMode       },
        {APMPlaneMode::THERMAL       , _thermalFlightMode     },
    });

    updateAvailableFlightModes({
        // Mode Name             ,  SM, Custom Mode                CanBeSet  adv    FW      MR
        { _manualFlightMode       , 0 , APMPlaneMode::MANUAL        , true , true , true , true},
        { _circleFlightMode       , 0 , APMPlaneMode::CIRCLE        , true , true , true , true},
        { _stabilizeFlightMode    , 0 , APMPlaneMode::STABILIZE     , true , true , true , true},
        { _trainingFlightMode     , 0 , APMPlaneMode::TRAINING      , true , true , true , true},
        { _acroFlightMode         , 0 , APMPlaneMode::ACRO          , true , true , true , true},
        { _flyByWireAFlightMode   , 0 , APMPlaneMode::FLY_BY_WIRE_A , true , true , true , true},
        { _flyByWireBFlightMode   , 0 , APMPlaneMode::FLY_BY_WIRE_B , true , true , true , true},
        { _cruiseFlightMode       , 0 , APMPlaneMode::CRUISE        , true , true , true , true},
        { _autoTuneFlightMode     , 0 , APMPlaneMode::AUTOTUNE      , true , true , true , true},
        { _autoFlightMode         , 0 , APMPlaneMode::AUTO          , true , true , true , true},
        { _rtlFlightMode          , 0 , APMPlaneMode::RTL           , true , true , true , true},
        { _loiterFlightMode       , 0 , APMPlaneMode::LOITER        , true , true , true , true},
        { _takeoffFlightMode      , 0 , APMPlaneMode::TAKEOFF       , true , true , true , true},
        { _avoidADSBFlightMode    , 0 , APMPlaneMode::AVOID_ADSB    , true , true , true , true},
        { _guidedFlightMode       , 0 , APMPlaneMode::GUIDED        , true , true , true , true},
        { _initializingFlightMode , 0 , APMPlaneMode::INITIALIZING  , true , true , true , true},
        { _qStabilizeFlightMode   , 0 , APMPlaneMode::QSTABILIZE    , true , true , true , true},
        { _qHoverFlightMode       , 0 , APMPlaneMode::QHOVER        , true , true , true , true},
        { _qLoiterFlightMode      , 0 , APMPlaneMode::QLOITER       , true , true , true , true},
        { _qLandFlightMode        , 0 , APMPlaneMode::QLAND         , true , true , true , true},
        { _qRTLFlightMode         , 0 , APMPlaneMode::QRTL          , true , true , true , true},
        { _qAutotuneFlightMode    , 0 , APMPlaneMode::QAUTOTUNE     , true , true , true , true},
        { _qAcroFlightMode        , 0 , APMPlaneMode::QACRO         , true , true , true , true},
        { _thermalFlightMode      , 0 , APMPlaneMode::THERMAL       , true , true , true , true},
    });

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_10 = _remapParamName[3][10];

        remapV3_10["BATT_ARM_VOLT"] =    QStringLiteral("ARMING_VOLT_MIN");
        remapV3_10["BATT2_ARM_VOLT"] =   QStringLiteral("ARMING_VOLT2_MIN");

        _remapParamNameIntialized = true;
    }
}

int ArduPlaneFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.10
    return majorVersionNumber == 3 ? 10 : Vehicle::versionNotSetValue;
}

void ArduPlaneFirmwarePlugin::updateAvailableFlightModes(FlightModeList modeList)
{
    _availableFlightModeList.clear();
    for(auto mode: modeList){
        mode.fixedWing = true;
        mode.multiRotor = true;
        _updateModeMappings(mode);
    }
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
    }
    return UINT32_MAX;
}
