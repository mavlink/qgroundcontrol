/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    };
    static const int modeCount = 21;

    APMCopterMode(uint32_t mode, bool settable);
};

class ArduCopterFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT

public:
    ArduCopterFirmwarePlugin(void);

    // Overrides from FirmwarePlugin
    bool    isCapable                           (const Vehicle *vehicle, FirmwareCapabilities capabilities) final;
    void    setGuidedMode                       (Vehicle* vehicle, bool guidedMode) final;
    void    pauseVehicle                        (Vehicle* vehicle) final;
    void    guidedModeRTL                       (Vehicle* vehicle) final;
    void    guidedModeLand                      (Vehicle* vehicle) final;
    void    guidedModeTakeoff                   (Vehicle* vehicle) final;
    void    guidedModeGotoLocation              (Vehicle* vehicle, const QGeoCoordinate& gotoCoord) final;
    void    guidedModeChangeAltitude            (Vehicle* vehicle, double altitudeChange) final;
    const FirmwarePlugin::remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap(void) const final { return _remapParamName; }
    int     remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const final;
    bool    multiRotorCoaxialMotors             (Vehicle* vehicle) final;
    bool    multiRotorXConfig                   (Vehicle* vehicle) final;
    QString geoFenceRadiusParam                 (Vehicle* vehicle) final;
    QString offlineEditingParamFile             (Vehicle* vehicle) final { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/APM/Copter.OfflineEditing.params"); }
    QString pauseFlightMode                     (void) const override { return QString("Brake"); }
    QString missionFlightMode                   (void) const override { return QString("Auto"); }
    QString rtlFlightMode                       (void) const override { return QString("RTL"); }
    QString landFlightMode                      (void) const override { return QString("Land"); }
    QString takeControlFlightMode               (void) const override { return QString("Loiter"); }
    bool    vehicleYawsToNextWaypointInMission  (const Vehicle* vehicle) const final;
    QString autoDisarmParameter                 (Vehicle* vehicle) final { Q_UNUSED(vehicle); return QStringLiteral("DISARM_DELAY"); }
    void    startMission                        (Vehicle* vehicle) override;

private:
    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t  _remapParamName;

    bool _guidedModeTakeoff(Vehicle* vehicle);
};

#endif
