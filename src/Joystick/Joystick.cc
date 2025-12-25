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
#include "JoystickAxisSettings.h"
#include "JoystickButtonActionSettings.h"

#include <QtCore/QSettings>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(JoystickLog, "Joystick.Joystick")
QGC_LOGGING_CATEGORY(JoystickVerboseLog, "Joystick.Joystick:verbose")

/*===========================================================================*/

AssignedButtonAction::AssignedButtonAction(const QString &actionName_, bool repeat_)
    : actionName(actionName_)
    , repeat(repeat_)
{
    // qCDebug(JoystickLog) << Q_FUNC_INFO << this;
}

AvailableButtonAction::AvailableButtonAction(const QString &actionName_, bool canRepeat_, QObject *parent)
    : QObject(parent)
    , _actionName(actionName_)
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
    , _rgCalibration(new AxisCalibration_t[static_cast<size_t>(_axisCount)])
    , _rgButtonValues(new uint8_t[static_cast<size_t>(_totalButtonCount)])
    , _mavlinkActionManager(new MavlinkActionManager(SettingsManager::instance()->mavlinkActionsSettings()->joystickActionsFile(), this))
    , _availableButtonActions(new QmlObjectListModel(this))
    , _joystickSettings(name, _axisCount, _totalButtonCount)
{
    qCDebug(JoystickLog)
        << name
        << "axisCount" << axisCount
        << "buttonCount" << buttonCount
        << "hatCount" << hatCount
        << this;

    for (int i = 0; i < _totalButtonCount; i++) {
        _rgButtonValues[i] = BUTTON_UP;
        _assignedButtonAcions.append(nullptr);
    }

    _loadFromSettingsIntoCalibrationData();
}

Joystick::~Joystick()
{
    _exitThread = true;
    wait();

    delete[] _rgCalibration;
    delete[] _rgButtonValues;

    _availableButtonActions->clearAndDeleteContents();
    qDeleteAll(_assignedButtonAcions);

    // qCDebug(JoystickLog) << Q_FUNC_INFO << this;
}

void Joystick::_loadFromSettingsIntoCalibrationData()
{
    bool calibrated = _joystickSettings.calibrated()->rawValue().toBool();
    int transmitterMode = _joystickSettings.transmitterMode()->rawValue().toInt();

    qCDebug(JoystickLog) << name();
    qCDebug(JoystickLog)
        << "    "
        << "calibrated" << calibrated
        << "throttleAccumulator" << _joystickSettings.throttleAccumulator()->rawValue().toBool()
        << "axisFrequencyHz" << _joystickSettings.axisFrequencyHz()->rawValue().toDouble()
        << "buttonFrequencyHz" << _joystickSettings.buttonFrequencyHz()->rawValue().toDouble()
        << "throttleModeCenterZero" << _joystickSettings.throttleModeCenterZero()->rawValue().toBool()
        << "negativeThrust" << _joystickSettings.negativeThrust()->rawValue().toBool()
        << "circleCorrection" << _joystickSettings.circleCorrection()->rawValue().toBool()
        << "exponentialPct" << _joystickSettings.exponentialPct()->rawValue().toDouble()
        << "enableManualControlExtensions" << _joystickSettings.enableManualControlExtensions()->rawValue().toBool()
        << "useDeadband" << _joystickSettings.useDeadband()->rawValue().toBool()
        << "transmitterMode" << transmitterMode;

    // First we validate the settings data in case we need to reset
    bool requiresReset = false;
    for (int axis = 0; axis < _axisCount; axis++) {
        AxisCalibration_t *const calibration = &_rgCalibration[axis];
        JoystickAxisSettings *axisSettings = _joystickSettings.getJoystickAxisSettingsByIndex(axis);

        if (!axisSettings) {
            qCWarning(JoystickLog) << "Internal Error: Unable to get settings for axis" << axis;
            continue;
        }

        int function = axisSettings->function()->rawValue().toInt();
        if (function < 0 || function >= maxAxisFunction) {
            qCWarning(JoystickLog) << "Invalid function" << function << "for axis" << axis;
            continue;
        }

        if (calibrated && function == maxAxisFunction) {
            qCWarning(JoystickLog) << "Joystick is marked calibrated but axis" << axis << "has no function assigned";
            continue;
        }
    }

    for (int axis = 0; axis < _axisCount; axis++) {
        AxisCalibration_t *const calibration = &_rgCalibration[axis];
        JoystickAxisSettings *axisSettings = _joystickSettings.getJoystickAxisSettingsByIndex(axis);

        if (!axisSettings) {
            qCWarning(JoystickLog) << "Internal Error: Unable to get settings for axis" << axis;
            continue;
        }

        calibration->center = axisSettings->center()->rawValue().toInt();
        calibration->min = axisSettings->min()->rawValue().toInt();
        calibration->max = axisSettings->max()->rawValue().toInt();
        calibration->deadband = axisSettings->deadband()->rawValue().toInt();
        calibration->reversed = axisSettings->reversed()->rawValue().toBool();
        int function = axisSettings->function()->rawValue().toInt();
        _rgFunctionAxis[function] = axis;

        qDebug(JoystickLog)
            << "    "
            << "axis" << axis
            << "min" << calibration->min
            << "max" << calibration->max
            << "center" << calibration->center
            << "reversed" << calibration->reversed
            << "deadband" << calibration->deadband
            << "function" << function;
    }

    // FunctionAxis mappings are always stored in TX mode 2
    // Remap to stored TX mode in settings for UI display
    _remapAxes (2, transmitterMode, _rgFunctionAxis);

    // Rebuild assigned button actions
    qDeleteAll(_assignedButtonAcions);
    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        JoystickButtonActionSettings *buttonSettings = _joystickSettings.getJoystickButtonActionSettingsByIndex(buttonIndex);

        QString actionName = buttonSettings->actionName()->rawValue().toString();
        bool repeat = buttonSettings->repeat()->rawValue().toBool();

        if (actionName.isEmpty() || (actionName == _buttonActionNone)) {
            continue;
        }

        AssignedButtonAction *buttonAction = new AssignedButtonAction(actionName, repeat);
        _assignedButtonAcions[buttonIndex] = buttonAction;
        _assignedButtonAcions[buttonIndex]->buttonElapsedTimer.start();

        qCDebug(JoystickLog)
            << "    "
            << "button" << buttonIndex
            << "actionName" << actionName
            << "repeat" << repeat;
    }
}

void Joystick::_saveFromCalibrationDataIntoSettings()
{
    bool calibrated = _joystickSettings.calibrated()->rawValue().toBool();
    int transmitterMode = _joystickSettings.transmitterMode()->rawValue().toInt();

    qCDebug(JoystickLog) << name();
    qCDebug(JoystickLog)
        << "    "
        << "calibrated" << calibrated
        << "throttleAccumulator" << _joystickSettings.throttleAccumulator()->rawValue().toBool()
        << "axisFrequencyHz" << _joystickSettings.axisFrequencyHz()->rawValue().toDouble()
        << "buttonFrequencyHz" << _joystickSettings.buttonFrequencyHz()->rawValue().toDouble()
        << "throttleModeCenterZero" << _joystickSettings.throttleModeCenterZero()->rawValue().toBool()
        << "negativeThrust" << _joystickSettings.negativeThrust()->rawValue().toBool()
        << "circleCorrection" << _joystickSettings.circleCorrection()->rawValue().toBool()
        << "exponentialPct" << _joystickSettings.exponentialPct()->rawValue().toDouble()
        << "enableManualControlExtensions" << _joystickSettings.enableManualControlExtensions()->rawValue().toBool()
        << "useDeadband" << _joystickSettings.useDeadband()->rawValue().toBool()
        << "transmitterMode" << transmitterMode;


    // FunctionAxis mappings are always stored in TX mode 2
    // Temporarily remap from calibration TX mode to mode 2 for storage
    _remapAxes (transmitterMode, 2, _rgFunctionAxis);

    for (int axis = 0; axis < _axisCount; axis++) {
        AxisCalibration_t *const calibration = &_rgCalibration[axis];
        JoystickAxisSettings *axisSettings = _joystickSettings.getJoystickAxisSettingsByIndex(axis);

        if (!axisSettings) {
            qCWarning(JoystickLog) << "Internal Error: Unable to get settings for axis" << axis;
            continue;
        }

        axisSettings->center()->setRawValue(calibration->center);
        axisSettings->min()->setRawValue(calibration->min);
        axisSettings->max()->setRawValue(calibration->max);
        axisSettings->deadband()->setRawValue(calibration->deadband);
        axisSettings->reversed()->setRawValue(calibration->reversed);
        AxisFunction_t function = _getFunctionForAxis(axis);
        axisSettings->function()->setRawValue(function);

        qDebug(JoystickLog)
            << "    "
            << "axis" << axis
            << "min" << calibration->min
            << "max" << calibration->max
            << "center" << calibration->center
            << "reversed" << calibration->reversed
            << "deadband" << calibration->deadband
            << "function" << function;
    }

    // Map back to current TX mode
    _remapAxes (2, transmitterMode, _rgFunctionAxis);

    // Save button actions
    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        JoystickButtonActionSettings *buttonActionSettings = _joystickSettings.getJoystickButtonActionSettingsByIndex(buttonIndex);

        AssignedButtonAction *buttonAction = _assignedButtonAcions[buttonIndex];

        if (buttonAction) {
            buttonActionSettings->actionName()->setRawValue(buttonAction->actionName);
            buttonActionSettings->repeat()->setRawValue(buttonAction->repeat);
            qCDebug(JoystickLog)
                << "    "
                << "button" << buttonIndex
                << "actionName" << buttonAction->actionName
                << "repeat" << buttonAction->repeat;
            continue;
        } else {
            buttonActionSettings->actionName()->setRawValue(_buttonActionNone);
            buttonActionSettings->repeat()->setRawValue(false);
        }
    }
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

void Joystick::_remapAxes(int fromMode, int toMode, int (&newMapping)[maxAxisFunction])
{
    int temp[maxAxisFunction];

    for (int function = 0; function < maxAxisFunction; function++) {
        temp[_mapFunctionMode(toMode, function)] = _rgFunctionAxis[_mapFunctionMode(fromMode, function)];
    }

    for (int function = 0; function < maxAxisFunction; function++) {
        newMapping[function] = temp[function];
    }
}

void Joystick::run()
{
    _open();

    _axisElapsedTimer.start();

    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        if (_assignedButtonAcions[buttonIndex]) {
            _assignedButtonAcions[buttonIndex]->buttonElapsedTimer.start();
        }
    }

    double maxAxisFrequencyHz = _joystickSettings.axisFrequencyHz()->rawMin().toDouble();
    double maxButtonFrequencyHz = _joystickSettings.buttonFrequencyHz()->rawMin().toDouble();

    while (!_exitThread) {
        _update();
        _handleButtons();
        if (axisCount() != 0) {
            _handleAxis();
        }

        const int sleep = qMin(static_cast<int>(1000.0 / maxAxisFrequencyHz), static_cast<int>(1000.0 / maxButtonFrequencyHz)) / 2;
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
        if (!_joystickSettings.calibrated()->rawValue().toBool()) {
            return;
        }

        //-- Process button press/release
        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            if ((_rgButtonValues[buttonIndex] == BUTTON_DOWN) || (_rgButtonValues[buttonIndex] == BUTTON_REPEAT)) {
                if (_assignedButtonAcions[buttonIndex]) {
                    const QString buttonAction = _assignedButtonAcions[buttonIndex]->actionName;
                    if (buttonAction.isEmpty() || (buttonAction == _buttonActionNone)) {
                        continue;
                    }

                    if (!_assignedButtonAcions[buttonIndex]->repeat) {
                        if (_rgButtonValues[buttonIndex] == BUTTON_DOWN) {
                            // Check for a multi-button action
                            QList<int> rgButtons = { buttonIndex };
                            bool executeButtonAction = true;
                            for (int multiIndex = 0; multiIndex < _totalButtonCount; multiIndex++) {
                                if (multiIndex == buttonIndex) {
                                    continue;
                                }

                                if (_assignedButtonAcions[multiIndex] && (_assignedButtonAcions[multiIndex]->actionName == buttonAction)) {
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
                        const int buttonDelay = static_cast<int>(1000.0 / _joystickSettings.buttonFrequencyHz()->rawValue().toDouble());
                        if (_assignedButtonAcions[buttonIndex]->buttonElapsedTimer.elapsed() > buttonDelay) {
                            _assignedButtonAcions[buttonIndex]->buttonElapsedTimer.start();
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
                        if (_assignedButtonAcions[buttonIndex]) {
                            const QString buttonAction = _assignedButtonAcions[buttonIndex]->actionName;
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

float Joystick::_adjustRange(int value, const AxisCalibration_t &calibration, bool withDeadbands)
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
    const int axisDelay = static_cast<int>(1000.0 / _joystickSettings.axisFrequencyHz()->rawValue().toDouble());
    if (_axisElapsedTimer.elapsed() <= axisDelay) {
        return;
    }

    _axisElapsedTimer.start();

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
        if (!_joystickSettings.calibrated()->rawValue().toBool()) {
            return;
        }

        bool useDeadband = _joystickSettings.useDeadband()->rawValue().toBool();
        bool throttleModeCenterZero = _joystickSettings.throttleModeCenterZero()->rawValue().toBool();
        bool negativeThrust = _joystickSettings.negativeThrust()->rawValue().toBool();
        bool circleCorrection = _joystickSettings.circleCorrection()->rawValue().toBool();
        bool throttleSmoothing = _joystickSettings.throttleAccumulator()->rawValue().toBool();
        bool enableManualControlExtensions = _joystickSettings.enableManualControlExtensions()->rawValue().toBool();
        double exponentialPercent = _joystickSettings.exponentialPct()->rawValue().toDouble();

        int axisIndex = _rgFunctionAxis[rollFunction];
        float roll = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        axisIndex = _rgFunctionAxis[pitchFunction];
        float pitch = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        axisIndex = _rgFunctionAxis[yawFunction];
        float yaw = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex],useDeadband);
        axisIndex = _rgFunctionAxis[throttleFunction];
        float throttle = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], throttleModeCenterZero ? useDeadband : false);

        float gimbalPitch = NAN;
        if (enableManualControlExtensions && _axisCount > 4) {
            axisIndex = _rgFunctionAxis[gimbalPitchFunction];
            gimbalPitch = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex],useDeadband);
        }

        float gimbalYaw = NAN;
        if (enableManualControlExtensions && _axisCount > 5) {
            axisIndex = _rgFunctionAxis[gimbalYawFunction];
            gimbalYaw = _adjustRange(_getAxisValue(axisIndex),   _rgCalibration[axisIndex],useDeadband);
        }

        if (throttleSmoothing) {
            static float throttleAccumulator = 0.f;
            throttleAccumulator += (throttle * (40 / 1000.f)); // for throttle to change from min to max it will take 1000ms (40ms is a loop time)
            throttleAccumulator = std::max(static_cast<float>(-1.f), std::min(throttleAccumulator, static_cast<float>(1.f)));
            throttle = throttleAccumulator;
        }

        if (circleCorrection) {
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

        if ( exponentialPercent > 0.0 ) {
            const float exponential = -static_cast<float>(exponentialPercent / 100.0);
            roll =  -exponential * powf(roll, 3) + ((1 + exponential) * roll);
            pitch = -exponential * powf(pitch,3) + ((1 + exponential) * pitch);
            yaw =   -exponential * powf(yaw,  3) + ((1 + exponential) * yaw);
        }

        // Adjust throttle to 0:1 range
        if (throttleModeCenterZero && vehicle->supportsThrottleModeCenterZero()) {
            if (!vehicle->supportsNegativeThrust() || !negativeThrust) {
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

void Joystick::setAxisCalibration(int axis, const AxisCalibration_t &calibration)
{
    if (!_validAxis(axis)) {
        return;
    }

    _rgCalibration[axis] = calibration;
}

Joystick::AxisCalibration_t Joystick::getAxisCalibration(int axis) const
{
    if (!_validAxis(axis)) {
        return AxisCalibration_t{};
    }

    return _rgCalibration[axis];
}

void Joystick::setFunctionAxis(AxisFunction_t function, int axis)
{
    if (!_validAxis(axis)) {
        return;
    }

    _rgFunctionAxis[function] = axis;
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
    if (!_validButton(button) || !_assignedButtonAcions[button]) {
        return;
    }

    _assignedButtonAcions[button]->repeat = repeat;
    _assignedButtonAcions[button]->buttonElapsedTimer.start();

    JoystickButtonActionSettings *buttonSettings = _joystickSettings.getJoystickButtonActionSettingsByIndex(button);
    if (buttonSettings) {
        buttonSettings->repeat()->setRawValue(repeat);
    }
}

bool Joystick::getButtonRepeat(int button)
{
    if (!_validButton(button) || !_assignedButtonAcions[button]) {
        return false;
    }

    return _assignedButtonAcions[button]->repeat;
}

void Joystick::setButtonAction(int button, const QString& actionName)
{
    if (!_validButton(button)) {
        return;
    }

    qCDebug(JoystickLog) << "button:actionName" << button << actionName;

    JoystickButtonActionSettings *buttonSettings = _joystickSettings.getJoystickButtonActionSettingsByIndex(button);

    if (actionName.isEmpty() || actionName == _buttonActionNone) {
        if (_assignedButtonAcions[button]) {
            delete _assignedButtonAcions[button];
            _assignedButtonAcions[button] = nullptr;
            if (buttonSettings) {
                buttonSettings->actionName()->setRawValue(_buttonActionNone);
                buttonSettings->repeat()->setRawValue(false);
            }
        }
    } else {
        if (!_assignedButtonAcions[button]) {
            _assignedButtonAcions[button] = new AssignedButtonAction(actionName, false /* repeat */);
        } else {
            _assignedButtonAcions[button]->actionName = actionName;
            //-- Make sure repeat is off if this action doesn't support repeats
            const int idx = _findAvailableButtonActionIndex(actionName);
            if (idx >= 0) {
                const AvailableButtonAction *const buttonAction = qobject_cast<const AvailableButtonAction*>(_availableButtonActions->get(idx));
                if (!buttonAction->canRepeat()) {
                    _assignedButtonAcions[button]->repeat = false;
                }
            }
        }

        if (buttonSettings) {
            buttonSettings->actionName()->setRawValue(_assignedButtonAcions[button]->actionName);
            buttonSettings->repeat()->setRawValue(_assignedButtonAcions[button]->repeat);
        }
    }

    emit buttonActionsChanged();
}

QString Joystick::getButtonAction(int button) const
{
    if (_validButton(button)) {
        if (_assignedButtonAcions[button]) {
            return _assignedButtonAcions[button]->actionName;
        }
    }

    return QString(_buttonActionNone);
}

QStringList Joystick::buttonActions() const
{
    QStringList list;
    for (int button = 0; button < _totalButtonCount; button++) {
        list << _assignedButtonAcions[button]->actionName;
    }

    return list;
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

int Joystick::_findAvailableButtonActionIndex(const QString &action)
{
    for (int i = 0; i < _availableButtonActions->count(); i++) {
        const AvailableButtonAction *const buttonAction = qobject_cast<const AvailableButtonAction*>(_availableButtonActions->get(i));
        if (buttonAction->action() == action) {
            return i;
        }
    }

    return -1;
}

void Joystick::_buildActionList(Vehicle *vehicle)
{
    if (_availableButtonActions->count()) {
        _availableButtonActions->clearAndDeleteContents();
    }
    _availableActionTitles.clear();

    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNone, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionArm, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionDisarm, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionToggleArm, false));
    if (vehicle) {
        const QStringList list = vehicle->flightModes();
        for (const QString &mode : list) {
            _availableButtonActions->append(new AvailableButtonAction(mode, false));
        }
    }

    _availableButtonActions->append(new AvailableButtonAction(_buttonActionVTOLFixedWing, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionVTOLMultiRotor, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionContinuousZoomIn, true));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionContinuousZoomOut, true));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStepZoomIn, true));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStepZoomOut, true));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNextStream, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionPreviousStream, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNextCamera, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionPreviousCamera, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionTriggerCamera, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStartVideoRecord, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStopVideoRecord, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionToggleVideoRecord, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalDown, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalUp, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalLeft, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalRight, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalCenter, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalYawLock, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalYawFollow, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionEmergencyStop, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperClose, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperOpen, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperStop, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionLandingGearDeploy, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionLandingGearRetract, false));
#ifndef QGC_NO_ARDUPILOT_DIALECT
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionMotorInterlockEnable, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionMotorInterlockDisable, false));
#endif

    const auto customActions = QGCCorePlugin::instance()->joystickActions();
    for (const auto &action : customActions) {
        _availableButtonActions->append(new AvailableButtonAction(action.name, action.canRepeat));
    }

    for (int i = 0; i < _mavlinkActionManager->actions()->count(); i++) {
        const MavlinkAction *const mavlinkAction = _mavlinkActionManager->actions()->value<const MavlinkAction*>(i);
        _availableButtonActions->append(new AvailableButtonAction(mavlinkAction->label(), false));
    }

    for (int i = 0; i < _availableButtonActions->count(); i++) {
        const AvailableButtonAction *const buttonAction = qobject_cast<const AvailableButtonAction*>(_availableButtonActions->get(i));
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

void Joystick::startConfiguration()
{
    _startPollingForConfiguration();
}

void Joystick::stopConfiguration()
{
    _stopPollingForConfiguration();
}

Joystick::AxisFunction_t Joystick::_getFunctionForAxis(int axis) const
{
    for (int function = 0; function < maxAxisFunction; function++) {
        if (_rgFunctionAxis[function] == axis) {
            return static_cast<AxisFunction_t>(function);
        }
    }

    return maxAxisFunction;
}
