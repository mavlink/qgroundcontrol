/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"
#include <QtCore/QSet>
#include <QtCore/QVariantMap>
#include <QtCore/QHash>

class Vehicle;

/// AM32 ESC EEPROM settings fact group
class AM32EepromFactGroup : public FactGroup
{
    Q_OBJECT

    // Read-only info properties
    Q_PROPERTY(Fact* firmwareMajor         READ firmwareMajor          CONSTANT)
    Q_PROPERTY(Fact* firmwareMinor         READ firmwareMinor          CONSTANT)
    Q_PROPERTY(Fact* bootloaderVersion     READ bootloaderVersion      CONSTANT)
    Q_PROPERTY(Fact* eepromVersion         READ eepromVersion          CONSTANT)

    // Boolean settings
    Q_PROPERTY(Fact* directionReversed     READ directionReversed      CONSTANT)
    Q_PROPERTY(Fact* bidirectionalMode     READ bidirectionalMode      CONSTANT)
    Q_PROPERTY(Fact* sineStartup           READ sineStartup            CONSTANT)
    Q_PROPERTY(Fact* complementaryPwm      READ complementaryPwm       CONSTANT)
    Q_PROPERTY(Fact* variablePwmFreq       READ variablePwmFreq        CONSTANT)
    Q_PROPERTY(Fact* stuckRotorProtection  READ stuckRotorProtection   CONSTANT)
    Q_PROPERTY(Fact* brakeOnStop           READ brakeOnStop            CONSTANT)
    Q_PROPERTY(Fact* antiStall             READ antiStall              CONSTANT)
    Q_PROPERTY(Fact* telemetry30ms         READ telemetry30ms          CONSTANT)
    Q_PROPERTY(Fact* lowVoltageCutoff      READ lowVoltageCutoff       CONSTANT)
    Q_PROPERTY(Fact* rcCarReversing        READ rcCarReversing         CONSTANT)
    Q_PROPERTY(Fact* hallSensors           READ hallSensors            CONSTANT)
    Q_PROPERTY(Fact* autoTiming            READ autoTiming             CONSTANT)
    Q_PROPERTY(Fact* disableStickCalibration READ disableStickCalibration CONSTANT)

    // Numeric settings
    Q_PROPERTY(Fact* maxRampSpeed          READ maxRampSpeed           CONSTANT)
    Q_PROPERTY(Fact* minDutyCycle          READ minDutyCycle           CONSTANT)
    Q_PROPERTY(Fact* timingAdvance         READ timingAdvance          CONSTANT)
    Q_PROPERTY(Fact* pwmFrequency          READ pwmFrequency           CONSTANT)
    Q_PROPERTY(Fact* startupPower          READ startupPower           CONSTANT)
    Q_PROPERTY(Fact* motorKv               READ motorKv                CONSTANT)
    Q_PROPERTY(Fact* motorPoles            READ motorPoles             CONSTANT)
    Q_PROPERTY(Fact* beepVolume            READ beepVolume             CONSTANT)
    Q_PROPERTY(Fact* activeBrakePower      READ activeBrakePower       CONSTANT)
    Q_PROPERTY(Fact* dragBrakeStrength     READ dragBrakeStrength      CONSTANT)
    Q_PROPERTY(Fact* runningBrakeAmount    READ runningBrakeAmount     CONSTANT)
    Q_PROPERTY(Fact* temperatureLimit      READ temperatureLimit       CONSTANT)
    Q_PROPERTY(Fact* currentLimit          READ currentLimit           CONSTANT)
    Q_PROPERTY(Fact* lowVoltageThreshold   READ lowVoltageThreshold    CONSTANT)
    Q_PROPERTY(Fact* sineModeRange         READ sineModeRange          CONSTANT)
    Q_PROPERTY(Fact* sineModeStrength      READ sineModeStrength       CONSTANT)
    Q_PROPERTY(Fact* inputType             READ inputType              CONSTANT)
    Q_PROPERTY(Fact* currentPidP           READ currentPidP            CONSTANT)
    Q_PROPERTY(Fact* currentPidI           READ currentPidI            CONSTANT)
    Q_PROPERTY(Fact* currentPidD           READ currentPidD            CONSTANT)
    Q_PROPERTY(Fact* absoluteVoltageCutoff READ absoluteVoltageCutoff  CONSTANT)
    Q_PROPERTY(Fact* servoLowThreshold     READ servoLowThreshold      CONSTANT)
    Q_PROPERTY(Fact* servoHighThreshold    READ servoHighThreshold     CONSTANT)
    Q_PROPERTY(Fact* servoNeutral          READ servoNeutral           CONSTANT)
    Q_PROPERTY(Fact* servoDeadband         READ servoDeadband          CONSTANT)

    Q_PROPERTY(bool  dataLoaded            READ dataLoaded             NOTIFY dataLoadedChanged)
    Q_PROPERTY(bool  hasUnsavedChanges     READ hasUnsavedChanges      NOTIFY hasUnsavedChangesChanged)
    Q_PROPERTY(int   escIndex              READ escIndex               WRITE setEscIndex NOTIFY escIndexChanged)

public:
    AM32EepromFactGroup(QObject* parent = nullptr);

    // Read-only info facts
    Fact* firmwareMajor()       { return &_firmwareMajorFact; }
    Fact* firmwareMinor()       { return &_firmwareMinorFact; }
    Fact* bootloaderVersion()    { return &_bootloaderVersionFact; }
    Fact* eepromVersion()        { return &_eepromVersionFact; }

    // Boolean setting facts
    Fact* directionReversed()    { return &_directionReversedFact; }
    Fact* bidirectionalMode()    { return &_bidirectionalModeFact; }
    Fact* sineStartup()          { return &_sineStartupFact; }
    Fact* complementaryPwm()     { return &_complementaryPwmFact; }
    Fact* variablePwmFreq()      { return &_variablePwmFreqFact; }
    Fact* stuckRotorProtection() { return &_stuckRotorProtectionFact; }
    Fact* brakeOnStop()          { return &_brakeOnStopFact; }
    Fact* antiStall()            { return &_antiStallFact; }
    Fact* telemetry30ms()        { return &_telemetry30msFact; }
    Fact* lowVoltageCutoff()     { return &_lowVoltageCutoffFact; }
    Fact* rcCarReversing()       { return &_rcCarReversingFact; }
    Fact* hallSensors()          { return &_hallSensorsFact; }
    Fact* autoTiming()           { return &_autoTimingFact; }
    Fact* disableStickCalibration() { return &_disableStickCalibrationFact; }

    // Numeric setting facts
    Fact* maxRampSpeed()         { return &_maxRampSpeedFact; }
    Fact* minDutyCycle()         { return &_minDutyCycleFact; }
    Fact* timingAdvance()        { return &_timingAdvanceFact; }
    Fact* pwmFrequency()         { return &_pwmFrequencyFact; }
    Fact* startupPower()         { return &_startupPowerFact; }
    Fact* motorKv()              { return &_motorKvFact; }
    Fact* motorPoles()           { return &_motorPolesFact; }
    Fact* beepVolume()           { return &_beepVolumeFact; }
    Fact* activeBrakePower()     { return &_activeBrakePowerFact; }
    Fact* dragBrakeStrength()    { return &_dragBrakeStrengthFact; }
    Fact* runningBrakeAmount()   { return &_runningBrakeAmountFact; }
    Fact* temperatureLimit()     { return &_temperatureLimitFact; }
    Fact* currentLimit()         { return &_currentLimitFact; }
    Fact* lowVoltageThreshold()  { return &_lowVoltageThresholdFact; }
    Fact* sineModeRange()        { return &_sineModeRangeFact; }
    Fact* sineModeStrength()     { return &_sineModeStrengthFact; }
    Fact* inputType()            { return &_inputTypeFact; }
    Fact* currentPidP()          { return &_currentPidPFact; }
    Fact* currentPidI()          { return &_currentPidIFact; }
    Fact* currentPidD()          { return &_currentPidDFact; }
    Fact* absoluteVoltageCutoff() { return &_absoluteVoltageCutoffFact; }
    Fact* servoLowThreshold()    { return &_servoLowThresholdFact; }
    Fact* servoHighThreshold()   { return &_servoHighThresholdFact; }
    Fact* servoNeutral()         { return &_servoNeutralFact; }
    Fact* servoDeadband()        { return &_servoDeadbandFact; }

    // Status
    bool dataLoaded() const { return _dataLoaded; }
    bool hasUnsavedChanges() const { return !_pendingChanges.isEmpty(); }
    int escIndex() const { return _escIndex; }
    void setEscIndex(int index);

    /// Parse EEPROM data from AM32_EEPROM message
    void handleEepromData(const uint8_t* data, int length);

    /// Pack current facts into EEPROM data for writing
    QByteArray packEepromData() const;

    /// Calculate write mask based on modified bytes
    void calculateWriteMask(uint32_t writeMask[6]) const;

    /// Request EEPROM read from ESC
    Q_INVOKABLE void requestReadAll(Vehicle* vehicle);

    /// Write EEPROM data to ESC (only modified bytes)
    Q_INVOKABLE void requestWrite(Vehicle* vehicle);

    /// Apply pending changes to this ESC
    Q_INVOKABLE void applyPendingChanges(const QVariantMap& changes);

    /// Get pending changes
    Q_INVOKABLE QVariantMap getPendingChanges() const { return _pendingChanges; }

    /// Clear pending changes (after write)
    Q_INVOKABLE void clearPendingChanges();

    /// Discard pending changes and revert to fact values
    Q_INVOKABLE void discardChanges();

    /// Check if settings match another ESC
    Q_INVOKABLE bool settingsMatch(AM32EepromFactGroup* other) const;

    /// Get a fact value by name (includes pending changes)
    Q_INVOKABLE QVariant getFactValue(const QString& factName) const;

signals:
    void dataLoadedChanged();
    void hasUnsavedChangesChanged();
    void escIndexChanged();
    void readComplete(bool success);
    void writeComplete(bool success);
    void pendingChangesUpdated();

private:
    void _initializeByteMapping();
    Fact* _getFactByName(const QString& name) const;

    // EEPROM byte indices for each setting
    enum EepromIndices {
        BYTE_EEPROM_START = 0,
        BYTE_EEPROM_VERSION = 1,
        BYTE_BOOTLOADER_VERSION = 2,
        BYTE_FIRMWARE_MAJOR = 3,
        BYTE_FIRMWARE_MINOR = 4,
        BYTE_MAX_RAMP_SPEED = 5,
        BYTE_MIN_DUTY_CYCLE = 6,
        BYTE_STICK_CALIBRATION = 7,
        BYTE_VOLTAGE_CUTOFF = 8,
        BYTE_CURRENT_PID_P = 9,
        BYTE_CURRENT_PID_I = 10,
        BYTE_CURRENT_PID_D = 11,
        BYTE_ACTIVE_BRAKE_POWER = 12,
        // 13-16 reserved
        BYTE_DIR_REVERSED = 17,
        BYTE_BI_DIRECTION = 18,
        BYTE_USE_SINE_START = 19,
        BYTE_COMP_PWM = 20,
        BYTE_VARIABLE_PWM = 21,
        BYTE_STUCK_ROTOR = 22,
        BYTE_TIMING_ADVANCE = 23,
        BYTE_PWM_FREQUENCY = 24,
        BYTE_STARTUP_POWER = 25,
        BYTE_MOTOR_KV = 26,
        BYTE_MOTOR_POLES = 27,
        BYTE_BRAKE_ON_STOP = 28,
        BYTE_ANTI_STALL = 29,
        BYTE_BEEP_VOLUME = 30,
        BYTE_TELEMETRY_30MS = 31,
        BYTE_SERVO_LOW = 32,
        BYTE_SERVO_HIGH = 33,
        BYTE_SERVO_NEUTRAL = 34,
        BYTE_SERVO_DEADBAND = 35,
        BYTE_LOW_VOLTAGE_CUTOFF = 36,
        BYTE_LOW_VOLTAGE_THRESHOLD = 37,
        BYTE_RC_CAR_REVERSING = 38,
        BYTE_HALL_SENSORS = 39,
        BYTE_SINE_MODE_RANGE = 40,
        BYTE_DRAG_BRAKE = 41,
        BYTE_RUNNING_BRAKE = 42,
        BYTE_TEMP_LIMIT = 43,
        BYTE_CURRENT_LIMIT = 44,
        BYTE_SINE_MODE_STRENGTH = 45,
        BYTE_INPUT_TYPE = 46,
        BYTE_AUTO_TIMING = 47
    };

    // Struct to organize byte mapping
    struct ByteMapping {
        Fact* fact;
        int byteIndex;

        ByteMapping(Fact* f = nullptr, int idx = -1) : fact(f), byteIndex(idx) {}
    };

    // Info facts (read-only)
    Fact _firmwareMajorFact         = Fact(0, QStringLiteral("firmwareMajor"), FactMetaData::valueTypeUint8);
    Fact _firmwareMinorFact         = Fact(0, QStringLiteral("firmwareMinor"), FactMetaData::valueTypeUint8);
    Fact _bootloaderVersionFact     = Fact(0, QStringLiteral("bootloaderVersion"), FactMetaData::valueTypeUint8);
    Fact _eepromVersionFact         = Fact(0, QStringLiteral("eepromVersion"), FactMetaData::valueTypeUint8);

    // Boolean setting facts
    Fact _directionReversedFact     = Fact(0, QStringLiteral("directionReversed"), FactMetaData::valueTypeBool);
    Fact _bidirectionalModeFact     = Fact(0, QStringLiteral("bidirectionalMode"), FactMetaData::valueTypeBool);
    Fact _sineStartupFact           = Fact(0, QStringLiteral("sineStartup"), FactMetaData::valueTypeBool);
    Fact _complementaryPwmFact      = Fact(0, QStringLiteral("complementaryPwm"), FactMetaData::valueTypeBool);
    Fact _variablePwmFreqFact       = Fact(0, QStringLiteral("variablePwmFreq"), FactMetaData::valueTypeBool);
    Fact _stuckRotorProtectionFact  = Fact(0, QStringLiteral("stuckRotorProtection"), FactMetaData::valueTypeBool);
    Fact _brakeOnStopFact           = Fact(0, QStringLiteral("brakeOnStop"), FactMetaData::valueTypeBool);
    Fact _antiStallFact             = Fact(0, QStringLiteral("antiStall"), FactMetaData::valueTypeBool);
    Fact _telemetry30msFact         = Fact(0, QStringLiteral("telemetry30ms"), FactMetaData::valueTypeBool);
    Fact _lowVoltageCutoffFact      = Fact(0, QStringLiteral("lowVoltageCutoff"), FactMetaData::valueTypeBool);
    Fact _rcCarReversingFact        = Fact(0, QStringLiteral("rcCarReversing"), FactMetaData::valueTypeBool);
    Fact _hallSensorsFact           = Fact(0, QStringLiteral("hallSensors"), FactMetaData::valueTypeBool);
    Fact _autoTimingFact            = Fact(0, QStringLiteral("autoTiming"), FactMetaData::valueTypeBool);
    Fact _disableStickCalibrationFact = Fact(0, QStringLiteral("disableStickCalibration"), FactMetaData::valueTypeBool);

    // Numeric setting facts
    Fact _maxRampSpeedFact          = Fact(0, QStringLiteral("maxRampSpeed"), FactMetaData::valueTypeDouble);
    Fact _minDutyCycleFact          = Fact(0, QStringLiteral("minDutyCycle"), FactMetaData::valueTypeDouble);
    Fact _timingAdvanceFact         = Fact(0, QStringLiteral("timingAdvance"), FactMetaData::valueTypeDouble);
    Fact _pwmFrequencyFact          = Fact(0, QStringLiteral("pwmFrequency"), FactMetaData::valueTypeUint8);
    Fact _startupPowerFact          = Fact(0, QStringLiteral("startupPower"), FactMetaData::valueTypeUint8);
    Fact _motorKvFact               = Fact(0, QStringLiteral("motorKv"), FactMetaData::valueTypeUint32);
    Fact _motorPolesFact            = Fact(0, QStringLiteral("motorPoles"), FactMetaData::valueTypeUint8);
    Fact _beepVolumeFact            = Fact(0, QStringLiteral("beepVolume"), FactMetaData::valueTypeUint8);
    Fact _activeBrakePowerFact      = Fact(0, QStringLiteral("activeBrakePower"), FactMetaData::valueTypeUint8);
    Fact _dragBrakeStrengthFact     = Fact(0, QStringLiteral("dragBrakeStrength"), FactMetaData::valueTypeUint8);
    Fact _runningBrakeAmountFact    = Fact(0, QStringLiteral("runningBrakeAmount"), FactMetaData::valueTypeUint8);
    Fact _temperatureLimitFact      = Fact(0, QStringLiteral("temperatureLimit"), FactMetaData::valueTypeUint8);
    Fact _currentLimitFact          = Fact(0, QStringLiteral("currentLimit"), FactMetaData::valueTypeDouble);
    Fact _lowVoltageThresholdFact   = Fact(0, QStringLiteral("lowVoltageThreshold"), FactMetaData::valueTypeDouble);
    Fact _sineModeRangeFact         = Fact(0, QStringLiteral("sineModeRange"), FactMetaData::valueTypeUint8);
    Fact _sineModeStrengthFact      = Fact(0, QStringLiteral("sineModeStrength"), FactMetaData::valueTypeUint8);
    Fact _inputTypeFact             = Fact(0, QStringLiteral("inputType"), FactMetaData::valueTypeUint8);
    Fact _currentPidPFact           = Fact(0, QStringLiteral("currentPidP"), FactMetaData::valueTypeDouble);
    Fact _currentPidIFact           = Fact(0, QStringLiteral("currentPidI"), FactMetaData::valueTypeUint8);
    Fact _currentPidDFact           = Fact(0, QStringLiteral("currentPidD"), FactMetaData::valueTypeDouble);
    Fact _absoluteVoltageCutoffFact = Fact(0, QStringLiteral("absoluteVoltageCutoff"), FactMetaData::valueTypeDouble);
    Fact _servoLowThresholdFact     = Fact(0, QStringLiteral("servoLowThreshold"), FactMetaData::valueTypeDouble);
    Fact _servoHighThresholdFact    = Fact(0, QStringLiteral("servoHighThreshold"), FactMetaData::valueTypeDouble);
    Fact _servoNeutralFact          = Fact(0, QStringLiteral("servoNeutral"), FactMetaData::valueTypeDouble);
    Fact _servoDeadbandFact         = Fact(0, QStringLiteral("servoDeadband"), FactMetaData::valueTypeUint8);

    bool _dataLoaded = false;
    int _escIndex = 0;

    // Change tracking
    QSet<int> _modifiedBytes;              // Set of byte indices that have been modified
    QByteArray _originalEepromData;        // Original EEPROM data from last read

    // Maps for efficient lookups
    QHash<QString, Fact*> _factsByName;    // Map fact names to fact pointers
    QHash<Fact*, int> _factToByteIndex;    // Map facts to their EEPROM byte index

    // Pending changes not yet written
    QVariantMap _pendingChanges;           // Map of fact name to pending value
};
