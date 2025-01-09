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

#pragma once

#include "APMFirmwarePlugin.h"

struct APMCopterMode
{
    enum Mode : uint32_t{
        STABILIZE   = 0,   // hold level position
        ACRO        = 1,   // rate control
        ALT_HOLD    = 2,   // AUTO control
        AUTO        = 3,   // AUTO control
        GUIDED      = 4,   // AUTO control
        LOITER      = 5,   // Hold a single location
        RTL         = 6,   // AUTO control
        CIRCLE      = 7,   // AUTO control
        POSITION    = 8,   // Deprecated
        LAND        = 9,   // AUTO control
        OF_LOITER   = 10,  // Deprecated
        DRIFT       = 11,  // Drift 'Car Like' mode
        RESERVED_12 = 12,  // RESERVED FOR FUTURE USE
        SPORT       = 13,
        FLIP        = 14,
        AUTOTUNE    = 15,
        POS_HOLD    = 16, // HYBRID LOITER.
        BRAKE       = 17,
        THROW       = 18,
        AVOID_ADSB  = 19,
        GUIDED_NOGPS= 20,
        SMART_RTL   = 21,  // SMART_RTL returns to home by retracing its steps
        FLOWHOLD    = 22,  // FLOWHOLD holds position with optical flow without rangefinder
        FOLLOW      = 23,  // follow attempts to follow another vehicle or ground station
        ZIGZAG      = 24,  // ZIGZAG mode is able to fly in a zigzag manner with predefined point A and point B
        SYSTEMID    = 25,
        AUTOROTATE  = 26,
        AUTO_RTL    = 27,
        TURTLE      = 28,
    };
};

class ArduCopterFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT

public:
    ArduCopterFirmwarePlugin(void);

    // Overrides from FirmwarePlugin
    void    guidedModeLand                      (Vehicle* vehicle) final;
    const FirmwarePlugin::remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap(void) const final { return _remapParamName; }
    int     remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const final;
    bool    multiRotorCoaxialMotors             (Vehicle* vehicle) final;
    bool    multiRotorXConfig                   (Vehicle* vehicle) final;
    QString offlineEditingParamFile             (Vehicle* vehicle) final { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/APM/Copter.OfflineEditing.params"); }
    QString pauseFlightMode                     (void) const override;
    QString landFlightMode                      (void) const override;
    QString takeControlFlightMode               (void) const override;
    QString followFlightMode                    (void) const override;
    QString gotoFlightMode                      (void) const override;
    QString takeOffFlightMode                   (void) const override;
    QString stabilizedFlightMode                (void) const override;
    QString autoDisarmParameter                 (Vehicle* vehicle) override { Q_UNUSED(vehicle); return QStringLiteral("DISARM_DELAY"); }
    bool    supportsSmartRTL                    (void) const override { return true; }

    void    updateAvailableFlightModes          (FlightModeList modeList) override;
protected:
    uint32_t    _convertToCustomFlightModeEnum(uint32_t val) const override;


    QString     _stabilizeFlightMode;
    QString     _acroFlightMode;
    QString     _altHoldFlightMode;
    QString     _autoFlightMode;
    QString     _guidedFlightMode;
    QString     _loiterFlightMode;
    QString     _rtlFlightMode;
    QString     _circleFlightMode;
    QString     _landFlightMode;
    QString     _driftFlightMode;
    QString     _sportFlightMode;
    QString     _flipFlightMode;
    QString     _autotuneFlightMode;
    QString     _posHoldFlightMode;
    QString     _brakeFlightMode;
    QString     _throwFlightMode;
    QString     _avoidADSBFlightMode;
    QString     _guidedNoGPSFlightMode;
    QString     _smartRtlFlightMode;
    QString     _flowHoldFlightMode;
    QString     _followFlightMode;
    QString     _zigzagFlightMode;
    QString     _systemIDFlightMode;
    QString     _autoRotateFlightMode;
    QString     _autoRTLFlightMode;
    QString     _turtleFlightMode;

private:
    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t  _remapParamName;
};
