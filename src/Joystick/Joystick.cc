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
#include "MultiVehicleManager.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#include <QtCore/QSettings>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(JoystickLog, "qgc.joystick.joystick")
QGC_LOGGING_CATEGORY(JoystickValuesLog, "qgc.joystick.joystickvalues")

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

int Joystick::_transmitterMode = 2;

Joystick::Joystick(const QString &name, int axisCount, int buttonCount, int hatCount, QObject *parent)
    : QThread(parent)
    , _name(name)
    , _axisCount(axisCount)
    , _buttonCount(buttonCount)
    , _hatCount(hatCount)
    , _hatButtonCount(4 * hatCount)
    , _totalButtonCount(_buttonCount + _hatButtonCount)
    , _rgAxisValues(new int[static_cast<size_t>(_axisCount)])
    , _rgCalibration(new Calibration_t[static_cast<size_t>(_axisCount)])
    , _rgButtonValues(new uint8_t[static_cast<size_t>(_totalButtonCount)])
    , _mavlinkActionManager(new MavlinkActionManager(SettingsManager::instance()->mavlinkActionsSettings()->joystickActionsFile(), this))
    , _assignableButtonActions(new QmlObjectListModel(this))
{
    // qCDebug(JoystickLog) << Q_FUNC_INFO << this;

    for (int i = 0; i < _axisCount; i++) {
        _rgAxisValues[i] = 0;
    }

    for (int i = 0; i < _totalButtonCount; i++) {
        _rgButtonValues[i] = BUTTON_UP;
        _buttonActionArray.append(nullptr);
    }

    _buildActionList(MultiVehicleManager::instance()->activeVehicle());
    _updateTXModeSettingsKey(MultiVehicleManager::instance()->activeVehicle());
    _loadSettings();

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &Joystick::_activeVehicleChanged);
    (void) connect(MultiVehicleManager::instance()->vehicles(), &QmlObjectListModel::countChanged, this, &Joystick::_vehicleCountChanged);
}

Joystick::~Joystick()
{
    if (!_exitThread) {
        qCWarning(JoystickLog) << "thread still running!";
    }

    delete[] _rgAxisValues;
    delete[] _rgCalibration;
    delete[] _rgButtonValues;

    _assignableButtonActions->clearAndDeleteContents();
    qDeleteAll(_buttonActionArray);

    // qCDebug(JoystickLog) << Q_FUNC_INFO << this;
}

void Joystick::stop()
{
    _exitThread = true;
    wait();
}

void Joystick::_setDefaultCalibration()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);

    _calibrated = settings.value(_calibratedSettingsKey, false).toBool();

    // Only set default calibrations if we do not have a calibration for this gamecontroller
    if (_calibrated) {
        return;
    }

    for (int axis = 0; axis < _axisCount; axis++) {
        Joystick::Calibration_t calibration;
        _rgCalibration[axis] = calibration;
    }

    _rgCalibration[1].reversed = true;
    _rgCalibration[3].reversed = true;

    // Default TX Mode 2 axis assignments for gamecontrollers
    _rgFunctionAxis[rollFunction] = 2;
    _rgFunctionAxis[pitchFunction] = 3;
    _rgFunctionAxis[yawFunction] = 0;
    _rgFunctionAxis[throttleFunction] = 1;

    _rgFunctionAxis[gimbalPitchFunction] = 4;
    _rgFunctionAxis[gimbalYawFunction] = 5;

    _exponential = 0;
    _accumulator = false;
    _deadband = false;
    _axisFrequencyHz = _defaultAxisFrequencyHz;
    _buttonFrequencyHz = _defaultButtonFrequencyHz;
    _throttleMode = ThrottleModeDownZero;
    _calibrated = true;
    _circleCorrection = false;

    _saveSettings();
}

void Joystick::_updateTXModeSettingsKey(Vehicle *activeVehicle)
{
    if (activeVehicle) {
        if (activeVehicle->fixedWing()) {
            _txModeSettingsKey = _fixedWingTXModeSettingsKey;
        } else if (activeVehicle->multiRotor()) {
            _txModeSettingsKey = _multiRotorTXModeSettingsKey;
        } else if (activeVehicle->rover()) {
            _txModeSettingsKey = _roverTXModeSettingsKey;
        } else if (activeVehicle->vtol()) {
            _txModeSettingsKey = _vtolTXModeSettingsKey;
        } else if (activeVehicle->sub()) {
            _txModeSettingsKey = _submarineTXModeSettingsKey;
        } else {
            _txModeSettingsKey = nullptr;
            qCWarning(JoystickLog) << "No valid joystick TXmode settings key for selected vehicle";
        }
    } else {
        _txModeSettingsKey = nullptr;
    }
}

void Joystick::_activeVehicleChanged(Vehicle *activeVehicle)
{
    _updateTXModeSettingsKey(activeVehicle);

    if (activeVehicle) {
        QSettings settings;
        settings.beginGroup(_settingsGroup);

        const int mode = settings.value(_txModeSettingsKey, activeVehicle->firmwarePlugin()->defaultJoystickTXMode()).toInt();
        setTXMode(mode);
    }
}

void Joystick::_vehicleCountChanged(int count)
{
    if (count == 0) {
        // then the last vehicle has been deleted
        qCDebug(JoystickLog) << "Stopping joystick thread due to last active vehicle deletion";
        this->stopPolling();
    }
}

void Joystick::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);

    Vehicle *const activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    if (_txModeSettingsKey && activeVehicle) {
        _transmitterMode = settings.value(_txModeSettingsKey, activeVehicle->firmwarePlugin()->defaultJoystickTXMode()).toInt();
    }

    settings.beginGroup(_name);

    bool badSettings = false;
    bool convertOk;

    _calibrated = settings.value(_calibratedSettingsKey, false).toBool();
    _exponential = settings.value(_exponentialSettingsKey, 0).toFloat();
    _accumulator = settings.value(_accumulatorSettingsKey, false).toBool();
    _deadband = settings.value(_deadbandSettingsKey, false).toBool();
    _axisFrequencyHz = settings.value(_axisFrequencySettingsKey, _defaultAxisFrequencyHz).toFloat(&convertOk);
    _buttonFrequencyHz = settings.value(_buttonFrequencySettingsKey, _defaultButtonFrequencyHz).toFloat(&convertOk);
    _circleCorrection  = settings.value(_circleCorrectionSettingsKey, false).toBool();
    _negativeThrust = settings.value(_negativeThrustSettingsKey, false).toBool();
    _throttleMode = static_cast<ThrottleMode_t>(settings.value(_throttleModeSettingsKey, ThrottleModeDownZero).toInt(&convertOk));

    badSettings |= !convertOk;

    qCDebug(JoystickLog) << Q_FUNC_INFO << "calibrated:txmode:throttlemode:exponential:deadband:badsettings" << _calibrated << _transmitterMode << _throttleMode << _exponential << _deadband << badSettings;

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

        qCDebug(JoystickLog) << Q_FUNC_INFO << "axis:min:max:trim:reversed:deadband:badsettings" << axis << calibration->min << calibration->max << calibration->center << calibration->reversed << calibration->deadband << badSettings;
    }

    int workingAxis = 0;
    for (int function = 0; function < maxFunction; function++) {
        int functionAxis = settings.value(_rgFunctionSettingsKey[function], -1).toInt(&convertOk);
        badSettings |= (!convertOk || (functionAxis >= _axisCount));

        if (functionAxis >= 0) {
            workingAxis++;
        }

        if (functionAxis < _axisCount) {
            _rgFunctionAxis[function] = functionAxis;
        }

        qCDebug(JoystickLog) << Q_FUNC_INFO << "function:axis:badsettings" << function << functionAxis << badSettings;
    }

    badSettings |= (workingAxis < 4);

    // FunctionAxis mappings are always stored in TX mode 2
    // Remap to stored TX mode in settings
    _remapAxes (2, _transmitterMode, _rgFunctionAxis);

    for (int button = 0; button < _totalButtonCount; button++) {
        const QString buttonAction = settings.value(QString(_buttonActionNameKey).arg(button), QString()).toString();
        if (buttonAction.isEmpty() || (buttonAction == _buttonActionNone)) {
            continue;
        }

        if (_buttonActionArray[button]) {
            delete _buttonActionArray[button];
            _buttonActionArray[button] = nullptr;
        }

        AssignedButtonAction *const ap = new AssignedButtonAction(buttonAction);
        ap->repeat = settings.value(QString(_buttonActionRepeatKey).arg(button), false).toBool();
        _buttonActionArray[button] = ap;
        _buttonActionArray[button]->buttonTime.start();

        qCDebug(JoystickLog) << Q_FUNC_INFO << "button:action" << button << _buttonActionArray[button]->action << _buttonActionArray[button]->repeat;
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
            settings.setValue(QString(_buttonActionNameKey).arg(button), _buttonActionArray[button]->action);
            settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
            qCDebug(JoystickLog) << Q_FUNC_INFO << "button:action:repeat" << button <<  _buttonActionArray[button]->action << _buttonActionArray[button]->repeat;
        }
    }
}

void Joystick::_saveSettings()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);

    if (_txModeSettingsKey) {
        settings.setValue(_txModeSettingsKey, _transmitterMode);
    }

    settings.beginGroup(_name);
    settings.setValue(_calibratedSettingsKey, _calibrated);
    settings.setValue(_exponentialSettingsKey, _exponential);
    settings.setValue(_accumulatorSettingsKey, _accumulator);
    settings.setValue(_deadbandSettingsKey, _deadband);
    settings.setValue(_axisFrequencySettingsKey, _axisFrequencyHz);
    settings.setValue(_buttonFrequencySettingsKey, _buttonFrequencyHz);
    settings.setValue(_throttleModeSettingsKey, _throttleMode);
    settings.setValue(_negativeThrustSettingsKey, _negativeThrust);
    settings.setValue(_circleCorrectionSettingsKey, _circleCorrection);

    qCDebug(JoystickLog) << Q_FUNC_INFO << "calibrated:throttlemode:deadband:txmode" << _calibrated << _throttleMode << _deadband << _circleCorrection << _transmitterMode;

    const QString minTpl("Axis%1Min");
    const QString maxTpl("Axis%1Max");
    const QString trimTpl("Axis%1Trim");
    const QString revTpl("Axis%1Rev");
    const QString deadbndTpl("Axis%1Deadbnd");

    for (int axis = 0; axis < _axisCount; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];
        settings.setValue(trimTpl.arg(axis), calibration->center);
        settings.setValue(minTpl.arg(axis), calibration->min);
        settings.setValue(maxTpl.arg(axis), calibration->max);
        settings.setValue(revTpl.arg(axis), calibration->reversed);
        settings.setValue(deadbndTpl.arg(axis), calibration->deadband);
        qCDebug(JoystickLog) << Q_FUNC_INFO
                             << "name:axis:min:max:trim:reversed:deadband"
                             << _name
                             << axis
                             << calibration->min
                             << calibration->max
                             << calibration->center
                             << calibration->reversed
                             << calibration->deadband;
    }

    // Write mode 2 mappings without changing mapping currently in use
    int temp[maxFunction];
    _remapAxes(_transmitterMode, 2, temp);

    for (int function = 0; function < maxFunction; function++) {
        settings.setValue(_rgFunctionSettingsKey[function], temp[function]);
        qCDebug(JoystickLog) << Q_FUNC_INFO << "name:function:axis" << _name << function << _rgFunctionSettingsKey[function];
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

void Joystick::_remapAxes(int currentMode, int newMode, int (&newMapping)[maxFunction])
{
    int temp[maxFunction];

    for (int function = 0; function < maxFunction; function++) {
        temp[_mapFunctionMode(newMode, function)] = _rgFunctionAxis[_mapFunctionMode(currentMode, function)];
    }

    for (int function = 0; function < maxFunction; function++) {
        newMapping[function] = temp[function];
    }
}

void Joystick::setTXMode(int mode)
{
    if ((mode > 0) && (mode <= 4)) {
        _remapAxes(_transmitterMode, mode, _rgFunctionAxis);
        _transmitterMode = mode;
        _saveSettings();
    } else {
        qCWarning(JoystickLog) << "Invalid mode:" << mode;
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

        const int sleep = qMin(static_cast<int>(1000.0f / _maxAxisFrequencyHz), static_cast<int>(1000.0f / _maxButtonFrequencyHz)) / 2;
        QThread::msleep(sleep);
    }

    _close();
}

void Joystick::_handleButtons()
{
    int lastBbuttonValues[256]{};

    //-- Update button states
    for (int buttonIndex = 0; buttonIndex < _buttonCount; buttonIndex++) {
        const bool newButtonValue = _getButton(buttonIndex);
        if (buttonIndex < 256) {
            lastBbuttonValues[buttonIndex] = _rgButtonValues[buttonIndex];
        }

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
                lastBbuttonValues[rgButtonValueIndex] = _rgButtonValues[rgButtonValueIndex];
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
                    const int buttonDelay = static_cast<int>(1000.0f / _buttonFrequencyHz);
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
                if ((lastBbuttonValues[buttonIndex] == BUTTON_DOWN) || (lastBbuttonValues[buttonIndex] == BUTTON_REPEAT)) {
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

void Joystick::_handleAxis()
{
    const int axisDelay = static_cast<int>(1000.0f / _axisFrequencyHz);
    if (_axisTime.elapsed() <= axisDelay) {
        return;
    }

    _axisTime.start();

    for (int axisIndex = 0; axisIndex < _axisCount; axisIndex++) {
        int newAxisValue = _getAxis(axisIndex);
        // Calibration code requires signal to be emitted even if value hasn't changed
        _rgAxisValues[axisIndex] = newAxisValue;
        emit rawAxisValueChanged(axisIndex, newAxisValue);
    }

    if (!_activeVehicle->joystickEnabled() || _calibrationMode || !_calibrated) {
        return;
    }

    int axis = _rgFunctionAxis[rollFunction];
    float roll = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis], _deadband);

    axis = _rgFunctionAxis[pitchFunction];
    float pitch = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis], _deadband);

    axis = _rgFunctionAxis[yawFunction];
    float yaw = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis],_deadband);

    axis = _rgFunctionAxis[throttleFunction];
    float throttle = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis], (_throttleMode == ThrottleModeDownZero) ? false :_deadband);

    float gimbalPitch = 0.0f;
    if (_axisCount > 4) {
        axis = _rgFunctionAxis[gimbalPitchFunction];
        gimbalPitch = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis],_deadband);
    }

    float gimbalYaw = 0.0f;
    if (_axisCount > 5) {
        axis = _rgFunctionAxis[gimbalYawFunction];
        gimbalYaw = _adjustRange(_rgAxisValues[axis],   _rgCalibration[axis],_deadband);
    }

    if (_accumulator) {
        static float throttle_accu = 0.f;
        throttle_accu += (throttle * (40 / 1000.f)); // for throttle to change from min to max it will take 1000ms (40ms is a loop time)
        throttle_accu = std::max(static_cast<float>(-1.f), std::min(throttle_accu, static_cast<float>(1.f)));
        throttle = throttle_accu;
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

    if ( _exponential < -0.01f) {
        // Exponential (0% to -50% range like most RC radios)
        // _exponential is set by a slider in joystickConfigAdvanced.qml
        // Calculate new RPY with exponential applied
        roll = -_exponential*powf(roll, 3) + ((1+_exponential) * roll);
        pitch = -_exponential*powf(pitch,3) + ((1+_exponential) * pitch);
        yaw = -_exponential*powf(yaw,  3) + ((1+_exponential) * yaw);
    }

    // Adjust throttle to 0:1 range
    if ((_throttleMode == ThrottleModeCenterZero) && (_activeVehicle->supportsThrottleModeCenterZero())) {
        if (!_activeVehicle->supportsNegativeThrust() || !_negativeThrust) {
            throttle = std::max(0.0f, throttle);
        }
    } else {
        throttle = (throttle + 1.0f) / 2.0f;
    }

    qCDebug(JoystickValuesLog) << "name:roll:pitch:yaw:throttle:gimbalPitch:gimbalYaw" << name() << roll << -pitch << yaw << throttle << gimbalPitch << gimbalYaw;

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

    const uint16_t shortButtons = static_cast<uint16_t>(buttonPressedBits & 0xFFFF);
    _activeVehicle->sendJoystickDataThreadSafe(roll, pitch, yaw, throttle, shortButtons);
}

void Joystick::startPolling(Vehicle* vehicle)
{
    if (vehicle) {
        if (_activeVehicle) {
            (void) disconnect(this, &Joystick::setArmed, _activeVehicle, &Vehicle::setArmedShowError);
            (void) disconnect(this, &Joystick::setVtolInFwdFlight, _activeVehicle, &Vehicle::setVtolInFwdFlight);
            (void) disconnect(this, &Joystick::setFlightMode, _activeVehicle, &Vehicle::setFlightMode);
            (void) disconnect(this, &Joystick::gimbalYawLock, _activeVehicle->gimbalController(), &GimbalController::gimbalYawLock);
            (void) disconnect(this, &Joystick::centerGimbal, _activeVehicle->gimbalController(), &GimbalController::centerGimbal);
            (void) disconnect(this, &Joystick::gimbalPitchStart, _activeVehicle->gimbalController(), &GimbalController::gimbalPitchStart);
            (void) disconnect(this, &Joystick::gimbalYawStart, _activeVehicle->gimbalController(), &GimbalController::gimbalYawStart);
            (void) disconnect(this, &Joystick::gimbalPitchStop, _activeVehicle->gimbalController(), &GimbalController::gimbalPitchStop);
            (void) disconnect(this, &Joystick::gimbalYawStop, _activeVehicle->gimbalController(), &GimbalController::gimbalYawStop);
            (void) disconnect(this, &Joystick::emergencyStop, _activeVehicle, &Vehicle::emergencyStop);
            (void) disconnect(this, &Joystick::gripperAction, _activeVehicle, &Vehicle::setGripperAction);
            (void) disconnect(this, &Joystick::landingGearDeploy, _activeVehicle, &Vehicle::landingGearDeploy);
            (void) disconnect(this, &Joystick::landingGearRetract, _activeVehicle, &Vehicle::landingGearRetract);
            (void) disconnect(_activeVehicle, &Vehicle::flightModesChanged, this, &Joystick::_flightModesChanged);
        }

        _activeVehicle = vehicle;

        // If joystick is not calibrated, disable it
        if ((axisCount() != 0) && !_calibrated) {
            vehicle->setJoystickEnabled(false);
        }

        // Update qml in case of joystick transition
        emit calibratedChanged(_calibrated);

        _buildActionList(vehicle);

        if (vehicle->joystickEnabled()) {
            _pollingStartedForCalibration = false;
            (void) connect(this, &Joystick::setArmed, _activeVehicle, &Vehicle::setArmedShowError);
            (void) connect(this, &Joystick::setVtolInFwdFlight, _activeVehicle, &Vehicle::setVtolInFwdFlight);
            (void) connect(this, &Joystick::setFlightMode, _activeVehicle, &Vehicle::setFlightMode);
            (void) connect(this, &Joystick::gimbalYawLock, _activeVehicle->gimbalController(), &GimbalController::gimbalYawLock);
            (void) connect(this, &Joystick::centerGimbal, _activeVehicle->gimbalController(), &GimbalController::centerGimbal);
            (void) connect(this, &Joystick::gimbalPitchStart,   _activeVehicle->gimbalController(), &GimbalController::gimbalPitchStart);
            (void) connect(this, &Joystick::gimbalYawStart,     _activeVehicle->gimbalController(), &GimbalController::gimbalYawStart);
            (void) connect(this, &Joystick::gimbalPitchStop,    _activeVehicle->gimbalController(), &GimbalController::gimbalPitchStop);
            (void) connect(this, &Joystick::gimbalYawStop,      _activeVehicle->gimbalController(), &GimbalController::gimbalYawStop);
            (void) connect(this, &Joystick::emergencyStop, _activeVehicle, &Vehicle::emergencyStop);
            (void) connect(this, &Joystick::gripperAction, _activeVehicle, &Vehicle::setGripperAction);
            (void) connect(this, &Joystick::landingGearDeploy, _activeVehicle, &Vehicle::landingGearDeploy);
            (void) connect(this, &Joystick::landingGearRetract, _activeVehicle, &Vehicle::landingGearRetract);
            (void) connect(_activeVehicle, &Vehicle::flightModesChanged, this, &Joystick::_flightModesChanged);
        }
    }

    if (!isRunning()) {
        _exitThread = false;
        start();
    }
}

void Joystick::stopPolling()
{
    if (!isRunning()) {
        _activeVehicle = nullptr;
        return;
    }

    if (_activeVehicle && _activeVehicle->joystickEnabled()) {
        (void) disconnect(this, &Joystick::setArmed, _activeVehicle, &Vehicle::setArmedShowError);
        (void) disconnect(this, &Joystick::setVtolInFwdFlight, _activeVehicle, &Vehicle::setVtolInFwdFlight);
        (void) disconnect(this, &Joystick::setFlightMode, _activeVehicle, &Vehicle::setFlightMode);
        (void) disconnect(this, &Joystick::gimbalYawLock, _activeVehicle->gimbalController(), &GimbalController::gimbalYawLock);
        (void) disconnect(this, &Joystick::centerGimbal, _activeVehicle->gimbalController(), &GimbalController::centerGimbal);
        (void) disconnect(this, &Joystick::gimbalPitchStart, _activeVehicle->gimbalController(), &GimbalController::gimbalPitchStart);
        (void) disconnect(this, &Joystick::gimbalYawStart, _activeVehicle->gimbalController(), &GimbalController::gimbalYawStart);
        (void) disconnect(this, &Joystick::gimbalPitchStop, _activeVehicle->gimbalController(), &GimbalController::gimbalPitchStop);
        (void) disconnect(this, &Joystick::gimbalYawStop, _activeVehicle->gimbalController(), &GimbalController::gimbalYawStop);
        (void) disconnect(this, &Joystick::gripperAction, _activeVehicle, &Vehicle::setGripperAction);
        (void) disconnect(this, &Joystick::landingGearDeploy, _activeVehicle, &Vehicle::landingGearDeploy);
        (void) disconnect(this, &Joystick::landingGearRetract, _activeVehicle, &Vehicle::landingGearRetract);
        (void) disconnect(_activeVehicle, &Vehicle::flightModesChanged, this, &Joystick::_flightModesChanged);
        _activeVehicle = nullptr;
    }

    _exitThread = true;
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
    if ((static_cast<int>(function) < 0) || (function >= maxFunction)) {
        qCWarning(JoystickLog) << "Invalid function" << function;
    }

    return _rgFunctionAxis[function];
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
    settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
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

    qCWarning(JoystickLog) << Q_FUNC_INFO << button << action;

    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    if (action.isEmpty() || action == _buttonActionNone) {
        if (_buttonActionArray[button]) {
            delete _buttonActionArray[button];
            _buttonActionArray[button] = nullptr;
            settings.remove(QString(_buttonActionNameKey).arg(button));
            settings.remove(QString(_buttonActionRepeatKey).arg(button));
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

        settings.setValue(QString(_buttonActionNameKey).arg(button), _buttonActionArray[button]->action);
        settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
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

void Joystick::setThrottleMode(int mode)
{
    if (mode < 0 || (mode >= ThrottleModeMax)) {
        qCWarning(JoystickLog) << "Invalid throttle mode" << mode;
        return;
    }

    if (mode != _throttleMode) {
        _throttleMode = static_cast<ThrottleMode_t>(mode);
        if (_throttleMode == ThrottleModeDownZero) {
            setAccumulator(false);
        }

        _saveSettings();
        emit throttleModeChanged(_throttleMode);
    }
}

void Joystick::setNegativeThrust(bool allowNegative)
{
    if (_negativeThrust != allowNegative) {
        _negativeThrust = allowNegative;
        _saveSettings();
        emit negativeThrustChanged(_negativeThrust);
    }
}

void Joystick::setExponential(float expo)
{
    if (expo != _exponential) {
        _exponential = expo;
        _saveSettings();
        emit exponentialChanged(_exponential);
    }
}

void Joystick::setAccumulator(bool accu)
{
    if (accu != _accumulator) {
        _accumulator = accu;
        _saveSettings();
        emit accumulatorChanged(_accumulator);
    }
}

void Joystick::setDeadband(bool deadband)
{
    if (deadband != _deadband) {
        _deadband = deadband;
        _saveSettings();
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

void Joystick::setAxisFrequency(float val)
{
    float result = val;
    result = qMax(_minAxisFrequencyHz, result);
    result = qMin(_maxAxisFrequencyHz, result);

    if (result != _axisFrequencyHz) {
        _axisFrequencyHz = result;
        _saveSettings();
        emit axisFrequencyHzChanged();
    }
}

void Joystick::setButtonFrequency(float val)
{
    float result = val;
    result = qMax(_minButtonFrequencyHz, result);
    result = qMin(_maxButtonFrequencyHz, result);

    if (result != _buttonFrequencyHz) {
        _buttonFrequencyHz = result;
        _saveSettings();
        emit buttonFrequencyHzChanged();
    }
}

void Joystick::setCalibrationMode(bool calibrating)
{
    _calibrationMode = calibrating;
    if (calibrating && !isRunning()) {
        _pollingStartedForCalibration = true;
        startPolling(MultiVehicleManager::instance()->activeVehicle());
    } else if (_pollingStartedForCalibration) {
        stopPolling();
    }
}

void Joystick::_executeButtonAction(const QString &action, bool buttonDown)
{
    if (!_activeVehicle || !_activeVehicle->joystickEnabled() || (action == _buttonActionNone)) {
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
            emit setArmed(!_activeVehicle->armed());
        }
    } else if (action == _buttonActionVTOLFixedWing) {
        if (buttonDown) {
            emit setVtolInFwdFlight(true);
        }
    } else if (action == _buttonActionVTOLMultiRotor) {
        if (buttonDown) {
            emit setVtolInFwdFlight(false);
        }
    } else if (_activeVehicle->flightModes().contains(action)) {
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
    } else if (action == _buttonActionGripperGrab) {
        if (buttonDown) {
            emit gripperAction(GRIPPER_ACTION_GRAB);
        }
    } else if (action == _buttonActionGripperRelease) {
        if (buttonDown) {
            emit gripperAction(GRIPPER_ACTION_RELEASE);
        }
    } else if (action == _buttonActionLandingGearDeploy) {
        if (buttonDown) {
            emit landingGearDeploy();
        }
    } else if (action == _buttonActionLandingGearRetract) {
        if (buttonDown) {
            emit landingGearRetract();
        }
    } else {
        if (buttonDown && _activeVehicle) {
            emit unknownAction(action);
            for (int i = 0; i<_mavlinkActionManager->actions()->count(); i++) {
                MavlinkAction *const mavlinkAction = _mavlinkActionManager->actions()->value<MavlinkAction*>(i);
                if (action == mavlinkAction->label()) {
                    mavlinkAction->sendTo(_activeVehicle);
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

void Joystick::_buildActionList(Vehicle *activeVehicle)
{
    if (_assignableButtonActions->count()) {
        _assignableButtonActions->clearAndDeleteContents();
    }
    _availableActionTitles.clear();

    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionNone));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionArm));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionDisarm));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionToggleArm));
    if (activeVehicle) {
        const QStringList list = activeVehicle->flightModes();
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
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGripperGrab));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionGripperRelease));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionLandingGearDeploy));
    _assignableButtonActions->append(new AssignableButtonAction(_buttonActionLandingGearRetract));

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
