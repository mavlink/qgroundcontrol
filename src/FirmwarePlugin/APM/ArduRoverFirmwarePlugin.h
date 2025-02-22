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
    explicit ArduRoverFirmwarePlugin(QObject *parent = nullptr);
    ~ArduRoverFirmwarePlugin();

    QString pauseFlightMode() const final { return QStringLiteral("Hold"); }
    QString followFlightMode() const final { return QStringLiteral("Follow"); }
    void guidedModeChangeAltitude(Vehicle* vehicle, double altitudeChange, bool pauseVehicle) final;
    int remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const final;
    const FirmwarePlugin::remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap() const final { return _remapParamName; }
    bool supportsNegativeThrust(Vehicle*) const final { return true; }
    bool supportsSmartRTL() const final { return true; }
    QString offlineEditingParamFile(Vehicle *vehicle) const final { Q_UNUSED(vehicle); return QStringLiteral(":/FirmwarePlugin/APM/Rover.OfflineEditing.params"); }

    QString stabilizedFlightMode() const final;
    void updateAvailableFlightModes(FlightModeList &modeList) final;

protected:
    uint32_t _convertToCustomFlightModeEnum(uint32_t val) const final;

    const QString _manualFlightMode = tr("Manual");
    const QString _acroFlightMode = tr("Acro");
    const QString _learningFlightMode = tr("Learning");
    const QString _steeringFlightMode = tr("Steering");
    const QString _holdFlightMode = tr("Hold");
    const QString _loiterFlightMode = tr("Loiter");
    const QString _followFlightMode = tr("Follow");
    const QString _simpleFlightMode = tr("Simple");
    const QString _dockFlightMode = tr("Dock");
    const QString _circleFlightMode = tr("Circle");
    const QString _autoFlightMode = tr("Auto");
    const QString _rtlFlightMode = tr("RTL");
    const QString _smartRtlFlightMode = tr("Smart RTL");
    const QString _guidedFlightMode = tr("Guided");
    const QString _initializingFlightMode = tr("Initializing");

private:
    static bool _remapParamNameIntialized;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t _remapParamName;
};
