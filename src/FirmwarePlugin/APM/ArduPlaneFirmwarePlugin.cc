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
        { _initializingFlightMode , APMPlaneMode::INITIALIZING  , false, true },
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
        FirmwarePlugin::remapParamNameMap_t &remapV4_5 = _remapParamName[4][5];

        remapV4_5["AIRSPEED_MIN"] = QStringLiteral("ARSPD_FBW_MIN");
        remapV4_5["AIRSPEED_MAX"] = QStringLiteral("ARSPD_FBW_MAX");
        remapV4_5["RTL_ALTITUDE"] = QStringLiteral("ALT_HOLD_RTL");
        // LAND_SPEED is only used in a Copter component

        // ArduPilot 4.7: QuadPlane parameter renames and SI unit conversion
        FirmwarePlugin::remapParamNameMap_t &remapV4_7 = _remapParamName[4][7];

        // Attitude controller
        remapV4_7["Q_A_ANGLE_MAX"]       = QStringLiteral("Q_ANGLE_MAX");
        remapV4_7["Q_A_ACC_R_MAX"]       = QStringLiteral("Q_A_ACCEL_R_MAX");
        remapV4_7["Q_A_ACC_P_MAX"]       = QStringLiteral("Q_A_ACCEL_P_MAX");
        remapV4_7["Q_A_ACC_Y_MAX"]       = QStringLiteral("Q_A_ACCEL_Y_MAX");
        remapV4_7["Q_A_RATE_WPY_MAX"]    = QStringLiteral("Q_A_SLEW_YAW");

        // Loiter
        remapV4_7["Q_LOIT_SPEED_MS"]     = QStringLiteral("Q_LOIT_SPEED");
        remapV4_7["Q_LOIT_ACC_MAX_M"]    = QStringLiteral("Q_LOIT_ACC_MAX");
        remapV4_7["Q_LOIT_BRK_ACC_M"]    = QStringLiteral("Q_LOIT_BRK_ACCEL");
        remapV4_7["Q_LOIT_BRK_JRK_M"]    = QStringLiteral("Q_LOIT_BRK_JERK");

        // Pilot
        remapV4_7["Q_PILOT_SPD_UP"]      = QStringLiteral("Q_PILOT_SPEED_UP");
        remapV4_7["Q_PILOT_SPD_DN"]      = QStringLiteral("Q_PILOT_SPEED_DN");
        remapV4_7["Q_PILOT_TKO_ALT_M"]   = QStringLiteral("Q_PILOT_TKOFF_ALT");

        // Position controller: Q_P_VELXY_* -> Q_P_NE_VEL_*
        remapV4_7["Q_P_NE_VEL_P"]        = QStringLiteral("Q_P_VELXY_P");
        remapV4_7["Q_P_NE_VEL_I"]        = QStringLiteral("Q_P_VELXY_I");
        remapV4_7["Q_P_NE_VEL_D"]        = QStringLiteral("Q_P_VELXY_D");
        remapV4_7["Q_P_NE_VEL_IMAX"]     = QStringLiteral("Q_P_VELXY_IMAX");
        remapV4_7["Q_P_NE_VEL_FLTE"]     = QStringLiteral("Q_P_VELXY_FLTE");
        remapV4_7["Q_P_NE_VEL_FLTD"]     = QStringLiteral("Q_P_VELXY_FLTD");
        remapV4_7["Q_P_NE_VEL_FF"]       = QStringLiteral("Q_P_VELXY_FF");

        // Position controller: Q_P_VELZ_* -> Q_P_D_VEL_*
        remapV4_7["Q_P_D_VEL_P"]         = QStringLiteral("Q_P_VELZ_P");
        remapV4_7["Q_P_D_VEL_I"]         = QStringLiteral("Q_P_VELZ_I");
        remapV4_7["Q_P_D_VEL_D"]         = QStringLiteral("Q_P_VELZ_D");
        remapV4_7["Q_P_D_VEL_IMAX"]      = QStringLiteral("Q_P_VELZ_IMAX");
        remapV4_7["Q_P_D_VEL_FLTE"]      = QStringLiteral("Q_P_VELZ_FLTE");
        remapV4_7["Q_P_D_VEL_FF"]        = QStringLiteral("Q_P_VELZ_FF");

        // Position controller: Q_P_ACCZ_* -> Q_P_D_ACC_*
        remapV4_7["Q_P_D_ACC_P"]         = QStringLiteral("Q_P_ACCZ_P");
        remapV4_7["Q_P_D_ACC_I"]         = QStringLiteral("Q_P_ACCZ_I");
        remapV4_7["Q_P_D_ACC_D"]         = QStringLiteral("Q_P_ACCZ_D");
        remapV4_7["Q_P_D_ACC_IMAX"]      = QStringLiteral("Q_P_ACCZ_IMAX");
        remapV4_7["Q_P_D_ACC_FLTD"]      = QStringLiteral("Q_P_ACCZ_FLTD");
        remapV4_7["Q_P_D_ACC_FLTE"]      = QStringLiteral("Q_P_ACCZ_FLTE");
        remapV4_7["Q_P_D_ACC_FLTT"]      = QStringLiteral("Q_P_ACCZ_FLTT");
        remapV4_7["Q_P_D_ACC_FF"]        = QStringLiteral("Q_P_ACCZ_FF");
        remapV4_7["Q_P_D_ACC_SMAX"]      = QStringLiteral("Q_P_ACCZ_SMAX");

        // Waypoint navigation
        remapV4_7["Q_WP_ACC"]            = QStringLiteral("Q_WP_ACCEL");
        remapV4_7["Q_WP_ACC_CNR"]        = QStringLiteral("Q_WP_ACCEL_C");
        remapV4_7["Q_WP_ACC_Z"]          = QStringLiteral("Q_WP_ACCEL_Z");
        remapV4_7["Q_WP_RADIUS_M"]       = QStringLiteral("Q_WP_RADIUS");
        remapV4_7["Q_WP_SPD"]            = QStringLiteral("Q_WP_SPEED");
        remapV4_7["Q_WP_SPD_DN"]         = QStringLiteral("Q_WP_SPEED_DN");
        remapV4_7["Q_WP_SPD_UP"]         = QStringLiteral("Q_WP_SPEED_UP");

        // EKF
        remapV4_7["EK3_FLOW_MAX"]        = QStringLiteral("EK3_MAX_FLOW");

        // Common
        remapV4_7["ARMING_SKIPCHK"]      = QStringLiteral("ARMING_CHECK");

        _remapParamNameIntialized = true;
    }
}

ArduPlaneFirmwarePlugin::~ArduPlaneFirmwarePlugin()
{

}

int ArduPlaneFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    return ((majorVersionNumber == 4) ? 7 : Vehicle::versionNotSetValue);
}

QString ArduPlaneFirmwarePlugin::takeOffFlightMode() const
{
    return _modeEnumToString.value(APMPlaneMode::TAKEOFF, _takeoffFlightMode);
}

QString ArduPlaneFirmwarePlugin::stabilizedFlightMode() const
{
    return _modeEnumToString.value(APMPlaneMode::STABILIZE, _stabilizeFlightMode);
}

QString ArduPlaneFirmwarePlugin::pauseFlightMode() const
{
    return _modeEnumToString.value(APMPlaneMode::LOITER, _loiterFlightMode);
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
