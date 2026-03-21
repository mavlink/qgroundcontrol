#include "ArduCopterFirmwarePlugin.h"
#include "ParameterManager.h"
#include "Vehicle.h"

bool ArduCopterFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduCopterFirmwarePlugin::_remapParamName;

ArduCopterFirmwarePlugin::ArduCopterFirmwarePlugin(QObject *parent)
    : APMFirmwarePlugin(parent)
{
    _setModeEnumToModeStringMapping({
        { APMCopterMode::STABILIZE,    _stabilizeFlightMode     },
        { APMCopterMode::ACRO,         _acroFlightMode          },
        { APMCopterMode::ALT_HOLD,     _altHoldFlightMode       },
        { APMCopterMode::AUTO,         _autoFlightMode          },
        { APMCopterMode::GUIDED,       _guidedFlightMode        },
        { APMCopterMode::LOITER,       _loiterFlightMode        },
        { APMCopterMode::RTL,          _rtlFlightMode           },
        { APMCopterMode::CIRCLE,       _circleFlightMode        },
        { APMCopterMode::LAND,         _landFlightMode          },
        { APMCopterMode::DRIFT,        _driftFlightMode         },
        { APMCopterMode::SPORT,        _sportFlightMode         },
        { APMCopterMode::FLIP,         _flipFlightMode          },
        { APMCopterMode::AUTOTUNE,     _autotuneFlightMode      },
        { APMCopterMode::POS_HOLD,     _posHoldFlightMode       },
        { APMCopterMode::BRAKE,        _brakeFlightMode         },
        { APMCopterMode::THROW,        _throwFlightMode         },
        { APMCopterMode::AVOID_ADSB,   _avoidADSBFlightMode     },
        { APMCopterMode::GUIDED_NOGPS, _guidedNoGPSFlightMode   },
        { APMCopterMode::SMART_RTL,    _smartRtlFlightMode      },
        { APMCopterMode::FLOWHOLD,     _flowHoldFlightMode      },
        { APMCopterMode::FOLLOW,       _followFlightMode        },
        { APMCopterMode::ZIGZAG,       _zigzagFlightMode        },
        { APMCopterMode::SYSTEMID,     _systemIDFlightMode      },
        { APMCopterMode::AUTOROTATE,   _autoRotateFlightMode    },
        { APMCopterMode::AUTO_RTL,     _autoRTLFlightMode       },
        { APMCopterMode::TURTLE,       _turtleFlightMode        },
    });

    static FlightModeList availableFlightModes = {
        // Mode Name             , Custom Mode                CanBeSet  adv
        { _stabilizeFlightMode   , APMCopterMode::STABILIZE,     true , true },
        { _acroFlightMode        , APMCopterMode::ACRO,          true , true },
        { _altHoldFlightMode     , APMCopterMode::ALT_HOLD,      true , true },
        { _autoFlightMode        , APMCopterMode::AUTO,          true , true },
        { _guidedFlightMode      , APMCopterMode::GUIDED,        true , true },
        { _loiterFlightMode      , APMCopterMode::LOITER,        true , true },
        { _rtlFlightMode         , APMCopterMode::RTL,           true , true },
        { _circleFlightMode      , APMCopterMode::CIRCLE,        true , true },
        { _landFlightMode        , APMCopterMode::LAND,          true , true },
        { _driftFlightMode       , APMCopterMode::DRIFT,         true , true },
        { _sportFlightMode       , APMCopterMode::SPORT,         true , true },
        { _flipFlightMode        , APMCopterMode::FLIP,          true , true },
        { _autotuneFlightMode    , APMCopterMode::AUTOTUNE,      true , true },
        { _posHoldFlightMode     , APMCopterMode::POS_HOLD,      true , true },
        { _brakeFlightMode       , APMCopterMode::BRAKE,         true , true },
        { _throwFlightMode       , APMCopterMode::THROW,         true , true },
        { _avoidADSBFlightMode   , APMCopterMode::AVOID_ADSB,    true , true },
        { _guidedNoGPSFlightMode , APMCopterMode::GUIDED_NOGPS,  true , true },
        { _smartRtlFlightMode    , APMCopterMode::SMART_RTL,     true , true },
        { _flowHoldFlightMode    , APMCopterMode::FLOWHOLD,      true , true },
        { _followFlightMode      , APMCopterMode::FOLLOW,        true , true },
        { _zigzagFlightMode      , APMCopterMode::ZIGZAG,        true , true },
        { _systemIDFlightMode    , APMCopterMode::SYSTEMID,      true , true },
        { _autoRotateFlightMode  , APMCopterMode::AUTOROTATE,    true , true },
        { _autoRTLFlightMode     , APMCopterMode::AUTO_RTL,      true , true },
        { _turtleFlightMode      , APMCopterMode::TURTLE,        true , true },
    };
    updateAvailableFlightModes(availableFlightModes);

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t &remapV4_0 = _remapParamName[4][0];

        remapV4_0["TUNE_MIN"] = QStringLiteral("TUNE_LOW");
        remapV4_0["TUNE_MAX"] = QStringLiteral("TUNE_HIGH");

        // ArduPilot 4.7: massive parameter rename and SI unit conversion
        FirmwarePlugin::remapParamNameMap_t &remapV4_7 = _remapParamName[4][7];

        // Position controller: PSC_VELXY_* -> PSC_NE_VEL_*
        remapV4_7["PSC_NE_VEL_P"]    = QStringLiteral("PSC_VELXY_P");
        remapV4_7["PSC_NE_VEL_I"]    = QStringLiteral("PSC_VELXY_I");
        remapV4_7["PSC_NE_VEL_D"]    = QStringLiteral("PSC_VELXY_D");
        remapV4_7["PSC_NE_VEL_IMAX"] = QStringLiteral("PSC_VELXY_IMAX");
        remapV4_7["PSC_NE_VEL_FLTE"] = QStringLiteral("PSC_VELXY_FLTE");
        remapV4_7["PSC_NE_VEL_FLTD"] = QStringLiteral("PSC_VELXY_FLTD");
        remapV4_7["PSC_NE_VEL_FF"]   = QStringLiteral("PSC_VELXY_FF");

        // Position controller: PSC_VELZ_* -> PSC_D_VEL_*
        remapV4_7["PSC_D_VEL_P"]     = QStringLiteral("PSC_VELZ_P");
        remapV4_7["PSC_D_VEL_I"]     = QStringLiteral("PSC_VELZ_I");
        remapV4_7["PSC_D_VEL_D"]     = QStringLiteral("PSC_VELZ_D");
        remapV4_7["PSC_D_VEL_IMAX"]  = QStringLiteral("PSC_VELZ_IMAX");
        remapV4_7["PSC_D_VEL_FLTE"]  = QStringLiteral("PSC_VELZ_FLTE");
        remapV4_7["PSC_D_VEL_FF"]    = QStringLiteral("PSC_VELZ_FF");

        // Position controller: PSC_ACCZ_* -> PSC_D_ACC_*
        remapV4_7["PSC_D_ACC_P"]     = QStringLiteral("PSC_ACCZ_P");
        remapV4_7["PSC_D_ACC_I"]     = QStringLiteral("PSC_ACCZ_I");
        remapV4_7["PSC_D_ACC_D"]     = QStringLiteral("PSC_ACCZ_D");
        remapV4_7["PSC_D_ACC_IMAX"]  = QStringLiteral("PSC_ACCZ_IMAX");
        remapV4_7["PSC_D_ACC_FLTD"]  = QStringLiteral("PSC_ACCZ_FLTD");
        remapV4_7["PSC_D_ACC_FLTE"]  = QStringLiteral("PSC_ACCZ_FLTE");
        remapV4_7["PSC_D_ACC_FLTT"]  = QStringLiteral("PSC_ACCZ_FLTT");
        remapV4_7["PSC_D_ACC_FF"]    = QStringLiteral("PSC_ACCZ_FF");
        remapV4_7["PSC_D_ACC_SMAX"]  = QStringLiteral("PSC_ACCZ_SMAX");

        // Position controller: PSC_POSXY_P -> PSC_NE_POS_P (simply renamed)
        remapV4_7["PSC_NE_POS_P"]    = QStringLiteral("PSC_POSXY_P");

        // Position controller: PSC_POSZ_P -> PSC_D_POS_P (simply renamed)
        remapV4_7["PSC_D_POS_P"]     = QStringLiteral("PSC_POSZ_P");

        // Waypoint navigation: WPNAV_* -> WP_*
        remapV4_7["WP_ACC"]          = QStringLiteral("WPNAV_ACCEL");
        remapV4_7["WP_ACC_CNR"]      = QStringLiteral("WPNAV_ACCEL_C");
        remapV4_7["WP_ACC_Z"]        = QStringLiteral("WPNAV_ACCEL_Z");
        remapV4_7["WP_RADIUS_M"]     = QStringLiteral("WPNAV_RADIUS");
        remapV4_7["WP_SPD"]          = QStringLiteral("WPNAV_SPEED");
        remapV4_7["WP_SPD_DN"]       = QStringLiteral("WPNAV_SPEED_DN");
        remapV4_7["WP_SPD_UP"]       = QStringLiteral("WPNAV_SPEED_UP");

        // RTL parameters
        remapV4_7["RTL_ALT_M"]       = QStringLiteral("RTL_ALT");
        remapV4_7["RTL_SPEED_MS"]    = QStringLiteral("RTL_SPEED");
        remapV4_7["RTL_ALT_FINAL_M"] = QStringLiteral("RTL_ALT_FINAL");
        remapV4_7["RTL_CLIMB_MIN_M"] = QStringLiteral("RTL_CLIMB_MIN");

        // Landing parameters
        remapV4_7["LAND_SPD_MS"]     = QStringLiteral("LAND_SPEED");
        remapV4_7["LAND_SPD_HIGH_MS"]= QStringLiteral("LAND_SPEED_HIGH");
        remapV4_7["LAND_ALT_LOW_M"]  = QStringLiteral("LAND_ALT_LOW");

        // Loiter parameters
        remapV4_7["LOIT_SPEED_MS"]   = QStringLiteral("LOIT_SPEED");
        remapV4_7["LOIT_ACC_MAX_M"]  = QStringLiteral("LOIT_ACC_MAX");
        remapV4_7["LOIT_BRK_ACC_M"]  = QStringLiteral("LOIT_BRK_ACCEL");
        remapV4_7["LOIT_BRK_JRK_M"] = QStringLiteral("LOIT_BRK_JERK");

        // Pilot parameters
        remapV4_7["PILOT_ACC_Z"]     = QStringLiteral("PILOT_ACCEL_Z");
        remapV4_7["PILOT_SPD_UP"]    = QStringLiteral("PILOT_SPEED_UP");
        remapV4_7["PILOT_SPD_DN"]    = QStringLiteral("PILOT_SPEED_DN");
        remapV4_7["PILOT_TKO_ALT_M"] = QStringLiteral("PILOT_TKOFF_ALT");

        // Attitude controller
        remapV4_7["ATC_ANGLE_MAX"]   = QStringLiteral("ANGLE_MAX");
        remapV4_7["ATC_ACC_R_MAX"]   = QStringLiteral("ATC_ACCEL_R_MAX");
        remapV4_7["ATC_ACC_P_MAX"]   = QStringLiteral("ATC_ACCEL_P_MAX");
        remapV4_7["ATC_ACC_Y_MAX"]   = QStringLiteral("ATC_ACCEL_Y_MAX");
        remapV4_7["ATC_RATE_WPY_MAX"]= QStringLiteral("ATC_SLEW_YAW");

        // Circle
        remapV4_7["CIRCLE_RADIUS_M"] = QStringLiteral("CIRCLE_RADIUS");

        // PosHold
        remapV4_7["PHLD_BRK_ANGLE"]  = QStringLiteral("PHLD_BRAKE_ANGLE");
        remapV4_7["PHLD_BRK_RATE"]   = QStringLiteral("PHLD_BRAKE_RATE");

        // EKF
        remapV4_7["EK3_FLOW_MAX"]    = QStringLiteral("EK3_MAX_FLOW");

        _remapParamNameIntialized = true;
    }
}

ArduCopterFirmwarePlugin::~ArduCopterFirmwarePlugin()
{

}

int ArduCopterFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    return ((majorVersionNumber == 4) ? 7 : Vehicle::versionNotSetValue);
}

bool ArduCopterFirmwarePlugin::multiRotorXConfig(Vehicle *vehicle) const
{
    return (vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "FRAME")->rawValue().toInt() != 0);
}

QString ArduCopterFirmwarePlugin::pauseFlightMode() const
{
    return _modeEnumToString.value(APMCopterMode::BRAKE, _brakeFlightMode);
}

QString ArduCopterFirmwarePlugin::landFlightMode() const
{
    return _modeEnumToString.value(APMCopterMode::LAND, _landFlightMode);
}

QString ArduCopterFirmwarePlugin::takeControlFlightMode() const
{
    return _modeEnumToString.value(APMCopterMode::LOITER, _loiterFlightMode);
}

QString ArduCopterFirmwarePlugin::followFlightMode() const
{
    return _modeEnumToString.value(APMCopterMode::FOLLOW, _followFlightMode);
}

QString ArduCopterFirmwarePlugin::stabilizedFlightMode() const
{
    return _modeEnumToString.value(APMCopterMode::STABILIZE, _stabilizeFlightMode);
}

void ArduCopterFirmwarePlugin::updateAvailableFlightModes(FlightModeList &modeList)
{
    for (FirmwareFlightMode &mode: modeList) {
        mode.fixedWing = false;
        mode.multiRotor = true;
    }

    _updateFlightModeList(modeList);

}

uint32_t ArduCopterFirmwarePlugin::_convertToCustomFlightModeEnum(uint32_t val) const
{
    switch (val) {
    case APMCustomMode::AUTO:
        return APMCopterMode::AUTO;
    case APMCustomMode::GUIDED:
        return APMCopterMode::GUIDED;
    case APMCustomMode::RTL:
        return APMCopterMode::RTL;
    case APMCustomMode::SMART_RTL:
        return APMCopterMode::SMART_RTL;
    default:
        return UINT32_MAX;
    }
}
