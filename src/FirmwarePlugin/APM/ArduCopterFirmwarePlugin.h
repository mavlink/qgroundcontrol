/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
    explicit ArduCopterFirmwarePlugin(QObject *parent = nullptr);
    ~ArduCopterFirmwarePlugin();

    void guidedModeLand(Vehicle *vehicle) const override { _setFlightModeAndValidate(vehicle, landFlightMode()); }
    const FirmwarePlugin::remapParamNameMajorVersionMap_t &paramNameRemapMajorVersionMap() const override { return _remapParamName; }
    int remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const override;
    bool multiRotorCoaxialMotors(Vehicle* /*vehicle*/) const override { return _coaxialMotors; }
    bool multiRotorXConfig(Vehicle *vehicle) const override;
    QString offlineEditingParamFile(Vehicle *vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/APM/Copter.OfflineEditing.params"); }
    QString pauseFlightMode() const override;
    QString landFlightMode() const override;
    QString takeControlFlightMode() const override;
    QString followFlightMode() const override;
    QString gotoFlightMode() const override { return guidedFlightMode(); }
    QString takeOffFlightMode() const override { return guidedFlightMode(); }
    QString stabilizedFlightMode() const override;
    QString autoDisarmParameter(Vehicle *vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("DISARM_DELAY"); }
    bool supportsSmartRTL() const override { return true; }

    void updateAvailableFlightModes(FlightModeList &modeList) override;

protected:
    uint32_t _convertToCustomFlightModeEnum(uint32_t val) const override;

private:
    const QString _stabilizeFlightMode = tr("Stabilize");
    const QString _acroFlightMode = tr("Acro");
    const QString _altHoldFlightMode = tr("Altitude Hold");
    const QString _autoFlightMode = tr("Auto");
    const QString _guidedFlightMode = tr("Guided");
    const QString _loiterFlightMode = tr("Loiter");
    const QString _rtlFlightMode = tr("RTL");
    const QString _circleFlightMode = tr("Circle");
    const QString _landFlightMode = tr("Land");
    const QString _driftFlightMode = tr("Drift");
    const QString _sportFlightMode = tr("Sport");
    const QString _flipFlightMode = tr("Flip");
    const QString _autotuneFlightMode = tr("Autotune");
    const QString _posHoldFlightMode = tr("Position Hold");
    const QString _brakeFlightMode = tr("Brake");
    const QString _throwFlightMode = tr("Throw");
    const QString _avoidADSBFlightMode = tr("Avoid ADSB");
    const QString _guidedNoGPSFlightMode = tr("Guided No GPS");
    const QString _smartRtlFlightMode = tr("Smart RTL");
    const QString _flowHoldFlightMode = tr("Flow Hold");
    const QString _followFlightMode = tr("Follow");
    const QString _zigzagFlightMode = tr("ZigZag");
    const QString _systemIDFlightMode = tr("SystemID");
    const QString _autoRotateFlightMode = tr("AutoRotate");
    const QString _autoRTLFlightMode = tr("AutoRTL");
    const QString _turtleFlightMode = tr("Turtle");

    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t _remapParamName;
};
