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

struct APMRoverMode
{
    enum Mode : uint32_t{
        MANUAL          = 0,
        ACRO            = 1,
        LEARNING        = 2, // Deprecated
        STEERING        = 3,
        HOLD            = 4,
        LOITER          = 5,
        FOLLOW          = 6,
        SIMPLE          = 7,
        DOCK            = 8,
        CIRCLE          = 9,
        AUTO            = 10,
        RTL             = 11,
        SMART_RTL       = 12,
        GUIDED          = 15,
        INITIALIZING    = 16
    };
};

class ArduRoverFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT
    
public:
    ArduRoverFirmwarePlugin(void);

    // Overrides from FirmwarePlugin
    QString pauseFlightMode                         (void) const override { return QStringLiteral("Hold"); }
    QString followFlightMode                        (void) const override { return QStringLiteral("Follow"); }
    void    guidedModeChangeAltitude                (Vehicle* vehicle, double altitudeChange, bool pauseVehicle) final;
    int     remapParamNameHigestMinorVersionNumber  (int majorVersionNumber) const final;
    const   FirmwarePlugin::remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap(void) const final { return _remapParamName; }
    bool    supportsNegativeThrust                  (Vehicle *) final;
    bool    supportsSmartRTL                        (void) const override { return true; }
    QString offlineEditingParamFile                 (Vehicle* vehicle) override { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/APM/Rover.OfflineEditing.params"); }

    QString stabilizedFlightMode                    (void) const override;
    void    updateAvailableFlightModes              (FlightModeList modeList) override;

protected:
    uint32_t    _convertToCustomFlightModeEnum(uint32_t val) const override;


    QString     _manualFlightMode       ;
    QString     _acroFlightMode         ;
    QString     _learningFlightMode     ;
    QString     _steeringFlightMode     ;
    QString     _holdFlightMode         ;
    QString     _loiterFlightMode       ;
    QString     _followFlightMode       ;
    QString     _simpleFlightMode       ;
    QString     _dockFlightMode         ;
    QString     _circleFlightMode       ;
    QString     _autoFlightMode         ;
    QString     _rtlFlightMode          ;
    QString     _smartRtlFlightMode     ;
    QString     _guidedFlightMode       ;
    QString     _initializingFlightMode ;

private:
    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t  _remapParamName;
};
