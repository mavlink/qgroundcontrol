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

struct APMPlaneMode
{
    enum Mode : uint32_t {
        MANUAL        = 0,
        CIRCLE        = 1,
        STABILIZE     = 2,
        TRAINING      = 3,
        ACRO          = 4,
        FLY_BY_WIRE_A = 5,
        FLY_BY_WIRE_B = 6,
        CRUISE        = 7,
        AUTOTUNE      = 8,
        RESERVED_9    = 9,  // RESERVED FOR FUTURE USE
        AUTO          = 10,
        RTL           = 11,
        LOITER        = 12,
        TAKEOFF       = 13,
        AVOID_ADSB    = 14,
        GUIDED        = 15,
        INITIALIZING  = 16,
        QSTABILIZE    = 17,
        QHOVER        = 18,
        QLOITER       = 19,
        QLAND         = 20,
        QRTL          = 21,
        QAUTOTUNE     = 22,
        QACRO         = 23,
        THERMAL       = 24,
        LOITER2QLAND  = 25,
        AUTOLAND      = 26,
    };
};

class ArduPlaneFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT

public:
    explicit ArduPlaneFirmwarePlugin(QObject *parent = nullptr);
    ~ArduPlaneFirmwarePlugin();

    QString pauseFlightMode() const final { return QString("Loiter"); }
    QString offlineEditingParamFile(Vehicle *vehicle) const final { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/APM/Plane.OfflineEditing.params"); }
    QString autoDisarmParameter(Vehicle *vehicle) const final { Q_UNUSED(vehicle); return QStringLiteral("LAND_DISARMDELAY"); }
    int remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const final;
    const FirmwarePlugin::remapParamNameMajorVersionMap_t &paramNameRemapMajorVersionMap() const final { return _remapParamName; }

    QString stabilizedFlightMode() const final;
    void updateAvailableFlightModes(FlightModeList &modeList) final;

protected:
    uint32_t _convertToCustomFlightModeEnum(uint32_t val) const final;

    const QString _manualFlightMode = tr("Manual");
    const QString _circleFlightMode = tr("Circle");
    const QString _stabilizeFlightMode = tr("Stabilize");
    const QString _trainingFlightMode = tr("Training");
    const QString _acroFlightMode = tr("Acro");
    const QString _flyByWireAFlightMode = tr("FBW A");
    const QString _flyByWireBFlightMode = tr("FBW B");
    const QString _cruiseFlightMode = tr("Cruise");
    const QString _autoTuneFlightMode = tr("Autotune");
    const QString _autoFlightMode = tr("Auto");
    const QString _rtlFlightMode = tr("RTL");
    const QString _loiterFlightMode = tr("Loiter");
    const QString _takeoffFlightMode = tr("Takeoff");
    const QString _avoidADSBFlightMode = tr("Avoid ADSB");
    const QString _guidedFlightMode = tr("Guided");
    const QString _initializingFlightMode = tr("Initializing");
    const QString _qStabilizeFlightMode = tr("QuadPlane Stabilize");
    const QString _qHoverFlightMode = tr("QuadPlane Hover");
    const QString _qLoiterFlightMode = tr("QuadPlane Loiter");
    const QString _qLandFlightMode = tr("QuadPlane Land");
    const QString _qRTLFlightMode = tr("QuadPlane RTL");
    const QString _qAutotuneFlightMode = tr("QuadPlane AutoTune");
    const QString _qAcroFlightMode = tr("QuadPlane Acro");
    const QString _thermalFlightMode = tr("Thermal");
    const QString _loiter2qlandFlightMode = tr("Loiter to QLand");
    const QString _autolandFlightMode = tr("Autoland");

private:
    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t  _remapParamName;
};
