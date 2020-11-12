/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef ArduCopterFirmwarePlugin_H
#define ArduCopterFirmwarePlugin_H

#include "APMFirmwarePlugin.h"

class APMCopterMode : public APMCustomMode
{
public:
    enum Mode {
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
#if 0
    // Follow me not ready for Stable
        FOLLOW      = 23,  // follow attempts to follow another vehicle or ground station
#endif
        ZIGZAG      = 24,  // ZIGZAG mode is able to fly in a zigzag manner with predefined point A and point B
    };

    APMCopterMode(uint32_t mode, bool settable);
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
    QString pauseFlightMode                     (void) const override { return QStringLiteral("Brake"); }
    QString landFlightMode                      (void) const override { return QStringLiteral("Land"); }
    QString takeControlFlightMode               (void) const override { return QStringLiteral("Loiter"); }
    QString followFlightMode                    (void) const override { return QStringLiteral("Follow"); }
    QString autoDisarmParameter                 (Vehicle* vehicle) override { Q_UNUSED(vehicle); return QStringLiteral("DISARM_DELAY"); }
    bool    supportsSmartRTL                    (void) const override { return true; }
#if 0
    // Follow me not ready for Stable
    void    sendGCSMotionReport                 (Vehicle* vehicle, FollowMe::GCSMotionReport& motionReport, uint8_t estimatationCapabilities) override;
#endif

private:
    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t  _remapParamName;
};

#endif
