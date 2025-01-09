/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Pritam Ghanghas <pritam.ghanghas@gmail.com>

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
    };
};

class ArduPlaneFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT
    
public:
    ArduPlaneFirmwarePlugin(void);

    // Overrides from FirmwarePlugin
    QString pauseFlightMode                         (void) const override { return QString("Loiter"); }
    QString offlineEditingParamFile                 (Vehicle* vehicle) final { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/APM/Plane.OfflineEditing.params"); }
    QString autoDisarmParameter                     (Vehicle* vehicle) final { Q_UNUSED(vehicle); return QStringLiteral("LAND_DISARMDELAY"); }
    int     remapParamNameHigestMinorVersionNumber  (int majorVersionNumber) const final;    
    const FirmwarePlugin::remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap(void) const final { return _remapParamName; }

    QString stabilizedFlightMode                    (void) const override;
    void    updateAvailableFlightModes              (FlightModeList modeList) override;

protected:
    uint32_t    _convertToCustomFlightModeEnum(uint32_t val) const override;


    QString     _manualFlightMode       ;
    QString     _circleFlightMode       ;
    QString     _stabilizeFlightMode    ;
    QString     _trainingFlightMode     ;
    QString     _acroFlightMode         ;
    QString     _flyByWireAFlightMode   ;
    QString     _flyByWireBFlightMode   ;
    QString     _cruiseFlightMode       ;
    QString     _autoTuneFlightMode     ;
    QString     _autoFlightMode         ;
    QString     _rtlFlightMode          ;
    QString     _loiterFlightMode       ;
    QString     _takeoffFlightMode      ;
    QString     _avoidADSBFlightMode    ;
    QString     _guidedFlightMode       ;
    QString     _initializingFlightMode ;
    QString     _qStabilizeFlightMode   ;
    QString     _qHoverFlightMode       ;
    QString     _qLoiterFlightMode      ;
    QString     _qLandFlightMode        ;
    QString     _qRTLFlightMode         ;
    QString     _qAutotuneFlightMode    ;
    QString     _qAcroFlightMode        ;
    QString     _thermalFlightMode      ;
private:
    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t  _remapParamName;
};
