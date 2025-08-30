/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AM32EepromFactGroup.h"
#include "Vehicle.h"
#include <QtCore/QDebug>

void AM32EepromFactGroup::instantiateEepromFacts()
{
    // QmlObjectListModel _am32_settings
    // AM32Setting(int escIndex, QString name, FactMetaData::ValueType_t type, uint8_t eepromByteIndex)

    // ---------------
    // 48 bytes total
    // ---------------
    // 5 read-only
    // 4 reserved
    // 39 settings
    auto setting = new AM32SettingConfig("maxRampSpeed", FactMetaData::valueTypeDouble, 5); // % duty cycle per ms (value / 10) (default 160 == 16%)
    auto setting = new AM32SettingConfig("minDutyCycle", FactMetaData::valueTypeDouble, 6); // % min duty cycle (value / 2) (default 4 == 2%)
    auto setting = new AM32SettingConfig("disableStickCalibration", FactMetaData::valueTypeBool, 7); // disable stick based calibration (default 0)
    auto setting = new AM32SettingConfig("absoluteVoltageCutoff", FactMetaData::valueTypeDouble, 8); // voltage level 1 to 100 in 0.5v increments (default 10)
    auto setting = new AM32SettingConfig("currentPidP", FactMetaData::valueTypeUint32, 9); // current PID P value x 2 (default 100 = 200)
    auto setting = new AM32SettingConfig("currentPidI", FactMetaData::valueTypeUint32, 10); // current control I value (default 0)
    auto setting = new AM32SettingConfig("currentPidD", FactMetaData::valueTypeUint32, 11); // current control D value x 10 (default 50 = 500)
    auto setting = new AM32SettingConfig("activeBrakePower", FactMetaData::valueTypeUint8, 12); // 1-5 percent duty cycle (default 2)
    auto setting = new AM32SettingConfig("directionReversed", FactMetaData::valueTypeBool, 17); // direction reversed (default 0)
    auto setting = new AM32SettingConfig("bidirectionalMode", FactMetaData::valueTypeBool, 18); // bidirectional mode (default 0)
    auto setting = new AM32SettingConfig("sineStartup", FactMetaData::valueTypeBool, 19); // sinusoidal startup (default 0)
    auto setting = new AM32SettingConfig("complementaryPwm", FactMetaData::valueTypeBool, 20); // complementary pwm (default 1)
    auto setting = new AM32SettingConfig("variablePwmFreq", FactMetaData::valueTypeBool, 21); // variable pwm frequency (default 1)
    auto setting = new AM32SettingConfig("stuckRotorProtection", FactMetaData::valueTypeBool, 22); // stuck rotor protection (default 1)
    auto setting = new AM32SettingConfig("timingAdvance", FactMetaData::valueTypeDouble, 23); // timing advance x0.9375, ei 16 = 15 degrees (default 26 == 24.375)
    auto setting = new AM32SettingConfig("pwmFrequency", FactMetaData::valueTypeUint8, 24); // pwm frequency mutiples of 1k (8 to 144)(default 24 == 24khz)
    auto setting = new AM32SettingConfig("startupPower", FactMetaData::valueTypeUint8, 25); // startup power 50-150 percent (default 100)
    auto setting = new AM32SettingConfig("motorKv", FactMetaData::valueTypeUint32, 26); // motor KV in increments of 40 (default 55 = 2200kv)
    auto setting = new AM32SettingConfig("motorPoles", FactMetaData::valueTypeUint8, 27); // motor poles (default 14)
    auto setting = new AM32SettingConfig("brakeOnStop", FactMetaData::valueTypeBool, 28); // brake on stop (default 0)
    auto setting = new AM32SettingConfig("antiStall", FactMetaData::valueTypeBool, 29); // anti stall protection, throttle boost at low rpm (default 0)
    auto setting = new AM32SettingConfig("beepVolume", FactMetaData::valueTypeUint8, 30); // beep volume, range 0 to 11 (default 5)
    auto setting = new AM32SettingConfig("telemetry30ms", FactMetaData::valueTypeBool, 31); // 30 Millisecond telemetry output (default 0)
    auto setting = new AM32SettingConfig("servoLowThreshold", FactMetaData::valueTypeUint32, 32); // servo low value =  (value * 2) + 750us (default 128 == 1006)
    auto setting = new AM32SettingConfig("servoHighThreshold", FactMetaData::valueTypeUint32, 33); // servo high value = (value * 2) + 1750us (default 128 == 1006)
    auto setting = new AM32SettingConfig("servoNeutral", FactMetaData::valueTypeUint32, 34); // servo neutral base 1374 + value Us. IE 128 = 1500 us. (default 128 == 1500)
    auto setting = new AM32SettingConfig("servoDeadband", FactMetaData::valueTypeUint8, 35); // servo dead band 0-100, applied to either side of neutral (default 50)
    auto setting = new AM32SettingConfig("lowVoltageCutoff", FactMetaData::valueTypeBool, 36); // low voltage cuttoff (default 0)
    auto setting = new AM32SettingConfig("lowVoltageThreshold", FactMetaData::valueTypeDouble, 37); // low voltage threshold, ((value + 250) / 100), (default 50 == 3.0v)
    auto setting = new AM32SettingConfig("rcCarReversing", FactMetaData::valueTypeBool, 38); // rc car type reversing, brake on first aplication return to center to reverse (default 0)
    auto setting = new AM32SettingConfig("hallSensors", FactMetaData::valueTypeBool, 39); // Hall sensors if equipped  (default 0)
    auto setting = new AM32SettingConfig("sineModeRange", FactMetaData::valueTypeUint8, 40); // Sine Mode Range 5-25 percent of throttle (default 15)
    auto setting = new AM32SettingConfig("dragBrakeStrength", FactMetaData::valueTypeUint8, 41); // Drag Brake Strength 1-10 , 10 being full strength (default 10)
    auto setting = new AM32SettingConfig("runningBrakeLevel", FactMetaData::valueTypeUint8, 42); // amount of brake to use when the motor is running (default 10)
    auto setting = new AM32SettingConfig("temperatureLimit", FactMetaData::valueTypeUint8, 43); // temperature limit 70-140 degrees C. above 140 disables (default 141)
    auto setting = new AM32SettingConfig("currentLimit", FactMetaData::valueTypeUint8, 44); // current protection level (value * 2) above 100 disables (default 102 == disabled, > 200A)
    auto setting = new AM32SettingConfig("sineModePower", FactMetaData::valueTypeUint8, 45); // sine mode strength 1-10 (default 6)
    auto setting = new AM32SettingConfig("inputType", FactMetaData::valueTypeUint8, 46); // input type selector 1:Auto 2:Dshot 3:Servo 4:PWM 5:Serial 6:BetaFlightSafeArming (default 1)
    auto setting = new AM32SettingConfig("autoTiming", FactMetaData::valueTypeUint8, 47); // auto timing advance (0 - 32)(degrees)(default 0)




    for (auto f : ) {
        _am32_settings.append(setting);
    }
}

AM32EepromFactGroup::AM32EepromFactGroup(QObject* parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/AM32EepromFact.json"), parent)
{
    // Iterate over structure, create facts, call _addFact(ptr)


    // Add all facts to the group
    _addFact(&_firmwareMajorFact);
    _addFact(&_firmwareMinorFact);
    _addFact(&_bootloaderVersionFact);
    _addFact(&_eepromVersionFact);

    _addFact(&_directionReversedFact);
    _addFact(&_bidirectionalModeFact);
    _addFact(&_sineStartupFact);
    _addFact(&_complementaryPwmFact);
    _addFact(&_variablePwmFreqFact);
    _addFact(&_stuckRotorProtectionFact);
    _addFact(&_brakeOnStopFact);
    _addFact(&_antiStallFact);
    _addFact(&_telemetry30msFact);
    _addFact(&_lowVoltageCutoffFact);
    _addFact(&_rcCarReversingFact);
    _addFact(&_hallSensorsFact);
    _addFact(&_autoTimingFact);
    _addFact(&_disableStickCalibrationFact);

    _addFact(&_maxRampSpeedFact);
    _addFact(&_minDutyCycleFact);
    _addFact(&_timingAdvanceFact);
    _addFact(&_pwmFrequencyFact);
    _addFact(&_startupPowerFact);
    _addFact(&_motorKvFact);
    _addFact(&_motorPolesFact);
    _addFact(&_beepVolumeFact);
    _addFact(&_activeBrakePowerFact);
    _addFact(&_dragBrakeStrengthFact);
    _addFact(&_runningBrakeAmountFact);
    _addFact(&_temperatureLimitFact);
    _addFact(&_currentLimitFact);
    _addFact(&_lowVoltageThresholdFact);
    _addFact(&_sineModeRangeFact);
    _addFact(&_sineModeStrengthFact);
    _addFact(&_inputTypeFact);
    _addFact(&_currentPidPFact);
    _addFact(&_currentPidIFact);
    _addFact(&_currentPidDFact);
    _addFact(&_absoluteVoltageCutoffFact);
    _addFact(&_servoLowThresholdFact);
    _addFact(&_servoHighThresholdFact);
    _addFact(&_servoNeutralFact);
    _addFact(&_servoDeadbandFact);

    // Initialize the byte mapping
    _initializeByteMapping();
}

void AM32EepromFactGroup::_initializeByteMapping()
{
    // Build the name-to-fact map for all facts
    auto addToMaps = [this](Fact* fact, int byteIndex = -1) {
        _factsByName[fact->name()] = fact;
        if (byteIndex >= 0) {
            _factToByteIndex[fact] = byteIndex;
        }
    };

    // Read-only facts (no byte index)
    addToMaps(&_firmwareMajorFact);
    addToMaps(&_firmwareMinorFact);
    addToMaps(&_bootloaderVersionFact);
    addToMaps(&_eepromVersionFact);

    // Editable facts with their byte indices
    const QList<ByteMapping> mappings = {
        {&_maxRampSpeedFact, BYTE_MAX_RAMP_SPEED},
        {&_minDutyCycleFact, BYTE_MIN_DUTY_CYCLE},
        {&_disableStickCalibrationFact, BYTE_STICK_CALIBRATION},
        {&_absoluteVoltageCutoffFact, BYTE_VOLTAGE_CUTOFF},
        {&_currentPidPFact, BYTE_CURRENT_PID_P},
        {&_currentPidIFact, BYTE_CURRENT_PID_I},
        {&_currentPidDFact, BYTE_CURRENT_PID_D},
        {&_activeBrakePowerFact, BYTE_ACTIVE_BRAKE_POWER},
        {&_directionReversedFact, BYTE_DIR_REVERSED},
        {&_bidirectionalModeFact, BYTE_BI_DIRECTION},
        {&_sineStartupFact, BYTE_USE_SINE_START},
        {&_complementaryPwmFact, BYTE_COMP_PWM},
        {&_variablePwmFreqFact, BYTE_VARIABLE_PWM},
        {&_stuckRotorProtectionFact, BYTE_STUCK_ROTOR},
        {&_timingAdvanceFact, BYTE_TIMING_ADVANCE},
        {&_pwmFrequencyFact, BYTE_PWM_FREQUENCY},
        {&_startupPowerFact, BYTE_STARTUP_POWER},
        {&_motorKvFact, BYTE_MOTOR_KV},
        {&_motorPolesFact, BYTE_MOTOR_POLES},
        {&_brakeOnStopFact, BYTE_BRAKE_ON_STOP},
        {&_antiStallFact, BYTE_ANTI_STALL},
        {&_beepVolumeFact, BYTE_BEEP_VOLUME},
        {&_telemetry30msFact, BYTE_TELEMETRY_30MS},
        {&_servoLowThresholdFact, BYTE_SERVO_LOW},
        {&_servoHighThresholdFact, BYTE_SERVO_HIGH},
        {&_servoNeutralFact, BYTE_SERVO_NEUTRAL},
        {&_servoDeadbandFact, BYTE_SERVO_DEADBAND},
        {&_lowVoltageCutoffFact, BYTE_LOW_VOLTAGE_CUTOFF},
        {&_lowVoltageThresholdFact, BYTE_LOW_VOLTAGE_THRESHOLD},
        {&_rcCarReversingFact, BYTE_RC_CAR_REVERSING},
        {&_hallSensorsFact, BYTE_HALL_SENSORS},
        {&_sineModeRangeFact, BYTE_SINE_MODE_RANGE},
        {&_dragBrakeStrengthFact, BYTE_DRAG_BRAKE},
        {&_runningBrakeAmountFact, BYTE_RUNNING_BRAKE},
        {&_temperatureLimitFact, BYTE_TEMP_LIMIT},
        {&_currentLimitFact, BYTE_CURRENT_LIMIT},
        {&_sineModeStrengthFact, BYTE_SINE_MODE_STRENGTH},
        {&_inputTypeFact, BYTE_INPUT_TYPE},
        {&_autoTimingFact, BYTE_AUTO_TIMING}
    };

    for (const auto& mapping : mappings) {
        addToMaps(mapping.fact, mapping.byteIndex);
    }
}

void AM32EepromFactGroup::setEscIndex(int index)
{
    if (_escIndex != index) {
        _escIndex = index;
        emit escIndexChanged();
    }
}

void AM32EepromFactGroup::handleEepromData(const uint8_t* data, int length)
{
    if (length < 48) {
        qWarning() << "AM32 EEPROM data too short:" << length;
        return;
    }

    // Store original data for comparison
    _originalEepromData = QByteArray(reinterpret_cast<const char*>(data), length);

    // Clear pending changes since we're loading fresh data
    _pendingChanges.clear();
    _modifiedBytes.clear();
    _originalValues.clear();

    // Parse read-only info
    _eepromVersionFact.setRawValue(data[1]);
    _bootloaderVersionFact.setRawValue(data[2]);
    _firmwareMajorFact.setRawValue(data[3]);
    _firmwareMinorFact.setRawValue(data[4]);

    // Store original values for read-only facts
    _originalValues["eepromVersion"] = data[1];
    _originalValues["bootloaderVersion"] = data[2];
    _originalValues["firmwareMajor"] = data[3];
    _originalValues["firmwareMinor"] = data[4];

    // Parse configurable settings with proper conversions
    double maxRampSpeed = data[5] / 10.0;  // value/10 percent per ms
    double minDutyCycle = data[6] / 2.0;   // value/2 percent
    _maxRampSpeedFact.setRawValue(maxRampSpeed);
    _minDutyCycleFact.setRawValue(minDutyCycle);
    _originalValues["maxRampSpeed"] = maxRampSpeed;
    _originalValues["minDutyCycle"] = minDutyCycle;

    // Stick calibration setting
    bool disableStickCalibration = (data[7] != 0);
    _disableStickCalibrationFact.setRawValue(disableStickCalibration);
    _originalValues["disableStickCalibration"] = disableStickCalibration;

    // Voltage and PID settings
    double absoluteVoltageCutoff = data[8] * 0.5;  // value * 0.5 volts
    double currentPidP = data[9] * 2.0;    // P value x2
    int currentPidI = data[10];         // I value
    double currentPidD = data[11] * 10.0;  // D value x10
    int activeBrakePower = data[12];

    _absoluteVoltageCutoffFact.setRawValue(absoluteVoltageCutoff);
    _currentPidPFact.setRawValue(currentPidP);
    _currentPidIFact.setRawValue(currentPidI);
    _currentPidDFact.setRawValue(currentPidD);
    _activeBrakePowerFact.setRawValue(activeBrakePower);

    _originalValues["absoluteVoltageCutoff"] = absoluteVoltageCutoff;
    _originalValues["currentPidP"] = currentPidP;
    _originalValues["currentPidI"] = currentPidI;
    _originalValues["currentPidD"] = currentPidD;
    _originalValues["activeBrakePower"] = activeBrakePower;

    // Boolean flags
    bool directionReversed = (data[17] != 0);
    bool bidirectionalMode = (data[18] != 0);
    bool sineStartup = (data[19] != 0);
    bool complementaryPwm = (data[20] != 0);
    bool variablePwmFreq = (data[21] != 0);
    bool stuckRotorProtection = (data[22] != 0);

    _directionReversedFact.setRawValue(directionReversed);
    _bidirectionalModeFact.setRawValue(bidirectionalMode);
    _sineStartupFact.setRawValue(sineStartup);
    _complementaryPwmFact.setRawValue(complementaryPwm);
    _variablePwmFreqFact.setRawValue(variablePwmFreq);
    _stuckRotorProtectionFact.setRawValue(stuckRotorProtection);

    _originalValues["directionReversed"] = directionReversed;
    _originalValues["bidirectionalMode"] = bidirectionalMode;
    _originalValues["sineStartup"] = sineStartup;
    _originalValues["complementaryPwm"] = complementaryPwm;
    _originalValues["variablePwmFreq"] = variablePwmFreq;
    _originalValues["stuckRotorProtection"] = stuckRotorProtection;

    // Timing and power
    double timingAdvance;
    if (data[23] < 10) {
        // Old format: 0-3 mapped to 0-24 degrees
        timingAdvance = data[23] * 8 * 0.9375;
    } else if (data[23] <= 42) {
        // New format: subtract 10 and multiply by 0.9375
        timingAdvance = (data[23] - 10) * 0.9375;
    } else {
        // Default to 15 degrees
        timingAdvance = 15.0;
    }
    _timingAdvanceFact.setRawValue(timingAdvance);
    _originalValues["timingAdvance"] = timingAdvance;

    int pwmFrequency = data[24];
    int startupPower = data[25];
    int motorKv = data[26] * 40 + 20;  // KV in increments of 40
    int motorPoles = data[27];
    bool brakeOnStop = (data[28] != 0);
    bool antiStall = (data[29] != 0);
    int beepVolume = data[30];
    bool telemetry30ms = (data[31] != 0);

    _pwmFrequencyFact.setRawValue(pwmFrequency);
    _startupPowerFact.setRawValue(startupPower);
    _motorKvFact.setRawValue(motorKv);
    _motorPolesFact.setRawValue(motorPoles);
    _brakeOnStopFact.setRawValue(brakeOnStop);
    _antiStallFact.setRawValue(antiStall);
    _beepVolumeFact.setRawValue(beepVolume);
    _telemetry30msFact.setRawValue(telemetry30ms);

    _originalValues["pwmFrequency"] = pwmFrequency;
    _originalValues["startupPower"] = startupPower;
    _originalValues["motorKv"] = motorKv;
    _originalValues["motorPoles"] = motorPoles;
    _originalValues["brakeOnStop"] = brakeOnStop;
    _originalValues["antiStall"] = antiStall;
    _originalValues["beepVolume"] = beepVolume;
    _originalValues["telemetry30ms"] = telemetry30ms;

    // Servo settings
    double servoLowThreshold = data[32] * 2 + 750;   // (value*2) + 750us
    double servoHighThreshold = data[33] * 2 + 1750; // (value*2) + 1750us
    double servoNeutral = data[34] + 1374;           // 1374 + value us
    int servoDeadband = data[35];                 // direct value

    _servoLowThresholdFact.setRawValue(servoLowThreshold);
    _servoHighThresholdFact.setRawValue(servoHighThreshold);
    _servoNeutralFact.setRawValue(servoNeutral);
    _servoDeadbandFact.setRawValue(servoDeadband);

    _originalValues["servoLowThreshold"] = servoLowThreshold;
    _originalValues["servoHighThreshold"] = servoHighThreshold;
    _originalValues["servoNeutral"] = servoNeutral;
    _originalValues["servoDeadband"] = servoDeadband;

    // Protection settings
    bool lowVoltageCutoff = (data[36] != 0);
    double lowVoltageThreshold = (data[37] + 250) / 100.0;  // (value+250)/100 V/cell
    bool rcCarReversing = (data[38] != 0);
    bool hallSensors = (data[39] != 0);

    _lowVoltageCutoffFact.setRawValue(lowVoltageCutoff);
    _lowVoltageThresholdFact.setRawValue(lowVoltageThreshold);
    _rcCarReversingFact.setRawValue(rcCarReversing);
    _hallSensorsFact.setRawValue(hallSensors);

    _originalValues["lowVoltageCutoff"] = lowVoltageCutoff;
    _originalValues["lowVoltageThreshold"] = lowVoltageThreshold;
    _originalValues["rcCarReversing"] = rcCarReversing;
    _originalValues["hallSensors"] = hallSensors;

    // Sine mode and brake settings
    int sineModeRange = data[40];
    int dragBrakeStrength = data[41];
    int runningBrakeAmount = data[42];
    int temperatureLimit = data[43];  // 141 = disabled
    double currentLimit = data[44];
    int sineModeStrength = data[45];
    int inputType = data[46];
    bool autoTiming = (data[47] != 0);

    _sineModeRangeFact.setRawValue(sineModeRange);
    _dragBrakeStrengthFact.setRawValue(dragBrakeStrength);
    _runningBrakeAmountFact.setRawValue(runningBrakeAmount);
    _temperatureLimitFact.setRawValue(temperatureLimit);
    _currentLimitFact.setRawValue(currentLimit);
    _sineModeStrengthFact.setRawValue(sineModeStrength);
    _inputTypeFact.setRawValue(inputType);
    _autoTimingFact.setRawValue(autoTiming);

    _originalValues["sineModeRange"] = sineModeRange;
    _originalValues["dragBrakeStrength"] = dragBrakeStrength;
    _originalValues["runningBrakeAmount"] = runningBrakeAmount;
    _originalValues["temperatureLimit"] = temperatureLimit;
    _originalValues["currentLimit"] = currentLimit;
    _originalValues["sineModeStrength"] = sineModeStrength;
    _originalValues["inputType"] = inputType;
    _originalValues["autoTiming"] = autoTiming;

    clearPendingChanges();

    _dataLoaded = true;
    emit dataLoadedChanged();

    qDebug() << "ESC" << (_escIndex + 1) << " received eeprom data";
}

void AM32EepromFactGroup::applyPendingChanges(const QVariantMap& changes)
{
    // Apply the pending changes from the UI
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        const QString& factName = it.key();
        const QVariant& value = it.value();

        qDebug() << "applyPendingChanges";
        qDebug() << "ESC " << _escIndex << factName << value;
        _pendingChanges[factName] = value;

        // Track which byte was modified
        Fact* fact = _factsByName.value(factName, nullptr);
        if (fact && _factToByteIndex.contains(fact)) {
            _modifiedBytes.insert(_factToByteIndex[fact]);

            // Special case: low voltage threshold affects cutoff byte too
            if (fact == &_lowVoltageThresholdFact) {
                // Check if low voltage cutoff is or will be enabled
                bool cutoffEnabled = _pendingChanges.contains("lowVoltageCutoff")
                    ? _pendingChanges["lowVoltageCutoff"].toBool()
                    : _lowVoltageCutoffFact.rawValue().toBool();
                if (cutoffEnabled) {
                    _modifiedBytes.insert(BYTE_LOW_VOLTAGE_CUTOFF);
                }
            }
        }
    }

    emit pendingChangesUpdated();
    emit hasUnsavedChangesChanged();
}

void AM32EepromFactGroup::clearPendingChanges()
{
    _pendingChanges.clear();
    _modifiedBytes.clear();
    emit hasUnsavedChangesChanged();
    emit pendingChangesUpdated();
    qDebug() << "ESC" << (_escIndex + 1) << "clearPendingChanges, pendingChangesUpdated";
}

void AM32EepromFactGroup::clearPendingChange(const QString& factName)
{
    // Remove the specific fact from pending changes
    if (_pendingChanges.contains(factName)) {
        _pendingChanges.remove(factName);

        // Also remove from modified bytes if present
        Fact* fact = _factsByName.value(factName, nullptr);
        if (fact && _factToByteIndex.contains(fact)) {
            _modifiedBytes.remove(_factToByteIndex[fact]);

            // Special case: if clearing low voltage threshold, also clear cutoff byte
            if (fact == &_lowVoltageThresholdFact) {
                _modifiedBytes.remove(BYTE_LOW_VOLTAGE_CUTOFF);
            }
        }

        // Restore the fact to its original value
        if (fact && _originalValues.contains(factName)) {
            fact->setRawValue(_originalValues[factName]);
        }

        emit pendingChangesUpdated();
        emit hasUnsavedChangesChanged();
    }
}

void AM32EepromFactGroup::discardChanges()
{
    // Revert facts back to their current hardware values (re-parse EEPROM data)
    if (!_originalEepromData.isEmpty() && _dataLoaded) {
        handleEepromData(reinterpret_cast<const uint8_t*>(_originalEepromData.data()),
                         _originalEepromData.size());
    }
}

bool AM32EepromFactGroup::settingsMatch(AM32EepromFactGroup* other) const
{
    if (!other || !other->dataLoaded() || !_dataLoaded) {
        return false;
    }

    // Compare all editable facts
    for (auto it = _factToByteIndex.begin(); it != _factToByteIndex.end(); ++it) {
        Fact* myFact = it.key();
        Fact* otherFact = other->_factsByName.value(myFact->name(), nullptr);

        if (!otherFact) {
            return false;
        }

        // Get the effective values (including pending changes)
        QVariant myValue = getFactValue(myFact->name());
        QVariant otherValue = other->getFactValue(myFact->name());

        if (myValue != otherValue) {
            return false;
        }
    }

    return true;
}

QVariant AM32EepromFactGroup::getFactValue(const QString& factName) const
{
    // If there's a pending change, return that, otherwise return the fact value
    if (_pendingChanges.contains(factName)) {
        return _pendingChanges[factName];
    }

    Fact* fact = _factsByName.value(factName, nullptr);
    if (fact) {
        return fact->rawValue();
    }

    return QVariant();
}

QVariant AM32EepromFactGroup::getOriginalValue(const QString& factName) const
{
    // Return the original loaded value for the fact
    return _originalValues.value(factName, QVariant());
}

Fact* AM32EepromFactGroup::_getFactByName(const QString& name) const
{
    return _factsByName.value(name, nullptr);
}

void AM32EepromFactGroup::calculateWriteMask(uint32_t writeMask[6]) const
{
    // Initialize mask to all zeros
    memset(writeMask, 0, 6 * sizeof(uint32_t));

    // Set bits only for modified bytes
    for (int byteIndex : _modifiedBytes) {
        if (byteIndex < 192) {
            int maskIndex = byteIndex / 32;
            int bitIndex = byteIndex % 32;
            writeMask[maskIndex] |= (1U << bitIndex);
        }
    }

    // Never write to read-only bytes 0-4
    writeMask[0] &= 0xFFFFFFE0;  // Clear bits 0-4
}

QByteArray AM32EepromFactGroup::packEepromData() const
{
    QByteArray packed(192, 0);  // Initialize with zeros

    // Helper to get value with pending changes
    auto getValue = [this](const QString& name) -> QVariant {
        if (_pendingChanges.contains(name)) {
            return _pendingChanges[name];
        }
        Fact* fact = _factsByName.value(name, nullptr);
        return fact ? fact->rawValue() : QVariant();
    };

    // Start byte must be 1
    packed[0] = 1;

    // Read-only values (keep existing)
    packed[1] = _eepromVersionFact.rawValue().toUInt();
    packed[2] = _bootloaderVersionFact.rawValue().toUInt();
    packed[3] = _firmwareMajorFact.rawValue().toUInt();
    packed[4] = _firmwareMinorFact.rawValue().toUInt();

    // Configurable settings with proper conversions
    packed[5] = static_cast<uint8_t>(getValue("maxRampSpeed").toDouble() * 10);  // %/ms to value/10
    packed[6] = static_cast<uint8_t>(getValue("minDutyCycle").toDouble() * 2);   // % to value/2
    packed[7] = getValue("disableStickCalibration").toBool() ? 1 : 0;
    packed[8] = static_cast<uint8_t>(getValue("absoluteVoltageCutoff").toDouble() / 0.5);  // V to value/0.5
    packed[9] = static_cast<uint8_t>(getValue("currentPidP").toDouble() / 2);    // P value / 2
    packed[10] = getValue("currentPidI").toUInt();
    packed[11] = static_cast<uint8_t>(getValue("currentPidD").toDouble() / 10);  // D value / 10
    packed[12] = getValue("activeBrakePower").toUInt();

    // Reserved bytes 13-16

    // Boolean flags
    packed[17] = getValue("directionReversed").toBool() ? 1 : 0;
    packed[18] = getValue("bidirectionalMode").toBool() ? 1 : 0;
    packed[19] = getValue("sineStartup").toBool() ? 1 : 0;
    packed[20] = getValue("complementaryPwm").toBool() ? 1 : 0;
    packed[21] = getValue("variablePwmFreq").toBool() ? 1 : 0;
    packed[22] = getValue("stuckRotorProtection").toBool() ? 1 : 0;

    // Timing advance - convert degrees to AM32 format
    double timingDegrees = getValue("timingAdvance").toDouble();
    packed[23] = static_cast<uint8_t>((timingDegrees / 0.9375) + 10);  // New format

    packed[24] = getValue("pwmFrequency").toUInt();
    packed[25] = getValue("startupPower").toUInt();
    packed[26] = static_cast<uint8_t>((getValue("motorKv").toUInt() - 20) / 40);  // KV to increments
    packed[27] = getValue("motorPoles").toUInt();
    packed[28] = getValue("brakeOnStop").toBool() ? 1 : 0;
    packed[29] = getValue("antiStall").toBool() ? 1 : 0;
    packed[30] = getValue("beepVolume").toUInt();
    packed[31] = getValue("telemetry30ms").toBool() ? 1 : 0;

    // Servo settings
    packed[32] = static_cast<uint8_t>((getValue("servoLowThreshold").toDouble() - 750) / 2);   // (us - 750) / 2
    packed[33] = static_cast<uint8_t>((getValue("servoHighThreshold").toDouble() - 1750) / 2); // (us - 1750) / 2
    packed[34] = static_cast<uint8_t>(getValue("servoNeutral").toDouble() - 1374);             // us - 1374
    packed[35] = getValue("servoDeadband").toUInt();                                           // direct value

    // Protection settings
    packed[36] = getValue("lowVoltageCutoff").toBool() ? 1 : 0;
    packed[37] = static_cast<uint8_t>(getValue("lowVoltageThreshold").toDouble() * 100 - 250);  // V/cell to value
    packed[38] = getValue("rcCarReversing").toBool() ? 1 : 0;
    packed[39] = getValue("hallSensors").toBool() ? 1 : 0;

    // Sine mode and brake settings
    packed[40] = getValue("sineModeRange").toUInt();
    packed[41] = getValue("dragBrakeStrength").toUInt();
    packed[42] = getValue("runningBrakeAmount").toUInt();

    // Temperature and current limits
    packed[43] = getValue("temperatureLimit").toUInt();
    packed[44] = getValue("currentLimit").toUInt() / 2;

    packed[45] = getValue("sineModeStrength").toUInt();
    packed[46] = getValue("inputType").toUInt();
    packed[47] = getValue("autoTiming").toBool() ? 1 : 0;

    return packed;
}

void AM32EepromFactGroup::requestReadAll(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    // Send MAV_CMD_AM32_REQUEST_EEPROM
    vehicle->sendMavCommand(
        vehicle->defaultComponentId(),
        MAV_CMD_AM32_REQUEST_EEPROM,
        false,  // showError
        255,  // param1: ESC index -- 255 means read all
        0, 0, 0, 0, 0, 0  // unused params
    );
}

void AM32EepromFactGroup::requestWrite(Vehicle* vehicle)
{
    if (!vehicle || _modifiedBytes.isEmpty()) {
        return;
    }

    // Pack current settings
    QByteArray packedData = packEepromData();

    // Calculate write mask based on modified bytes
    uint32_t writeMask[6];
    calculateWriteMask(writeMask);

    // Log which bytes we're writing
    qDebug() << "ESC" << (_escIndex + 1);
    qDebug() << "Writing AM32 EEPROM bytes:" << _modifiedBytes;
    qDebug() << "Write mask:" << Qt::hex
             << writeMask[0] << writeMask[1] << writeMask[2]
             << writeMask[3] << writeMask[4] << writeMask[5];
    qDebug() << "\n";

    // Send AM32_EEPROM message with write mode
    mavlink_message_t msg;
    mavlink_am32_eeprom_t eeprom;

    eeprom.target_system = vehicle->id();
    eeprom.target_component = vehicle->defaultComponentId();
    eeprom.index = _escIndex;
    eeprom.mode = 1;  // Write mode
    memcpy(eeprom.write_mask, writeMask, sizeof(writeMask));
    eeprom.length = qMin(packedData.size(), (int)sizeof(eeprom.data));
    memcpy(eeprom.data, packedData.data(), eeprom.length);

    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (sharedLink) {
        mavlink_msg_am32_eeprom_encode_chan(
            vehicle->id(),
            vehicle->defaultComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            &eeprom
        );

        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);

        // Clear pending changes after successful write
        clearPendingChanges();
    }
}
