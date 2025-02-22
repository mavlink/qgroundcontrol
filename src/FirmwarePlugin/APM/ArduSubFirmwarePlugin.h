/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

#pragma once

#include "APMFirmwarePlugin.h"
#include "FactGroup.h"

class APMSubmarineFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *camTilt             READ camTilt               CONSTANT)
    Q_PROPERTY(Fact *tetherTurns         READ tetherTurns           CONSTANT)
    Q_PROPERTY(Fact *lightsLevel1        READ lightsLevel1          CONSTANT)
    Q_PROPERTY(Fact *lightsLevel2        READ lightsLevel2          CONSTANT)
    Q_PROPERTY(Fact *pilotGain           READ pilotGain             CONSTANT)
    Q_PROPERTY(Fact *inputHold           READ inputHold             CONSTANT)
    Q_PROPERTY(Fact *rangefinderDistance READ rangefinderDistance   CONSTANT)
    Q_PROPERTY(Fact *rangefinderTarget   READ rangefinderTarget     CONSTANT)

public:
    explicit APMSubmarineFactGroup(QObject *parent = nullptr);
    ~APMSubmarineFactGroup();

    Fact *camTilt() { return &_camTiltFact; }
    Fact *tetherTurns() { return &_tetherTurnsFact; }
    Fact *lightsLevel1() { return &_lightsLevel1Fact; }
    Fact *lightsLevel2() { return &_lightsLevel2Fact; }
    Fact *pilotGain() { return &_pilotGainFact; }
    Fact *inputHold() { return &_inputHoldFact; }
    Fact *rangefinderDistance() { return &_rangefinderDistanceFact; }
    Fact *rangefinderTarget() { return &_rangefinderTargetFact; }

private:
    Fact _camTiltFact = Fact(0, _camTiltFactName, FactMetaData::valueTypeDouble);
    Fact _tetherTurnsFact = Fact(0, _tetherTurnsFactName, FactMetaData::valueTypeDouble);
    Fact _lightsLevel1Fact = Fact(0, _lightsLevel1FactName, FactMetaData::valueTypeDouble);
    Fact _lightsLevel2Fact = Fact(0, _lightsLevel2FactName, FactMetaData::valueTypeDouble);
    Fact _pilotGainFact = Fact(0, _pilotGainFactName, FactMetaData::valueTypeDouble);
    Fact _inputHoldFact = Fact(0, _inputHoldFactName, FactMetaData::valueTypeDouble);
    Fact _rollPitchToggleFact = Fact(0, _rollPitchToggleFactName, FactMetaData::valueTypeDouble);
    Fact _rangefinderDistanceFact = Fact(0, _rangefinderDistanceFactName, FactMetaData::valueTypeDouble);
    Fact _rangefinderTargetFact = Fact(0, _rangefinderTargetFactName, FactMetaData::valueTypeDouble);

    static constexpr const char *_camTiltFactName = "cameraTilt";
    static constexpr const char *_tetherTurnsFactName = "tetherTurns";
    static constexpr const char *_lightsLevel1FactName = "lights1";
    static constexpr const char *_lightsLevel2FactName = "lights2";
    static constexpr const char *_pilotGainFactName = "pilotGain";
    static constexpr const char *_inputHoldFactName = "inputHold";
    static constexpr const char *_rollPitchToggleFactName = "rollPitchToggle";
    static constexpr const char *_rangefinderDistanceFactName = "rangefinderDistance";
    static constexpr const char *_rangefinderTargetFactName = "rangefinderTarget";

    static const char *_settingsGroup;
};

/*===========================================================================*/

struct APMSubMode
{
    enum Mode : uint32_t{
        STABILIZE         = 0,   // Hold level position
        ACRO              = 1,   // Manual angular rate, throttle
        ALT_HOLD          = 2,   // Depth hold
        AUTO              = 3,   // Full auto to waypoint
        GUIDED            = 4,   // Full auto to coordinate/direction
        RESERVED_5        = 5,
        RESERVED_6        = 6,
        CIRCLE            = 7,   // Auto circling
        RESERVED_8        = 8,
        SURFACE           = 9,   // Auto return to surface
        RESERVED_10       = 10,
        RESERVED_11       = 11,
        RESERVED_12       = 12,
        RESERVED_13       = 13,
        RESERVED_14       = 14,
        RESERVED_15       = 15,
        POSHOLD           = 16,  // Hold position
        RESERVED_17       = 17,
        RESERVED_18       = 18,
        MANUAL            = 19,
        MOTORDETECTION    = 20,
        SURFTRAK          = 21,  // Surface (seafloor) tracking, aka hold range
    };
};

class ArduSubFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT

public:
    explicit ArduSubFirmwarePlugin(QObject *parent = nullptr);
    ~ArduSubFirmwarePlugin();

    int defaultJoystickTXMode() const final { return 3; }
    void initializeStreamRates(Vehicle *vehicle) final;
    bool isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities) const final;
    bool supportsThrottleModeCenterZero() const final { return false; }
    bool supportsRadio() const final { return false; }
    bool supportsJSButton() const final { return true; }
    bool supportsMotorInterference() const final { return false; }

    /// Return the resource file which contains the vehicle icon used in the flight view when the view is dark (Satellite for instance)
    QString vehicleImageOpaque(const Vehicle* vehicle) const final { return QStringLiteral("/qmlimages/subVehicleArrowOpaque.png"); }

    /// Return the resource file which contains the vehicle icon used in the flight view when the view is light (Map for instance)
    QString vehicleImageOutline(const Vehicle* vehicle) const final;

    QString brandImageIndoor(const Vehicle *vehicle) const final { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImageSub"); }
    QString brandImageOutdoor(const Vehicle *vehicle) const final { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImageSub"); }
    const FirmwarePlugin::remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap() const final { return _remapParamName; }
    int remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const final;
    const QVariantList &toolIndicators(const Vehicle *vehicle) final;
    const QVariantList &modeIndicators(const Vehicle *vehicle) final;
    bool adjustIncomingMavlinkMessage(Vehicle *vehicle, mavlink_message_t *message) final;
    QMap<QString, FactGroup*> *factGroups() final;
    void adjustMetaData(MAV_TYPE vehicleType, FactMetaData *metaData) final;

    QString stabilizedFlightMode() const final;
    QString motorDetectionFlightMode() const final;
    void updateAvailableFlightModes(FlightModeList &modeList) final;

protected:
    uint32_t _convertToCustomFlightModeEnum(uint32_t val) const final;

    const QString _manualFlightMode = tr("Manual");
    const QString _stabilizeFlightMode = tr("Stabilize");
    const QString _acroFlightMode = tr("Acro");
    const QString _altHoldFlightMode = tr("Depth Hold");
    const QString _autoFlightMode = tr("Auto");
    const QString _guidedFlightMode = tr("Guided");
    const QString _circleFlightMode = tr("Circle");
    const QString _surfaceFlightMode = tr("Surface");
    const QString _posHoldFlightMode =tr("Position Hold");
    const QString _motorDetectionFlightMode = tr("Motor Detection");
    const QString _surftrakFlightMode = tr("Surftrak");

private:
    QVariantList _toolIndicators;
    QVariantList _modeIndicators;
    static bool _remapParamNameIntialized;
    QMap<QString, QString> _factRenameMap;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t _remapParamName;
    void _handleNamedValueFloat(mavlink_message_t *message);
    void _handleMavlinkMessage(mavlink_message_t *message);

    QMap<QString, FactGroup*> _nameToFactGroupMap;
    APMSubmarineFactGroup _infoFactGroup;
};
