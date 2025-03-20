/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "ArduCopterFirmwarePlugin.h"
#include "ParameterManager.h"
#include "Vehicle.h"

bool ArduCopterFirmwarePlugin::_remapParamNameIntialized = false;
FirmwarePlugin::remapParamNameMajorVersionMap_t ArduCopterFirmwarePlugin::_remapParamName;


ArduCopterFirmwarePlugin::ArduCopterFirmwarePlugin(void)
    : _stabilizeFlightMode      (tr("Stabilize"))
    , _acroFlightMode           (tr("Acro"))
    , _altHoldFlightMode        (tr("Altitude Hold"))
    , _autoFlightMode           (tr("Auto"))
    , _guidedFlightMode         (tr("Guided"))
    , _loiterFlightMode         (tr("Loiter"))
    , _rtlFlightMode            (tr("RTL"))
    , _circleFlightMode         (tr("Circle"))
    , _landFlightMode           (tr("Land"))
    , _driftFlightMode          (tr("Drift"))
    , _sportFlightMode          (tr("Sport"))
    , _flipFlightMode           (tr("Flip"))
    , _autotuneFlightMode       (tr("Autotune"))
    , _posHoldFlightMode        (tr("Position Hold"))
    , _brakeFlightMode          (tr("Brake"))
    , _throwFlightMode          (tr("Throw"))
    , _avoidADSBFlightMode      (tr("Avoid ADSB"))
    , _guidedNoGPSFlightMode    (tr("Guided No GPS"))
    , _smartRtlFlightMode       (tr("Smart RTL"))
    , _flowHoldFlightMode       (tr("Flow Hold"))
    , _followFlightMode         (tr("Follow"))
    , _zigzagFlightMode         (tr("ZigZag"))
    , _systemIDFlightMode       (tr("SystemID"))
    , _autoRotateFlightMode     (tr("AutoRotate"))
    , _autoRTLFlightMode        (tr("AutoRTL"))
    , _turtleFlightMode         (tr("Turtle"))
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

    updateAvailableFlightModes({
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
    });

    if (!_remapParamNameIntialized) {
        FirmwarePlugin::remapParamNameMap_t& remapV3_6 = _remapParamName[3][6];

        remapV3_6["BATT_AMP_PERVLT"] =  QStringLiteral("BATT_AMP_PERVOL");
        remapV3_6["BATT2_AMP_PERVLT"] = QStringLiteral("BATT2_AMP_PERVOL");
        remapV3_6["BATT_LOW_MAH"] =     QStringLiteral("FS_BATT_MAH");
        remapV3_6["BATT_LOW_VOLT"] =    QStringLiteral("FS_BATT_VOLTAGE");
        remapV3_6["BATT_FS_LOW_ACT"] =  QStringLiteral("FS_BATT_ENABLE");
        remapV3_6["PSC_ACCZ_P"] =       QStringLiteral("ACCEL_Z_P");
        remapV3_6["PSC_ACCZ_I"] =       QStringLiteral("ACCEL_Z_I");

        FirmwarePlugin::remapParamNameMap_t& remapV3_7 = _remapParamName[3][7];

        remapV3_7["BATT_ARM_VOLT"] =    QStringLiteral("ARMING_VOLT_MIN");
        remapV3_7["BATT2_ARM_VOLT"] =   QStringLiteral("ARMING_VOLT2_MIN");
        remapV3_7["RC7_OPTION"] =       QStringLiteral("CH7_OPT");
        remapV3_7["RC8_OPTION"] =       QStringLiteral("CH8_OPT");
        remapV3_7["RC9_OPTION"] =       QStringLiteral("CH9_OPT");
        remapV3_7["RC10_OPTION"] =      QStringLiteral("CH10_OPT");
        remapV3_7["RC11_OPTION"] =      QStringLiteral("CH11_OPT");
        remapV3_7["RC12_OPTION"] =      QStringLiteral("CH12_OPT");

        FirmwarePlugin::remapParamNameMap_t& remapV4_0 = _remapParamName[4][0];

        remapV4_0["TUNE_MIN"] = QStringLiteral("TUNE_HIGH");
        remapV3_7["TUNE_MAX"] = QStringLiteral("TUNE_LOW");

        _remapParamNameIntialized = true;
    }
}

int ArduCopterFirmwarePlugin::remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const
{
    // Remapping supports up to 3.7
    return majorVersionNumber == 3 ? 7 : Vehicle::versionNotSetValue;
}

void ArduCopterFirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    _setFlightModeAndValidate(vehicle, landFlightMode());
}

bool ArduCopterFirmwarePlugin::multiRotorCoaxialMotors(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);
    return _coaxialMotors;
}

bool ArduCopterFirmwarePlugin::multiRotorXConfig(Vehicle* vehicle)
{
    return vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "FRAME")->rawValue().toInt() != 0;
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

QString ArduCopterFirmwarePlugin::gotoFlightMode() const
{
    return guidedFlightMode();
}

QString ArduCopterFirmwarePlugin::takeOffFlightMode() const
{
    return guidedFlightMode();
}

QString ArduCopterFirmwarePlugin::stabilizedFlightMode() const
{
    return _modeEnumToString.value(APMCopterMode::STABILIZE, _stabilizeFlightMode);
}

void ArduCopterFirmwarePlugin::updateAvailableFlightModes(FlightModeList modeList)
{
    for(auto &mode: modeList){
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
    }
    return UINT32_MAX;
}
