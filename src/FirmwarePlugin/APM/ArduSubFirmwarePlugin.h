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

/// @file
///     @author Rustom Jehangir <rusty@bluerobotics.com>

#ifndef ArduSubFirmwarePlugin_H
#define ArduSubFirmwarePlugin_H

#include "APMFirmwarePlugin.h"
class APMSubmarineFactGroup : public FactGroup
{
    Q_OBJECT

public:
    APMSubmarineFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* camTilt             READ camTilt             CONSTANT)
    Q_PROPERTY(Fact* tetherTurns         READ tetherTurns         CONSTANT)
    Q_PROPERTY(Fact* lightsLevel1        READ lightsLevel1        CONSTANT)
    Q_PROPERTY(Fact* lightsLevel2        READ lightsLevel2        CONSTANT)
    Q_PROPERTY(Fact* pilotGain           READ pilotGain           CONSTANT)
    Q_PROPERTY(Fact* inputHold           READ inputHold     CONSTANT)
    Q_PROPERTY(Fact* rangefinderDistance READ rangefinderDistance CONSTANT)

    Fact* camTilt             (void) { return &_camTiltFact; }
    Fact* tetherTurns         (void) { return &_tetherTurnsFact; }
    Fact* lightsLevel1        (void) { return &_lightsLevel1Fact; }
    Fact* lightsLevel2        (void) { return &_lightsLevel2Fact; }
    Fact* pilotGain           (void) { return &_pilotGainFact; }
    Fact* inputHold           (void) { return &_inputHoldFact; }
    Fact* rangefinderDistance (void) { return &_rangefinderDistanceFact; }

    static const char* _camTiltFactName;
    static const char* _tetherTurnsFactName;
    static const char* _lightsLevel1FactName;
    static const char* _lightsLevel2FactName;
    static const char* _pilotGainFactName;
    static const char* _inputHoldFactName;
    static const char* _rollPitchToggleFactName;
    static const char* _rangefinderDistanceFactName;

    static const char* _settingsGroup;

private:
    Fact            _camTiltFact;
    Fact            _tetherTurnsFact;
    Fact            _lightsLevel1Fact;
    Fact            _lightsLevel2Fact;
    Fact            _pilotGainFact;
    Fact            _inputHoldFact;
    Fact            _rollPitchToggleFact;
    Fact            _rangefinderDistanceFact;
};

class APMSubMode : public APMCustomMode
{
public:
    enum Mode {
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
    };

    APMSubMode(uint32_t mode, bool settable);
};

class ArduSubFirmwarePlugin : public APMFirmwarePlugin
{
    Q_OBJECT

public:
    ArduSubFirmwarePlugin(void);

    int defaultJoystickTXMode(void) final { return 3; }

    void initializeStreamRates(Vehicle* vehicle) override final;

    bool isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities) final;

    bool supportsThrottleModeCenterZero(void) final;

    bool supportsRadio(void) final;

    bool supportsJSButton(void) final;

    bool supportsMotorInterference(void) final;

    /// Return the resource file which contains the vehicle icon used in the flight view when the view is dark (Satellite for instance)
    virtual QString vehicleImageOpaque(const Vehicle* vehicle) const final;

    /// Return the resource file which contains the vehicle icon used in the flight view when the view is light (Map for instance)
    virtual QString vehicleImageOutline(const Vehicle* vehicle) const final;

    QString brandImageIndoor(const Vehicle* vehicle) const final{ Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImageSub"); }
    QString brandImageOutdoor(const Vehicle* vehicle) const final { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImageSub"); }
    const FirmwarePlugin::remapParamNameMajorVersionMap_t& paramNameRemapMajorVersionMap(void) const final { return _remapParamName; }
    int remapParamNameHigestMinorVersionNumber(int majorVersionNumber) const final;
    const QVariantList& toolIndicators(const Vehicle* vehicle) final;
    const QVariantList& modeIndicators(const Vehicle* vehicle) final;
    bool  adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message) final;
    virtual QMap<QString, FactGroup*>* factGroups(void) final;
    void adjustMetaData(MAV_TYPE vehicleType, FactMetaData* metaData) override final;


private:
    QVariantList _toolIndicators;
    QVariantList _modeIndicators;
    static bool _remapParamNameIntialized;
    QMap<QString, QString> _factRenameMap;
    static FirmwarePlugin::remapParamNameMajorVersionMap_t  _remapParamName;
    void _handleNamedValueFloat(mavlink_message_t* message);
    void _handleMavlinkMessage(mavlink_message_t* message);

    QMap<QString, FactGroup*> _nameToFactGroupMap;
    APMSubmarineFactGroup _infoFactGroup;
};
#endif
