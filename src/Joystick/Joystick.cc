/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "Joystick.h"
#include "MavlinkAction.h"
#include "MavlinkActionManager.h"
#include "MavlinkActionsSettings.h"
#include "FirmwarePlugin.h"
#include "GimbalController.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "JoystickManager.h"

#include <QtCore/QSettings>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(JoystickLog, "Joystick.Joystick")
QGC_LOGGING_CATEGORY(JoystickVerboseLog, "Joystick.Joystick:verbose")

/*===========================================================================*/

AssignedButtonAction::AssignedButtonAction(const QString &name)
    : action(name)
{
    // qCDebug(JoystickLog) << Q_FUNC_INFO << this;
}

AssignableButtonAction::AssignableButtonAction(const QString &action_, bool canRepeat_, QObject *parent)
    : QObject(parent)
    , _action(action_)
    , _repeat(canRepeat_)
{
    // qCDebug(JoystickLog) << Q_FUNC_INFO << this;
}

/*===========================================================================*/

Joystick::Joystick(const QString &name, int axisCount, int buttonCount, int hatCount, QObject *parent)
    : QThread(parent)
    , _name(name)
    , _axisCount(axisCount)
    , _buttonCount(buttonCount)
    , _hatCount(hatCount)
    , _hatButtonCount(4 * hatCount)
    , _totalButtonCount(_buttonCount + _hatButtonCount)
    , _rgCalibration(new Calibration_t[static_cast<size_t>(_axisCount)])
    , _rgButtonValues(new uint8_t[static_cast<size_t>(_totalButtonCount)])
    , _mavlinkActionManager(new MavlinkActionManager(SettingsManager::instance()->mavlinkActionsSettings()->joystickActionsFile(), this))
    , _assignableButtonActions(new QmlObjectListModel(this))
    , _metaDataMap(FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/JoystickFactsMetaData.json"), this))
    , _axisFrequencyHzFact(_joystickFactsSettingsGroup, _metaDataMap[_axisFrequencyHzFactName])
    , _buttonFrequencyHzFact(_joystickFactsSettingsGroup, _metaDataMap[_buttonFrequencyHzFactName])
    , _exponentialPctFact(_joystickFactsSettingsGroup, _metaDataMap[_exponentialPctFactName])
{
    qCDebug(JoystickLog)
        << name
        << "axisCount" << axisCount
        << "buttonCount" << buttonCount
        << "hatCount" << hatCount
        << this;

    for (int i = 0; i < _totalButtonCount; i++) {
        _rgButtonValues[i] = BUTTON_UP;
        _buttonActionArray.append(nullptr);
    }

    _loadSettings();

    connect(&_exponentialPctFact, &Fact::rawValueChanged, this, &Joystick::_saveExponentialPctSetting);
}

Joystick::~Joystick()
{
    _exitThread = true;
    wait();

    delete[] _rgCalibration;
    delete[] _rgButtonValues;

    _assignableButtonActions->clearAndDeleteContents();
    qDeleteAll(_buttonActionArray);

    // qCDebug(JoystickLog) << Q_FUNC_INFO << this;
}

void Joystick::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);

    settings.beginGroup(_name);

    bool badSettings = false;
    bool convertOk;

    _calibrated = settings.value(_calibratedSettingsKey, false).toBool();
    _throttleSmoothing = settings.value(_throttleSmoothingSettingsKey, false).toBool();
    _useDeadband = settings.value(_useDeadbandSettingsKey, false).toBool();
    double axisFrequency = settings.value(_axisFrequencySettingsKey, _defaultAxisFrequencyHz).toDouble(&convertOk);
    badSettings |= !convertOk;
    axisFrequency = qMin(qMax(_minAxisFrequencyHz, axisFrequency), _maxAxisFrequencyHz);
    _axisFrequencyHzFact.setRawValue(axisFrequency);
    double buttonFrequency = settings.value(_buttonFrequencySettingsKey, _defaultButtonFrequencyHz).toDouble(&convertOk);
    badSettings |= !convertOk;
    buttonFrequency = qMin(qMax(_minButtonFrequencyHz, buttonFrequency), _maxButtonFrequencyHz);
    _buttonFrequencyHzFact.setRawValue(buttonFrequency);
    _circleCorrection  = settings.value(_circleCorrectionSettingsKey, false).toBool();
    _negativeThrust = settings.value(_negativeThrustSettingsKey, false).toBool();
    _enableManualControlExtensions = settings.value(_manualControlExtensionsEnabledKey, false).toBool();
    _throttleModeCenterZero = settings.value(_throttleModeCenterZeroSettingsKey, 1).toInt(&convertOk);
    badSettings |= !convertOk;
    double exponentialPct = settings.value(_exponentialPctSettingsKey, 0).toDouble(&convertOk);
    badSettings |= !convertOk;
    exponentialPct = qMin(_maxExponentialPct, qMax(_minExponentialPct, exponentialPct));
    _exponentialPctFact.setRawValue(exponentialPct);
    int transmitterMode = settings.value(_transmitterModeSettingsKey, 2).toInt(&convertOk);
    badSettings |= !convertOk;
    transmitterMode = qMin(4, qMax(1, transmitterMode));
    _transmitterMode = transmitterMode;

    qCDebug(JoystickLog) << name();
    // Log loaded settings
    qCDebug(JoystickLog)
        << "    "
        << "calibrated" << _calibrated
        << "throttleSmoothing" << _throttleSmoothing
        << "axisFrequencyHz" << axisFrequency
        << "buttonFrequencyHz" << buttonFrequency
        << "throttleModeCenterZero" << _throttleModeCenterZero
        << "negativeThrust" << _negativeThrust
        << "circleCorrection" << _circleCorrection
        << "exponentialPct" << exponentialPct
        << "enableManualControlExtensions" << _enableManualControlExtensions
        << "useDeadband" << _useDeadband
        << "transmitterMode" << _transmitterMode
        << "badSettings" << badSettings;

    const QString minTpl("Axis%1Min");
    const QString maxTpl("Axis%1Max");
    const QString trimTpl("Axis%1Trim");
    const QString revTpl("Axis%1Rev");
    const QString deadbndTpl("Axis%1Deadbnd");

    for (int axis = 0; axis < _axisCount; axis++) {
        Calibration_t *const calibration = &_rgCalibration[axis];

        calibration->center = settings.value(trimTpl.arg(axis), 0).toInt(&convertOk);
        badSettings |= !convertOk;
        calibration->min = settings.value(minTpl.arg(axis), -32768).toInt(&convertOk);
        badSettings |= !convertOk;
        calibration->max = settings.value(maxTpl.arg(axis), 32767).toInt(&convertOk);
        badSettings |= !convertOk;
        calibration->deadband = settings.value(deadbndTpl.arg(axis), 0).toInt(&convertOk);
        badSettings |= !convertOk;
        calibration->reversed = settings.value(revTpl.arg(axis), false).toBool();

        qDebug(JoystickLog)
        << "    "
            << "axis" << axis
            << "min" << calibration->min
            << "max" << calibration->max
            << "center" << calibration->center
            << "reversed" << calibration->reversed
            << "deadband" << calibration->deadband
            << "badSettings" << badSettings;
    }

    int workingAxis = 0;
    for (int function = 0; function < maxAxisFunction; function++) {
        int functionAxis = settings.value(_rgFunctionSettingsKey[function], -1).toInt(&convertOk);
        badSettings |= (!convertOk || (functionAxis >= _axisCount));

        if (functionAxis >= 0) {
            workingAxis++;
        }

        if (functionAxis < _axisCount) {
            _rgFunctionAxis[function] = functionAxis;
        }

        qCDebug(JoystickLog)
            << "    "
            << axisFunctionToString(static_cast<AxisFunction_t>(function))
            << "functionAxis" << functionAxis
            << "badSettings" << badSettings;
    }

    badSettings |= (workingAxis < 4);

    // FunctionAxis mappings are always stored in TX mode 2
    // Remap to stored TX mode in settings
    _remapAxes (2, _transmitterMode, _rgFunctionAxis);

    for (int button = 0; button < _totalButtonCount; button++) {
        const QString buttonAction = settings.value(QString(_buttonActionNameSettingsKey).arg(button), QString()).toString();
        if (buttonAction.isEmpty() || (buttonAction == _buttonActionNone)) {
            continue;
        }

        if (_buttonActionArray[button]) {
            delete _buttonActionArray[button];
            _buttonActionArray[button] = nullptr;
        }

        AssignedButtonAction *const ap = new AssignedButtonAction(buttonAction);
        ap->repeat = settings.value(QString(_buttonActionRepeatSettingsKey).arg(button), false).toBool();
        _buttonActionArray[button] = ap;
        _buttonActionArray[button]->buttonTime.start();

        qCDebug(JoystickLog)
            << "    "
            << "button" << button
            << _buttonActionArray[button]->action
            << "repeat" << _buttonActionArray[button]->repeat;
    }

    if (badSettings) {
        _calibrated = false;
        settings.setValue(_calibratedSettingsKey, false);
    }
}

void Joystick::_saveButtonSettings()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);

    for (int button = 0; button < _totalButtonCount; button++) {
        if (_buttonActionArray[button]) {
            settings.setValue(QString(_buttonActionNameSettingsKey).arg(button), _buttonActionArray[button]->action);
            settings.setValue(QString(_buttonActionRepeatSettingsKey).arg(button), _buttonActionArray[button]->repeat);
            qCDebug(JoystickLog) << Q_FUNC_INFO << "button:action:repeat" << button <<  _buttonActionArray[button]->action << _buttonActionArray[button]->repeat;
        }
    }
}

void Joystick::_saveSettings()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);

    settings.beginGroup(_name);
    settings.setValue(_calibratedSettingsKey, _calibrated);
    settings.setValue(_throttleSmoothingSettingsKey, _throttleSmoothing);
    settings.setValue(_useDeadbandSettingsKey, _useDeadband);
    settings.setValue(_axisFrequencySettingsKey, axisFrequencyHz());
    settings.setValue(_buttonFrequencySettingsKey, buttonFrequencyHz());
    settings.setValue(_throttleModeCenterZeroSettingsKey, _throttleModeCenterZero);
    settings.setValue(_negativeThrustSettingsKey, _negativeThrust);
    settings.setValue(_circleCorrectionSettingsKey, _circleCorrection);
    settings.setValue(_manualControlExtensionsEnabledKey, _enableManualControlExtensions);
    settings.setValue(_exponentialPctSettingsKey, _exponentialPctFact.rawValue().toDouble());
    settings.setValue(_transmitterModeSettingsKey, _transmitterMode);

    qCDebug(JoystickLog) << _name;
    qCDebug(JoystickLog) << "  calibrated:throttleSmoothing:circlecorrection:negativeThrust:throttleCenterZero:deadband:axisfrequency:buttonfrequency:txmode";
    qCDebug(JoystickLog) << " " << _calibrated << _throttleSmoothing
                            << _circleCorrection << _negativeThrust << _throttleModeCenterZero
                            << _useDeadband << axisFrequencyHz() << buttonFrequencyHz()
                            << _transmitterMode;

    const QString minTpl("Axis%1Min");
    const QString maxTpl("Axis%1Max");
    const QString trimTpl("Axis%1Trim");
    const QString revTpl("Axis%1Rev");
    const QString deadbndTpl("Axis%1Deadbnd");

    qCDebug(JoystickLog) << "  axis:min:max:trim:reversed:deadband";
    for (int axis = 0; axis < _axisCount; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];
        settings.setValue(trimTpl.arg(axis), calibration->center);
        settings.setValue(minTpl.arg(axis), calibration->min);
        settings.setValue(maxTpl.arg(axis), calibration->max);
        settings.setValue(revTpl.arg(axis), calibration->reversed);
        settings.setValue(deadbndTpl.arg(axis), calibration->deadband);
        qCDebug(JoystickLog) << " " << axis
                                << calibration->min
                                << calibration->max
                                << calibration->center
                                << calibration->reversed
                                << calibration->deadband;
    }

    // Write mode 2 mappings without changing mapping currently in use
    int temp[maxAxisFunction];
    _remapAxes(_transmitterMode, 2, temp);

    qCDebug(JoystickLog) << "TX Mode mappings";
    for (int function = 0; function < maxAxisFunction; function++) {
        settings.setValue(_rgFunctionSettingsKey[function], temp[function]);
        qCDebug(JoystickLog) << " " << axisFunctionToString(static_cast<AxisFunction_t>(function)) << _rgFunctionSettingsKey[function];
    }

    _saveButtonSettings();
}

int Joystick::_mapFunctionMode(int mode, int function)
{
    static constexpr const int mapping[][6] = {
        { yawFunction, pitchFunction, rollFunction, throttleFunction, gimbalPitchFunction, gimbalYawFunction },
        { yawFunction, throttleFunction, rollFunction, pitchFunction, gimbalPitchFunction, gimbalYawFunction },
        { rollFunction, pitchFunction, yawFunction, throttleFunction, gimbalPitchFunction, gimbalYawFunction },
        { rollFunction, throttleFunction, yawFunction, pitchFunction, gimbalPitchFunction, gimbalYawFunction }
    };

    if ((mode > 0) && (mode <= 4)) {
        return mapping[mode - 1][function];
    }

    return -1;
}

void Joystick::_remapAxes(int currentMode, int newMode, int (&newMapping)[maxAxisFunction])
{
    int temp[maxAxisFunction];

    for (int function = 0; function < maxAxisFunction; function++) {
        temp[_mapFunctionMode(newMode, function)] = _rgFunctionAxis[_mapFunctionMode(currentMode, function)];
    }

    for (int function = 0; function < maxAxisFunction; function++) {
        newMapping[function] = temp[function];
    }
}

void Joystick::run()
{
    _open();

    _axisTime.start();

    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        if (_buttonActionArray[buttonIndex]) {
            _buttonActionArray[buttonIndex]->buttonTime.start();
        }
    }

    while (!_exitThread) {
        _update();
        _handleButtons();
        if (axisCount() != 0) {
            _handleAxis();
        }

        const int sleep = qMin(static_cast<int>(1000.0 / _maxAxisFrequencyHz), static_cast<int>(1000.0 / _maxButtonFrequencyHz)) / 2;
        QThread::msleep(sleep);
    }

    _close();
}

void Joystick::_handleButtons()
{
    if (_currentPollingType == NotPolling) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling!";
        return;
    }


    // Save preivous button states
    int lastButtonValues[256]{};
    for (int buttonIndex = 0; buttonIndex < _buttonCount; buttonIndex++) {
        const bool newButtonValue = _getButton(buttonIndex);
        if (buttonIndex < 256) {
            lastButtonValues[buttonIndex] = _rgButtonValues[buttonIndex];
        }
    }

    if (_currentPollingType == PollingForConfiguration) {
        //-- Update button states
        for (int buttonIndex = 0; buttonIndex < _buttonCount; buttonIndex++) {
            const bool newButtonValue = _getButton(buttonIndex);
            if (newButtonValue && (_rgButtonValues[buttonIndex] == BUTTON_UP)) {
                _rgButtonValues[buttonIndex] = BUTTON_DOWN;
                emit rawButtonPressedChanged(buttonIndex, newButtonValue);
            } else if (!newButtonValue && (_rgButtonValues[buttonIndex] != BUTTON_UP)) {
                _rgButtonValues[buttonIndex] = BUTTON_UP;
                emit rawButtonPressedChanged(buttonIndex, newButtonValue);
            }
        }

        //-- Update hat - append hat buttons to the end of the normal button list
        int numHatButtons = 4;
        for (int hatIndex = 0; hatIndex < _hatCount; hatIndex++) {
            for (int hatButtonIndex = 0; hatButtonIndex < numHatButtons; hatButtonIndex++) {
                // Create new index value that includes the normal button list
                const int rgButtonValueIndex = (hatIndex * numHatButtons) + hatButtonIndex + _buttonCount;
                // Get hat value from joystick
                const bool newButtonValue = _getHat(hatIndex, hatButtonIndex);
                if(rgButtonValueIndex < 256) {
                    lastButtonValues[rgButtonValueIndex] = _rgButtonValues[rgButtonValueIndex];
                }

                if (newButtonValue && (_rgButtonValues[rgButtonValueIndex] == BUTTON_UP)) {
                    _rgButtonValues[rgButtonValueIndex] = BUTTON_DOWN;
                    emit rawButtonPressedChanged(rgButtonValueIndex, newButtonValue);
                } else if (!newButtonValue && (_rgButtonValues[rgButtonValueIndex] != BUTTON_UP)) {
                    _rgButtonValues[rgButtonValueIndex] = BUTTON_UP;
                    emit rawButtonPressedChanged(rgButtonValueIndex, newButtonValue);
                }
            }
        }
    } else if (_currentPollingType == PollingForVehicle) {
        Vehicle *const vehicle = _pollingVehicle;
        if (!vehicle) {
            qCWarning(JoystickLog) << "Internal Error: No vehicle for joystick!";
            return;
        }
        if (!JoystickManager::instance()->joystickEnabledForVehicle(vehicle)) {
            qCWarning(JoystickLog) << "Internal Error: Joystick not enabled for vehicle!";
            return;
        }
        if (!_calibrated) {
            return;
        }

        //-- Process button press/release
        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            if ((_rgButtonValues[buttonIndex] == BUTTON_DOWN) || (_rgButtonValues[buttonIndex] == BUTTON_REPEAT)) {
                if (_buttonActionArray[buttonIndex]) {
                    const QString buttonAction = _buttonActionArray[buttonIndex]->action;
                    if (buttonAction.isEmpty() || (buttonAction == _buttonActionNone)) {
                        continue;
                    }

                    if (!_buttonActionArray[buttonIndex]->repeat) {
                        if (_rgButtonValues[buttonIndex] == BUTTON_DOWN) {
                            // Check for a multi-button action
                            QList<int> rgButtons = { buttonIndex };
                            bool executeButtonAction = true;
                            for (int multiIndex = 0; multiIndex < _totalButtonCount; multiIndex++) {
                                if (multiIndex == buttonIndex) {
                                    continue;
                                }

                                if (_buttonActionArray[multiIndex] && (_buttonActionArray[multiIndex]->action == buttonAction)) {
                                    // We found a multi-button action
                                    if ((_rgButtonValues[multiIndex] == BUTTON_DOWN) || (_rgButtonValues[multiIndex] == BUTTON_REPEAT)) {
                                        // So far so good
                                        rgButtons.append(multiIndex);
                                        continue;
                                    } else {
                                        // We are missing a press we need
                                        executeButtonAction = false;
                                        break;
                                    }
                                }
                            }

                            if (executeButtonAction) {
                                qCDebug(JoystickLog) << "Action triggered" << rgButtons << buttonAction;
                                _executeButtonAction(buttonAction, true);
                            }
                        }
                    } else {
                        //-- Process repeat buttons
                        const int buttonDelay = static_cast<int>(1000.0 / buttonFrequencyHz());
                        if (_buttonActionArray[buttonIndex]->buttonTime.elapsed() > buttonDelay) {
                            _buttonActionArray[buttonIndex]->buttonTime.start();
                            qCDebug(JoystickLog) << "Repeat button triggered" << buttonIndex << buttonAction;
                            _executeButtonAction(buttonAction, true);
                        }
                    }
                }
                //-- Flag it as processed
                _rgButtonValues[buttonIndex] = BUTTON_REPEAT;
            } else if (_rgButtonValues[buttonIndex] == BUTTON_UP) {
                //-- Button up transition
                if (buttonIndex < 256) {
                    if ((lastButtonValues[buttonIndex] == BUTTON_DOWN) || (lastButtonValues[buttonIndex] == BUTTON_REPEAT)) {
                        if (_buttonActionArray[buttonIndex]) {
                            const QString buttonAction = _buttonActionArray[buttonIndex]->action;
                            if (buttonAction.isEmpty() || buttonAction == _buttonActionNone) {
                                continue;
                            }
                            qCDebug(JoystickLog) << "Button up" << buttonIndex << buttonAction;
                            _executeButtonAction(buttonAction, false);
                        }
                    }
                }
            }
        }
    }
}

float Joystick::_adjustRange(int value, const Calibration_t &calibration, bool withDeadbands)
{
    float valueNormalized;
    float axisLength;
    float axisBasis;

    if (value > calibration.center) {
        axisBasis = 1.0f;
        valueNormalized = value - calibration.center;
        axisLength =  calibration.max - calibration.center;
    } else {
        axisBasis = -1.0f;
        valueNormalized = calibration.center - value;
        axisLength =  calibration.center - calibration.min;
    }

    float axisPercent;
    if (withDeadbands) {
        if (valueNormalized > calibration.deadband) {
            axisPercent = (valueNormalized - calibration.deadband) / (axisLength - calibration.deadband);
        } else if (valueNormalized<-calibration.deadband) {
            axisPercent = (valueNormalized + calibration.deadband) / (axisLength - calibration.deadband);
        } else {
            axisPercent = 0.f;
        }
    } else {
        axisPercent = valueNormalized / axisLength;
    }

    float correctedValue = axisBasis * axisPercent;
    if (calibration.reversed) {
        correctedValue *= -1.0f;
    }

    return std::max(-1.0f, std::min(correctedValue, 1.0f));
}

void Joystick::_handleAxis()
{
    const int axisDelay = static_cast<int>(1000.0 / axisFrequencyHz());
    if (_axisTime.elapsed() <= axisDelay) {
        return;
    }

    _axisTime.start();

    if (_currentPollingType == NotPolling) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling!";
        return;
    }

    if (_currentPollingType == PollingForConfiguration) {
        // Signal the axis values to Joystick Config/Cal. Axes in Joystick terminology are the same as Channels in
        // RemoteControlCalibrationController terminology.
        QVector<int> channelValues(_axisCount);
        for (int axisIndex = 0; axisIndex < _axisCount; axisIndex++) {
            channelValues[axisIndex] = _getAxisValue(axisIndex);
        }
        emit rawChannelValuesChanged(channelValues);
    } else if (_currentPollingType == PollingForVehicle) {
        Vehicle *const vehicle = _pollingVehicle;
        if (!vehicle) {
            qCWarning(JoystickLog) << "Internal Error: No vehicle for joystick!";
            return;
        }
        if (!JoystickManager::instance()->joystickEnabledForVehicle(vehicle)) {
            qCWarning(JoystickLog) << "Internal Error: Joystick not enabled for vehicle!";
            return;
        }
        if (!_calibrated) {
            return;
        }

        int axisIndex = _rgFunctionAxis[rollFunction];
        float roll = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], _useDeadband);
        axisIndex = _rgFunctionAxis[pitchFunction];
        float pitch = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], _useDeadband);
        axisIndex = _rgFunctionAxis[yawFunction];
        float yaw = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex],_useDeadband);
        axisIndex = _rgFunctionAxis[throttleFunction];
        float throttle = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], _throttleModeCenterZero ? _useDeadband : false);

        float gimbalPitch = NAN;
        if (_enableManualControlExtensions && _axisCount > 4) {
            axisIndex = _rgFunctionAxis[gimbalPitchFunction];
            gimbalPitch = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex],_useDeadband);
        }

        float gimbalYaw = NAN;
        if (_enableManualControlExtensions && _axisCount > 5) {
            axisIndex = _rgFunctionAxis[gimbalYawFunction];
            gimbalYaw = _adjustRange(_getAxisValue(axisIndex),   _rgCalibration[axisIndex],_useDeadband);
        }

        if (_throttleSmoothing) {
            static float throttleAccumulator = 0.f;
            throttleAccumulator += (throttle * (40 / 1000.f)); // for throttle to change from min to max it will take 1000ms (40ms is a loop time)
            throttleAccumulator = std::max(static_cast<float>(-1.f), std::min(throttleAccumulator, static_cast<float>(1.f)));
            throttle = throttleAccumulator;
        }

        if (_circleCorrection) {
            const float roll_limited = std::max(static_cast<float>(-M_PI_4), std::min(roll, static_cast<float>(M_PI_4)));
            const float pitch_limited = std::max(static_cast<float>(-M_PI_4), std::min(pitch, static_cast<float>(M_PI_4)));
            const float yaw_limited = std::max(static_cast<float>(-M_PI_4), std::min(yaw, static_cast<float>(M_PI_4)));
            const float throttle_limited = std::max(static_cast<float>(-M_PI_4), std::min(throttle, static_cast<float>(M_PI_4)));

            // Map from unit circle to linear range and limit
            roll = std::max(-1.0f, std::min(tanf(asinf(roll_limited)), 1.0f));
            pitch = std::max(-1.0f, std::min(tanf(asinf(pitch_limited)), 1.0f));
            yaw = std::max(-1.0f, std::min(tanf(asinf(yaw_limited)), 1.0f));
            throttle = std::max(-1.0f, std::min(tanf(asinf(throttle_limited)), 1.0f));
        }

        double exponentialPercent = _exponentialPctFact.rawValue().toDouble();
        if ( exponentialPercent > 0.0 ) {
            const float exponential = -static_cast<float>(exponentialPercent / 100.0);
            roll =  -exponential * powf(roll, 3) + ((1 + exponential) * roll);
            pitch = -exponential * powf(pitch,3) + ((1 + exponential) * pitch);
            yaw =   -exponential * powf(yaw,  3) + ((1 + exponential) * yaw);
        }

        // Adjust throttle to 0:1 range
        if (_throttleModeCenterZero && vehicle->supportsThrottleModeCenterZero()) {
            if (!vehicle->supportsNegativeThrust() || !_negativeThrust) {
                throttle = std::max(0.0f, throttle);
            }
        } else {
            throttle = (throttle + 1.0f) / 2.0f;
        }

        qCDebug(JoystickVerboseLog) << "name:roll:pitch:yaw:throttle:gimbalPitch:gimbalYaw" << name() << roll << -pitch << yaw << throttle << gimbalPitch << gimbalYaw;

        // NOTE: The buttonPressedBits going to MANUAL_CONTROL are currently used by ArduSub (and it only handles 16 bits)
        // Set up button bitmap
        quint64 buttonPressedBits = 0;  ///< Buttons pressed for manualControl signal
        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            const quint64 buttonBit = static_cast<quint64>(1LL << buttonIndex);
            if (_rgButtonValues[buttonIndex] != BUTTON_UP) {
                buttonPressedBits |= buttonBit;
            }
        }

        emit axisValues(roll, pitch, yaw, throttle);

        const uint16_t lowButtons = static_cast<uint16_t>(buttonPressedBits & 0xFFFF);
        const uint16_t highButtons = static_cast<uint16_t>((buttonPressedBits >> 16) & 0xFFFF);


        vehicle->sendJoystickDataThreadSafe(roll, pitch, yaw, throttle, lowButtons, highButtons, gimbalPitch, gimbalYaw);
    }
}

void Joystick::_startPollingForVehicle(Vehicle &vehicle)
{
    if (_currentPollingType == PollingForVehicle) {
        qCWarning(JoystickLog) << "Internal Error: Joystick already polling for vehicle!";
        return;
    }
    if (_previousPollingType != NotPolling) {
        qCWarning(JoystickLog) << "Internal Error: Joystick previous polling type not None:" << _pollingTypeToString(_previousPollingType);
        return;
    }
    if (_currentPollingType == NotPolling && isRunning()) {
        qCWarning(JoystickLog) << "Internal Error: Joystick polling should not be running!";
        return;
    }

    _currentPollingType = PollingForVehicle;
    _pollingVehicle = &vehicle;

    _buildActionList(_pollingVehicle);

    (void) connect(this, &Joystick::setArmed,           _pollingVehicle, &Vehicle::setArmedShowError);
    (void) connect(this, &Joystick::setVtolInFwdFlight, _pollingVehicle, &Vehicle::setVtolInFwdFlight);
    (void) connect(this, &Joystick::setFlightMode,      _pollingVehicle, &Vehicle::setFlightMode);
    (void) connect(this, &Joystick::emergencyStop,      _pollingVehicle, &Vehicle::emergencyStop);
    (void) connect(this, &Joystick::gripperAction,      _pollingVehicle, &Vehicle::sendGripperAction);
    (void) connect(this, &Joystick::landingGearDeploy,  _pollingVehicle, &Vehicle::landingGearDeploy);
    (void) connect(this, &Joystick::landingGearRetract, _pollingVehicle, &Vehicle::landingGearRetract);
    (void) connect(this, &Joystick::motorInterlock,     _pollingVehicle, &Vehicle::motorInterlock);

    (void) connect(_pollingVehicle, &Vehicle::flightModesChanged, this, &Joystick::_flightModesChanged);

    if (GimbalController *const gimbal = _pollingVehicle->gimbalController()) {
        (void) connect(this, &Joystick::gimbalYawLock,      gimbal, &GimbalController::gimbalYawLock);
        (void) connect(this, &Joystick::centerGimbal,       gimbal, &GimbalController::centerGimbal);
        (void) connect(this, &Joystick::gimbalPitchStart,   gimbal, &GimbalController::gimbalPitchStart);
        (void) connect(this, &Joystick::gimbalYawStart,     gimbal, &GimbalController::gimbalYawStart);
        (void) connect(this, &Joystick::gimbalPitchStop,    gimbal, &GimbalController::gimbalPitchStop);
        (void) connect(this, &Joystick::gimbalYawStop,      gimbal, &GimbalController::gimbalYawStop);
    }

    qDebug(JoystickLog) << "Started joystick polling for vehicle" << _pollingVehicle->id();

    start();
}

void Joystick::_startPollingForConfiguration()
{
    if (_currentPollingType == PollingForConfiguration) {
        qCWarning(JoystickLog) << "Internal Error: Joystick already polling for configuration!";
        return;
    }
    if (_previousPollingType != NotPolling) {
        qCWarning(JoystickLog) << "Internal Error: Joystick previous polling type not None:" << _pollingTypeToString(_previousPollingType);
        return;
    }

    qCDebug(JoystickLog) << "Started joystick polling for configuration. Saved previous polling type:" << _pollingTypeToString(_currentPollingType);

    _previousPollingType = _currentPollingType;;
    _currentPollingType = PollingForConfiguration;

    if (!isRunning()) {
        start();
    }
}

void Joystick::_stopPollingForConfiguration()
{
    if (_currentPollingType != PollingForConfiguration) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling for configuration!";
        return;
    }
    if (!isRunning()) {
        qCWarning(JoystickLog) << "Internal Error: Joystick polling not running!";
        return;
    }

    qCDebug(JoystickLog) << "Stopped joystick polling for configuration. Restored previous polling type:" << _pollingTypeToString(_previousPollingType);

    _currentPollingType = _previousPollingType;
    _previousPollingType = NotPolling;

    if (_currentPollingType == NotPolling) {
        _exitThread = true;
    }
}

void Joystick::_stopAllPolling()
{
    if (_pollingVehicle) {
        (void) disconnect(this, nullptr, _pollingVehicle, nullptr);
        (void) disconnect(_pollingVehicle, &Vehicle::flightModesChanged, this, &Joystick::_flightModesChanged);
        if (GimbalController *const gimbal = _pollingVehicle->gimbalController()) {
            (void) disconnect(this, nullptr, gimbal, nullptr);
        }
        _pollingVehicle = nullptr;
    }

    qCDebug(JoystickLog) << "Stopped all joystick polling";

    _currentPollingType = NotPolling;
    _previousPollingType = NotPolling;

    if (isRunning()) {
        _exitThread = true;
    }
}

QString Joystick::_pollingTypeToString(PollingType pollingType) const
{
    switch (pollingType) {
    case NotPolling:
        return QStringLiteral("NotPolling");
    case PollingForConfiguration:
        return QStringLiteral("PollingForConfiguration");
    case PollingForVehicle:
        return QStringLiteral("PollingForVehicle");
    default:
        break;
    }

    return QStringLiteral("UnknownPollingType");
}

void Joystick::setCalibration(int axis, const Calibration_t &calibration)
{
    if (!_validAxis(axis)) {
        return;
    }

    _calibrated = true;
    _rgCalibration[axis] = calibration;
    _saveSettings();
    emit calibratedChanged(_calibrated);
}

Joystick::Calibration_t Joystick::getCalibration(int axis) const
{
    if (!_validAxis(axis)) {
        return Calibration_t{};
    }

    return _rgCalibration[axis];
}

void Joystick::setFunctionAxis(AxisFunction_t function, int axis)
{
    if (!_validAxis(axis)) {
        return;
    }

    _calibrated = true;
    _rgFunctionAxis[function] = axis;
    _saveSettings();
    emit calibratedChanged(_calibrated);
}

int Joystick::getFunctionAxis(AxisFunction_t function) const
{
    if ((static_cast<int>(function) < 0) || (function >= maxAxisFunction)) {
        qCWarning(JoystickLog) << "Invalid function" << function;
    }

    return _rgFunctionAxis[function];
}

RemoteControlCalibrationController::StickFunction Joystick::mapAxisFunctionToRCCStickFunction(Joystick::AxisFunction_t axisFunction) const
{
    switch (axisFunction) {
        case rollFunction:
            return RemoteControlCalibrationController::stickFunctionRoll;
        case pitchFunction:
            return RemoteControlCalibrationController::stickFunctionPitch;
        case yawFunction:
            return RemoteControlCalibrationController::stickFunctionYaw;
        case throttleFunction:
            return RemoteControlCalibrationController::stickFunctionThrottle;
#if 0
        // NYI
        case gimbalPitchFunction:
            return RemoteControlCalibrationController::stickFunctionGimbalPitch;
        case gimbalYawFunction:
            return RemoteControlCalibrationController::stickFunctionGimbalYaw;
#endif
        default:
            qCWarning(JoystickLog) << "Invalid AxisFunction_t" << axisFunction;
            return RemoteControlCalibrationController::stickFunctionRoll;
    }
}

Joystick::AxisFunction_t Joystick::mapRCCStickFunctionToAxisFunction(RemoteControlCalibrationController::StickFunction stickFunction) const
{
    switch (stickFunction) {
        case RemoteControlCalibrationController::stickFunctionRoll:
            return rollFunction;
        case RemoteControlCalibrationController::stickFunctionPitch:
            return pitchFunction;
        case RemoteControlCalibrationController::stickFunctionYaw:
            return yawFunction;
        case RemoteControlCalibrationController::stickFunctionThrottle:
            return throttleFunction;
        default:
            qCWarning(JoystickLog) << "Invalid StickFunction" << stickFunction;
            return rollFunction;
    }
}

void Joystick::setFunctionForChannel(RemoteControlCalibrationController::StickFunction stickFunction, int channel)
{
    return setFunctionAxis(mapRCCStickFunctionToAxisFunction(stickFunction), channel);
}

int Joystick::getChannelForFunction(RemoteControlCalibrationController::StickFunction stickFunction) const
{
    return getFunctionAxis(mapRCCStickFunctionToAxisFunction(stickFunction));
}

void Joystick::setButtonRepeat(int button, bool repeat)
{
    if (!_validButton(button) || !_buttonActionArray[button]) {
        return;
    }

    _buttonActionArray[button]->repeat = repeat;
    _buttonActionArray[button]->buttonTime.start();

    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    settings.setValue(QString(_buttonActionRepeatSettingsKey).arg(button), _buttonActionArray[button]->repeat);
}

bool Joystick::getButtonRepeat(int button)
{
    if (!_validButton(button) || !_buttonActionArray[button]) {
        return false;
    }

    return _buttonActionArray[button]->repeat;
}

void Joystick::setButtonAction(int button, const QString& action)
{
    if (!_validButton(button)) {
        return;
    }

    qCDebug(JoystickLog) << "button:action" << button << action;

    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    if (action.isEmpty() || action == _buttonActionNone) {
        if (_buttonActionArray[button]) {
            delete _buttonActionArray[button];
            _buttonActionArray[button] = nullptr;
            settings.remove(QString(_buttonActionNameSettingsKey).arg(button));
            settings.remove(QString(_buttonActionRepeatSettingsKey).arg(button));
        }
    } else {
        if (!_buttonActionArray[button]) {
            _buttonActionArray[button] = new AssignedButtonAction(action);
        } else {
            _buttonActionArray[button]->action = action;
            //-- Make sure repeat is off if this action doesn't support repeats
            const int idx = _findAssignableButtonAction(action);
            if (idx >= 0) {
                const AssignableButtonAction *const buttonAction = qobject_cast<const AssignableButtonAction*>(_assignableButtonActions->get(idx));
                if (!buttonAction->canRepeat()) {
                    _buttonActionArray[button]->repeat = false;
                }
            }
        }

        settings.setValue(QString(_buttonActionNameSettingsKey).arg(button), _buttonActionArray[button]->action);
        settings.setValue(QString(_buttonActionRepeatSettingsKey).arg(button), _buttonActionArray[button]->repeat);
    }

    emit buttonActionsChanged();
}

QString Joystick::getButtonAction(int button) const
{
    if (_validButton(button)) {
        if (_buttonActionArray[button]) {
            return _buttonActionArray[button]->action;
        }
    }

    return QString(_buttonActionNone);
}

QStringList Joystick::buttonActions() const
{
    QStringList list;
    for (int button = 0; button < _totalButtonCount; button++) {
        list << getButtonAction(button);
    }

    return list;
}

void Joystick::setThrottleModeCenterZero(bool throttleCenterZero)
{
    if (_throttleModeCenterZero == throttleCenterZero) {
        return;
    }

    _throttleModeCenterZero = throttleCenterZero;

    if (!_throttleModeCenterZero) {
        setThrottleSmoothing(false);
    }

    _saveSettings();
    emit throttleModeCenterZeroChanged(_throttleModeCenterZero);
}

void Joystick::setNegativeThrust(bool allowNegative)
{
    if (_negativeThrust != allowNegative) {
        _negativeThrust = allowNegative;
        _saveSettings();
        emit negativeThrustChanged(_negativeThrust);
    }
}

void Joystick::setThrottleSmoothing(bool enabled)
{
    if (enabled != _throttleSmoothing) {
        _throttleSmoothing = enabled;
        _saveSettings();
        emit throttleSmoothingChanged(_throttleSmoothing);
    }
}

void Joystick::setUseDeadband(bool deadband)
{
    if (deadband != _useDeadband) {
        _useDeadband = deadband;
        _saveSettings();
        emit useDeadbandChanged(_useDeadband);
    }
}

void Joystick::setCircleCorrection(bool circleCorrection)
{
    if (circleCorrection != _circleCorrection) {
        _circleCorrection = circleCorrection;
        _saveSettings();
        emit circleCorrectionChanged(_circleCorrection);
    }
}

void Joystick::setAxisFrequency(double val)
{
    double result = val;
    result = qMax(_minAxisFrequencyHz, result);
    result = qMin(_maxAxisFrequencyHz, result);

    if (result != axisFrequencyHz()) {
        _axisFrequencyHzFact.setRawValue(result);
        _saveSettings();
        emit axisFrequencyHzChanged();
    }
}

void Joystick::setButtonFrequency(double val)
{
    double result = val;
    result = qMax(_minButtonFrequencyHz, result);
    result = qMin(_maxButtonFrequencyHz, result);

    if (result != buttonFrequencyHz()) {
        _buttonFrequencyHzFact.setRawValue(result);
        _saveSettings();
        emit buttonFrequencyHzChanged();
    }
}

void Joystick::_executeButtonAction(const QString &action, bool buttonDown)
{
    Vehicle *const vehicle = _pollingVehicle;
    if (!vehicle) {
        qCWarning(JoystickLog) << "Internal Error: No vehicle for joystick!";
        return;
    }
    if (!JoystickManager::instance()->joystickEnabledForVehicle(vehicle)) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not enabled for vehicle!";
        return;
    }
    if (action == _buttonActionNone) {
        return;
    }

    if (action == _buttonActionArm) {
        if (buttonDown) {
            emit setArmed(true);
        }
    } else if (action == _buttonActionDisarm) {
        if (buttonDown) {
            emit setArmed(false);
        }
    } else if (action == _buttonActionToggleArm) {
        if (buttonDown) {
            emit setArmed(!vehicle->armed());
        }
    } else if (action == _buttonActionVTOLFixedWing) {
        if (buttonDown) {
            emit setVtolInFwdFlight(true);
        }
    } else if (action == _buttonActionVTOLMultiRotor) {
        if (buttonDown) {
            emit setVtolInFwdFlight(false);
        }
    } else if (vehicle->flightModes().contains(action)) {
        if (buttonDown) {
            emit setFlightMode(action);
        }
    } else if ((action == _buttonActionContinuousZoomIn) || (action == _buttonActionContinuousZoomOut)) {
        if (buttonDown) {
            emit startContinuousZoom((action == _buttonActionContinuousZoomIn) ? 1 : -1);
        } else {
            emit stopContinuousZoom();
        }
    } else if ((action == _buttonActionStepZoomIn) || (action == _buttonActionStepZoomOut)) {
        if (buttonDown) {
            emit stepZoom((action == _buttonActionStepZoomIn) ? 1 : -1);
        }
    } else if ((action == _buttonActionNextStream) || (action == _buttonActionPreviousStream)) {
        if (buttonDown) {
            emit stepStream((action == _buttonActionNextStream) ? 1 : -1);
        }
    } else if ((action == _buttonActionNextCamera) || (action == _buttonActionPreviousCamera)) {
        if (buttonDown) {
            emit stepCamera((action == _buttonActionNextCamera) ? 1 : -1);
        }
    } else if (action == _buttonActionTriggerCamera) {
        if (buttonDown) {
            emit triggerCamera();
        }
    } else if (action == _buttonActionStartVideoRecord) {
        if (buttonDown) {
            emit startVideoRecord();
        }
    } else if (action == _buttonActionStopVideoRecord) {
        if (buttonDown) {
            emit stopVideoRecord();
        }
    } else if (action == _buttonActionToggleVideoRecord) {
        if (buttonDown) {
            emit toggleVideoRecord();
        }
    } else if (action == _buttonActionGimbalUp) {
        if (buttonDown) {
            emit gimbalPitchStart(1);
        } else {
            emit gimbalPitchStop();
        }
    } else if (action == _buttonActionGimbalDown) {
        if (buttonDown) {
            emit gimbalPitchStart(-1);
        } else {
            emit gimbalPitchStop();
        }
    } else if (action == _buttonActionGimbalLeft) {
        if (buttonDown) {
            emit gimbalYawStart(-1);
        } else {
            emit gimbalYawStop();
        }
    } else if (action == _buttonActionGimbalRight) {
        if (buttonDown) {
            emit gimbalYawStart(1);
        } else {
            emit gimbalYawStop();
        }
    } else if (action == _buttonActionGimbalCenter) {
        if (buttonDown) {
            emit centerGimbal();
        }
    } else if (action == _buttonActionGimbalYawLock) {
        if (buttonDown) {
            emit gimbalYawLock(true);
        }
    } else if (action == _buttonActionGimbalYawFollow) {
        if (buttonDown) {
            emit gimbalYawLock(false);
        }
    } else if (action == _buttonActionEmergencyStop) {
        if (buttonDown) {
            emit emergencyStop();
        }
    } else if (action == _buttonActionGripperClose) {
        if (buttonDown) {
            emit gripperAction(QGCMAVLink::GripperActionClose);
        }
    } else if (action == _buttonActionGripperOpen) {
        if (buttonDown) {
            emit gripperAction(QGCMAVLink::GripperActionOpen);
        }
    } else if (action == _buttonActionGripperStop) {
        if (buttonDown) {
            emit gripperAction(QGCMAVLink::GripperActionStop);
        }
    } else if (action == _buttonActionLandingGearDeploy) {
        if (buttonDown) {
            emit landingGearDeploy();
        }
    } else if (action == _buttonActionLandingGearRetract) {
        if (buttonDown) {
            emit landingGearRetract();
        }
    } else if (action == _buttonActionMotorInterlockEnable) {
        if (buttonDown) {
            emit motorInterlock(true);
        }
    } else if (action == _buttonActionMotorInterlockDisable) {
        if (buttonDown) {
            emit motorInterlock(false);
        }
    } else {
        if (buttonDown && vehicle) {
            emit unknownAction(action);
            for (int i = 0; i<_mavlinkActionManager->actions()->count(); i++) {
                MavlinkAction *const mavlinkAction = _mavlinkActionManager->actions()->value<MavlinkAction*>(i);
                if (action == mavlinkAction->label()) {
                    mavlinkAction->sendTo(vehicle);
                    return;
                }
            }
        }
    }
}

bool Joystick::_validAxis(int axis) const
{
    if ((axis >= 0) && (axis < _axisCount)) {
        return true;
    }

    qCWarning(JoystickLog) << "Invalid axis index" << axis;
    return false;
}

bool Joystick::_validButton(int button) const
{
    if ((button >= 0) && (button < _totalButtonCount)) {
        return true;
    }

    qCWarning(JoystickLog) << "Invalid button index" << button;
    return false;
}

int Joystick::_findAssignableButtonAction(const QString &action)
{
    for (int i = 0; i < _assignableButtonActions->count(); i++) {
        const AssignableButtonAction *const buttonAction = qobject_cast<const AssignableButtonAction*>(_assignableButtonActions->get(i));
        if (buttonAction->action() == action) {
            return i;
        }
    }

    return -1;
}

void Joystick::_buildActionList(Vehicle *vehicle)
{
    if (_assignableButtonActions->count()) {
        _assignableButtonActions->clearAndDeleteContents();
    }
    _availableActionTitles.clear();

    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionNone));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionArm));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionDisarm));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionToggleArm));
    if (vehicle) {
        const QStringList list = vehicle->flightModes();
        for (const QString &mode : list) {
            _assignableButtonActions->append(new AssignableButtonAction(mode));
        }
    }

    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionVTOLFixedWing));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionVTOLMultiRotor));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionContinuousZoomIn, true));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionContinuousZoomOut, true));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionStepZoomIn, true));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionStepZoomOut, true));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionNextStream));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionPreviousStream));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionNextCamera));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionPreviousCamera));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionTriggerCamera));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionStartVideoRecord));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionStopVideoRecord));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionToggleVideoRecord));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGimbalDown));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGimbalUp));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGimbalLeft));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGimbalRight));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGimbalCenter));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGimbalYawLock));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGimbalYawFollow));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionEmergencyStop));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGripperClose));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGripperOpen));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGripperStop));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionLandingGearDeploy));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionLandingGearRetract));
#ifndef QGC_NO_ARDUPILOT_DIALECT
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionMotorInterlockEnable));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionMotorInterlockDisable));
#endif

    const auto customActions = QGCCorePlugin::instance()->joystickActions();
    for (const auto &action : customActions) {
        _assignableButtonActions->append(new AssignableButtonAction(action.name, action.canRepeat));
    }

    for (int i = 0; i < _mavlinkActionManager->actions()->count(); i++) {
        const MavlinkAction *const mavlinkAction = _mavlinkActionManager->actions()->value<const MavlinkAction*>(i);
        _assignableButtonActions->append(new AssignableButtonAction(mavlinkAction->label()));
    }

    for (int i = 0; i < _assignableButtonActions->count(); i++) {
        const AssignableButtonAction *const buttonAction = qobject_cast<const AssignableButtonAction*>(_assignableButtonActions->get(i));
        _availableActionTitles << buttonAction->action();
    }

    emit assignableActionsChanged();
}

QString Joystick::axisFunctionToString(AxisFunction_t function)
{
    switch (function) {
    case rollFunction:
        return QStringLiteral("Roll");
    case pitchFunction:
        return QStringLiteral("Pitch");
    case yawFunction:
        return QStringLiteral("Yaw");
    case throttleFunction:
        return QStringLiteral("Throttle");
    case gimbalPitchFunction:
        return QStringLiteral("Gimbal Pitch");
    case gimbalYawFunction:
        return QStringLiteral("Gimbal Yaw");
    default:
        return QStringLiteral("Unknown");
    }
}

void Joystick::setEnableManualControlExtensions(bool enable)
{
    if (_enableManualControlExtensions != enable) {
        _enableManualControlExtensions = enable;
        _saveSettings();
        emit enableManualControlExtensionsChanged();
    }
}

void Joystick::startConfiguration()
{
    _startPollingForConfiguration();
}

void Joystick::stopConfiguration()
{
    _stopPollingForConfiguration();
}

void Joystick::_saveExponentialPctSetting()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    settings.setValue(_exponentialPctSettingsKey, _exponentialPctFact.rawValue());
}

void Joystick::setTransmitterMode(int mode)
{
    if (_transmitterMode != mode) {
        _transmitterMode = mode;
        _saveSettings();
        emit transmitterModeChanged(_transmitterMode);
    }
}
