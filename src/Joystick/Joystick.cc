/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "Joystick.h"
#include "QGC.h"
#include "AutoPilotPlugin.h"
#include "UAS.h"
#include "QGCApplication.h"
#include "VideoManager.h"
#include "QGCCameraManager.h"
#include "QGCCameraControl.h"

#include <QSettings>

QGC_LOGGING_CATEGORY(JoystickLog, "JoystickLog")
QGC_LOGGING_CATEGORY(JoystickValuesLog, "JoystickValuesLog")

const char* Joystick::_settingsGroup =                  "Joysticks";
const char* Joystick::_calibratedSettingsKey =          "Calibrated2"; // Increment number to force recalibration
const char* Joystick::_buttonActionSettingsKey =        "ButtonActionName%1";
const char* Joystick::_throttleModeSettingsKey =        "ThrottleMode";
const char* Joystick::_exponentialSettingsKey =         "Exponential";
const char* Joystick::_accumulatorSettingsKey =         "Accumulator";
const char* Joystick::_deadbandSettingsKey =            "Deadband";
const char* Joystick::_circleCorrectionSettingsKey =    "Circle_Correction";
const char* Joystick::_frequencySettingsKey =           "Frequency";
const char* Joystick::_txModeSettingsKey =              nullptr;
const char* Joystick::_fixedWingTXModeSettingsKey =     "TXMode_FixedWing";
const char* Joystick::_multiRotorTXModeSettingsKey =    "TXMode_MultiRotor";
const char* Joystick::_roverTXModeSettingsKey =         "TXMode_Rover";
const char* Joystick::_vtolTXModeSettingsKey =          "TXMode_VTOL";
const char* Joystick::_submarineTXModeSettingsKey =     "TXMode_Submarine";

const char* Joystick::_buttonActionArm =                QT_TR_NOOP("Arm");
const char* Joystick::_buttonActionDisarm =             QT_TR_NOOP("Disarm");
const char* Joystick::_buttonActionVTOLFixedWing =      QT_TR_NOOP("VTOL: Fixed Wing");
const char* Joystick::_buttonActionVTOLMultiRotor =     QT_TR_NOOP("VTOL: Multi-Rotor");
const char* Joystick::_buttonActionZoomIn =             QT_TR_NOOP("Zoom In");
const char* Joystick::_buttonActionZoomOut =            QT_TR_NOOP("Zoom Out");
const char* Joystick::_buttonActionNextStream =         QT_TR_NOOP("Next Video Stream");
const char* Joystick::_buttonActionPreviousStream =     QT_TR_NOOP("Previous Video Stream");
const char* Joystick::_buttonActionNextCamera =         QT_TR_NOOP("Next Camera");
const char* Joystick::_buttonActionPreviousCamera =     QT_TR_NOOP("Previous Camera");

const char* Joystick::_rgFunctionSettingsKey[Joystick::maxFunction] = {
    "RollAxis",
    "PitchAxis",
    "YawAxis",
    "ThrottleAxis"
};

int Joystick::_transmitterMode = 2;

Joystick::Joystick(const QString& name, int axisCount, int buttonCount, int hatCount, MultiVehicleManager* multiVehicleManager)
    : _exitThread(false)
    , _name(name)
    , _axisCount(axisCount)
    , _buttonCount(buttonCount)
    , _hatCount(hatCount)
    , _hatButtonCount(4*hatCount)
    , _totalButtonCount(_buttonCount+_hatButtonCount)
    , _calibrationMode(false)
    , _rgAxisValues(nullptr)
    , _rgCalibration(nullptr)
    , _rgButtonValues(nullptr)
    , _lastButtonBits(0)
    , _throttleMode(ThrottleModeDownZero)
    , _negativeThrust(false)
    , _exponential(0)
    , _accumulator(false)
    , _deadband(false)
    , _circleCorrection(true)
    , _frequency(25.0f)
    , _activeVehicle(nullptr)
    , _pollingStartedForCalibration(false)
    , _multiVehicleManager(multiVehicleManager)
{

    _rgAxisValues = new int[_axisCount];
    _rgCalibration = new Calibration_t[_axisCount];
    _rgButtonValues = new bool[_totalButtonCount];

    for (int i=0; i<_axisCount; i++) {
        _rgAxisValues[i] = 0;
    }
    for (int i=0; i<_totalButtonCount; i++) {
        _rgButtonValues[i] = false;
    }

    _updateTXModeSettingsKey(_multiVehicleManager->activeVehicle());

    _loadSettings();

    connect(_multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this, &Joystick::_activeVehicleChanged);
}

Joystick::~Joystick()
{
    // Crash out of the thread if it is still running
    terminate();
    wait();

    delete[] _rgAxisValues;
    delete[] _rgCalibration;
    delete[] _rgButtonValues;
}

void Joystick::_setDefaultCalibration(void) {
    QSettings   settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    _calibrated = settings.value(_calibratedSettingsKey, false).toBool();

    // Only set default calibrations if we do not have a calibration for this gamecontroller
    if(_calibrated) return;

    for(int axis = 0; axis < _axisCount; axis++) {
        Joystick::Calibration_t calibration;
        _rgCalibration[axis] = calibration;
    }

    _rgCalibration[1].reversed = true;
    _rgCalibration[3].reversed = true;

    // Default TX Mode 2 axis assignments for gamecontrollers
    _rgFunctionAxis[rollFunction]       = 2;
    _rgFunctionAxis[pitchFunction]      = 3;
    _rgFunctionAxis[yawFunction]        = 0;
    _rgFunctionAxis[throttleFunction]   = 1;

    _exponential = 0;
    _accumulator = false;
    _deadband = false;
    _circleCorrection = false;
    _frequency = 25.0f;
    _throttleMode = ThrottleModeDownZero;
    _calibrated = true;

    _saveSettings();
}

void Joystick::_updateTXModeSettingsKey(Vehicle* activeVehicle)
{
    if(activeVehicle) {
        if(activeVehicle->fixedWing()) {
            _txModeSettingsKey = _fixedWingTXModeSettingsKey;
        } else if(activeVehicle->multiRotor()) {
            _txModeSettingsKey = _multiRotorTXModeSettingsKey;
        } else if(activeVehicle->rover()) {
            _txModeSettingsKey = _roverTXModeSettingsKey;
        } else if(activeVehicle->vtol()) {
            _txModeSettingsKey = _vtolTXModeSettingsKey;
        } else if(activeVehicle->sub()) {
            _txModeSettingsKey = _submarineTXModeSettingsKey;
        } else {
            _txModeSettingsKey = nullptr;
            qWarning() << "No valid joystick TXmode settings key for selected vehicle";
            return;
        }
    } else {
        _txModeSettingsKey = nullptr;
    }
}

void Joystick::_activeVehicleChanged(Vehicle* activeVehicle)
{
    _updateTXModeSettingsKey(activeVehicle);

    if(activeVehicle) {
        QSettings settings;
        settings.beginGroup(_settingsGroup);
        int mode = settings.value(_txModeSettingsKey, activeVehicle->firmwarePlugin()->defaultJoystickTXMode()).toInt();

        setTXMode(mode);
    }
}

void Joystick::_loadSettings(void)
{
    QSettings   settings;

    settings.beginGroup(_settingsGroup);

    Vehicle* activeVehicle = _multiVehicleManager->activeVehicle();

    if(_txModeSettingsKey && activeVehicle)
        _transmitterMode = settings.value(_txModeSettingsKey, activeVehicle->firmwarePlugin()->defaultJoystickTXMode()).toInt();

    settings.beginGroup(_name);

    bool badSettings = false;
    bool convertOk;

    qCDebug(JoystickLog) << "_loadSettings " << _name;

    _calibrated = settings.value(_calibratedSettingsKey, false).toBool();
    _exponential = settings.value(_exponentialSettingsKey, 0).toFloat();
    _accumulator = settings.value(_accumulatorSettingsKey, false).toBool();
    _deadband = settings.value(_deadbandSettingsKey, false).toBool();
    _circleCorrection = settings.value(_circleCorrectionSettingsKey, false).toBool();
    _frequency = settings.value(_frequencySettingsKey, 25.0f).toFloat();

    _throttleMode = (ThrottleMode_t)settings.value(_throttleModeSettingsKey, ThrottleModeDownZero).toInt(&convertOk);
    badSettings |= !convertOk;

    qCDebug(JoystickLog) << "_loadSettings calibrated:txmode:throttlemode:exponential:deadband:badsettings" << _calibrated << _transmitterMode << _throttleMode << _exponential << _deadband << badSettings;

    QString minTpl  ("Axis%1Min");
    QString maxTpl  ("Axis%1Max");
    QString trimTpl ("Axis%1Trim");
    QString revTpl  ("Axis%1Rev");
    QString deadbndTpl  ("Axis%1Deadbnd");

    for (int axis=0; axis<_axisCount; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];

        calibration->center = settings.value(trimTpl.arg(axis), 0).toInt(&convertOk);
        badSettings |= !convertOk;

        calibration->min = settings.value(minTpl.arg(axis), -32768).toInt(&convertOk);
        badSettings |= !convertOk;

        calibration->max = settings.value(maxTpl.arg(axis), 32767).toInt(&convertOk);
        badSettings |= !convertOk;

        calibration->deadband = settings.value(deadbndTpl.arg(axis), 0).toInt(&convertOk);
        badSettings |= !convertOk;

        calibration->reversed = settings.value(revTpl.arg(axis), false).toBool();


        qCDebug(JoystickLog) << "_loadSettings axis:min:max:trim:reversed:deadband:badsettings" << axis << calibration->min << calibration->max << calibration->center << calibration->reversed << calibration->deadband << badSettings;
    }

    for (int function=0; function<maxFunction; function++) {
        int functionAxis;

        functionAxis = settings.value(_rgFunctionSettingsKey[function], -1).toInt(&convertOk);
        badSettings |= !convertOk || (functionAxis == -1);

        _rgFunctionAxis[function] = functionAxis;

        qCDebug(JoystickLog) << "_loadSettings function:axis:badsettings" << function << functionAxis << badSettings;
    }

    // FunctionAxis mappings are always stored in TX mode 2
    // Remap to stored TX mode in settings
    _remapAxes(2, _transmitterMode, _rgFunctionAxis);

    for (int button=0; button<_totalButtonCount; button++) {
        _rgButtonActions << settings.value(QString(_buttonActionSettingsKey).arg(button), QString()).toString();
        qCDebug(JoystickLog) << "_loadSettings button:action" << button << _rgButtonActions[button];
    }

    if (badSettings) {
        _calibrated = false;
        settings.setValue(_calibratedSettingsKey, false);
    }
}

void Joystick::_saveSettings(void)
{
    QSettings settings;

    settings.beginGroup(_settingsGroup);

    // Transmitter mode is static
    // Save the mode we are using
    if(_txModeSettingsKey)
        settings.setValue(_txModeSettingsKey, _transmitterMode);

    settings.beginGroup(_name);

    settings.setValue(_calibratedSettingsKey, _calibrated);
    settings.setValue(_exponentialSettingsKey, _exponential);
    settings.setValue(_accumulatorSettingsKey, _accumulator);
    settings.setValue(_deadbandSettingsKey, _deadband);
    settings.setValue(_circleCorrectionSettingsKey, _circleCorrection);
    settings.setValue(_frequencySettingsKey, _frequency);
    settings.setValue(_throttleModeSettingsKey, _throttleMode);

    qCDebug(JoystickLog) << "_saveSettings calibrated:throttlemode:deadband:txmode" << _calibrated << _throttleMode << _deadband << _circleCorrection << _transmitterMode;

    QString minTpl  ("Axis%1Min");
    QString maxTpl  ("Axis%1Max");
    QString trimTpl ("Axis%1Trim");
    QString revTpl  ("Axis%1Rev");
    QString deadbndTpl  ("Axis%1Deadbnd");

    for (int axis=0; axis<_axisCount; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];

        settings.setValue(trimTpl.arg(axis), calibration->center);
        settings.setValue(minTpl.arg(axis), calibration->min);
        settings.setValue(maxTpl.arg(axis), calibration->max);
        settings.setValue(revTpl.arg(axis), calibration->reversed);
        settings.setValue(deadbndTpl.arg(axis), calibration->deadband);

        qCDebug(JoystickLog) << "_saveSettings name:axis:min:max:trim:reversed:deadband"
                                << _name
                                << axis
                                << calibration->min
                                << calibration->max
                                << calibration->center
                                << calibration->reversed
                                << calibration->deadband;
    }

    // Always save function Axis mappings in TX Mode 2
    // Write mode 2 mappings without changing mapping currently in use
    int temp[maxFunction];
    _remapAxes(_transmitterMode, 2, temp);

    for (int function=0; function<maxFunction; function++) {
        settings.setValue(_rgFunctionSettingsKey[function], temp[function]);
        qCDebug(JoystickLog) << "_saveSettings name:function:axis" << _name << function << _rgFunctionSettingsKey[function];
    }

    for (int button=0; button<_totalButtonCount; button++) {
        settings.setValue(QString(_buttonActionSettingsKey).arg(button), _rgButtonActions[button]);
        qCDebug(JoystickLog) << "_saveSettings button:action" << button << _rgButtonActions[button];
    }
}

// Relative mappings of axis functions between different TX modes
int Joystick::_mapFunctionMode(int mode, int function) {

    static const int mapping[][4] = {
        { 2, 1, 0, 3 },
        { 2, 3, 0, 1 },
        { 0, 1, 2, 3 },
        { 0, 3, 2, 1 }};

    return mapping[mode-1][function];
}

// Remap current axis functions from current TX mode to new TX mode
void Joystick::_remapAxes(int currentMode, int newMode, int (&newMapping)[maxFunction]) {
    int temp[maxFunction];

    for(int function = 0; function < maxFunction; function++) {
        temp[_mapFunctionMode(newMode, function)] = _rgFunctionAxis[_mapFunctionMode(currentMode, function)];
    }

    for(int function = 0; function < maxFunction; function++) {
        newMapping[function] = temp[function];
    }

}

void Joystick::setTXMode(int mode) {
    if(mode > 0 && mode <= 4) {
        _remapAxes(_transmitterMode, mode, _rgFunctionAxis);
        _transmitterMode = mode;
        _saveSettings();
    } else {
        qCWarning(JoystickLog) << "Invalid mode:" << mode;
    }
}

/// Adjust the raw axis value to the -1:1 range given calibration information
float Joystick::_adjustRange(int value, Calibration_t calibration, bool withDeadbands)
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
        if (valueNormalized>calibration.deadband) {
            axisPercent = (valueNormalized - calibration.deadband) / (axisLength - calibration.deadband);
        } else if (valueNormalized<-calibration.deadband) {
            axisPercent = (valueNormalized + calibration.deadband) / (axisLength - calibration.deadband);
        } else {
            axisPercent = 0.f;
        }
    }
    else {
        axisPercent = valueNormalized / axisLength;
    }

    float correctedValue = axisBasis * axisPercent;

    if (calibration.reversed) {
        correctedValue *= -1.0f;
    }

#if 0
    qCDebug(JoystickLog) << "_adjustRange corrected:value:min:max:center:reversed:deadband:basis:normalized:length"
                            << correctedValue
                            << value
                            << calibration.min
                            << calibration.max
                            << calibration.center
                            << calibration.reversed
                            << calibration.deadband
                            << axisBasis
                            << valueNormalized
                            << axisLength;
#endif

    return std::max(-1.0f, std::min(correctedValue, 1.0f));
}


void Joystick::run(void)
{
    _open();

    while (!_exitThread) {
    _update();

        // Update axes
        for (int axisIndex=0; axisIndex<_axisCount; axisIndex++) {
            int newAxisValue = _getAxis(axisIndex);
            // Calibration code requires signal to be emitted even if value hasn't changed
            _rgAxisValues[axisIndex] = newAxisValue;
            emit rawAxisValueChanged(axisIndex, newAxisValue);
        }

        // Update buttons
        for (int buttonIndex=0; buttonIndex<_buttonCount; buttonIndex++) {
            bool newButtonValue = _getButton(buttonIndex);
            if (newButtonValue != _rgButtonValues[buttonIndex]) {
                _rgButtonValues[buttonIndex] = newButtonValue;
                emit rawButtonPressedChanged(buttonIndex, newButtonValue);
            }
        }

        // Update hat - append hat buttons to the end of the normal button list
        int numHatButtons = 4;
        for (int hatIndex=0; hatIndex<_hatCount; hatIndex++) {
            for (int hatButtonIndex=0; hatButtonIndex<numHatButtons; hatButtonIndex++) {
                // Create new index value that includes the normal button list
                int rgButtonValueIndex = hatIndex*numHatButtons + hatButtonIndex + _buttonCount;
                // Get hat value from joystick
                bool newButtonValue = _getHat(hatIndex,hatButtonIndex);
                if (newButtonValue != _rgButtonValues[rgButtonValueIndex]) {
                    _rgButtonValues[rgButtonValueIndex] = newButtonValue;
                    emit rawButtonPressedChanged(rgButtonValueIndex, newButtonValue);
                }
            }
        }

        if (_activeVehicle->joystickEnabled() && !_calibrationMode && _calibrated) {
            int     axis = _rgFunctionAxis[rollFunction];
            float   roll = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis], _deadband);

                    axis = _rgFunctionAxis[pitchFunction];
            float   pitch = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis], _deadband);

                    axis = _rgFunctionAxis[yawFunction];
            float   yaw = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis],_deadband);

                    axis = _rgFunctionAxis[throttleFunction];
            float   throttle = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis], _throttleMode==ThrottleModeDownZero?false:_deadband);

            if ( _accumulator ) {
                static float throttle_accu = 0.f;

                throttle_accu += throttle*(40/1000.f); //for throttle to change from min to max it will take 1000ms (40ms is a loop time)

                throttle_accu = std::max(static_cast<float>(-1.f), std::min(throttle_accu, static_cast<float>(1.f)));
                throttle = throttle_accu;
            }

            if ( _circleCorrection ) {
                float roll_limited = std::max(static_cast<float>(-M_PI_4), std::min(roll, static_cast<float>(M_PI_4)));
                float pitch_limited = std::max(static_cast<float>(-M_PI_4), std::min(pitch, static_cast<float>(M_PI_4)));
                float yaw_limited = std::max(static_cast<float>(-M_PI_4), std::min(yaw, static_cast<float>(M_PI_4)));
                float throttle_limited = std::max(static_cast<float>(-M_PI_4), std::min(throttle, static_cast<float>(M_PI_4)));

                // Map from unit circle to linear range and limit
                roll =      std::max(-1.0f, std::min(tanf(asinf(roll_limited)), 1.0f));
                pitch =     std::max(-1.0f, std::min(tanf(asinf(pitch_limited)), 1.0f));
                yaw =       std::max(-1.0f, std::min(tanf(asinf(yaw_limited)), 1.0f));
                throttle =  std::max(-1.0f, std::min(tanf(asinf(throttle_limited)), 1.0f));
            }

            if ( _exponential != 0 ) {
                // Exponential (0% to -50% range like most RC radios)
                //_exponential is set by a slider in joystickConfig.qml

                // Calculate new RPY with exponential applied
                roll =      -_exponential*powf(roll,3) + (1+_exponential)*roll;
                pitch =     -_exponential*powf(pitch,3) + (1+_exponential)*pitch;
                yaw =       -_exponential*powf(yaw,3) + (1+_exponential)*yaw;
            }

            // Adjust throttle to 0:1 range
            if (_throttleMode == ThrottleModeCenterZero && _activeVehicle->supportsThrottleModeCenterZero()) {
                if (!_activeVehicle->supportsNegativeThrust() || !_negativeThrust) {
                    throttle = std::max(0.0f, throttle);
                }
            } else {
                throttle = (throttle + 1.0f) / 2.0f;
            }

            // Set up button pressed information

            quint16 newButtonBits = 0;      // New set of button which are down
            quint16 buttonPressedBits = 0;  // Buttons pressed for manualControl signal

            for (int buttonIndex=0; buttonIndex<_totalButtonCount; buttonIndex++) {
                quint16 buttonBit = 1 << buttonIndex;

                if (!_rgButtonValues[buttonIndex]) {
                    // Button up, just record it
                    newButtonBits |= buttonBit;
                } else {
                    if (_lastButtonBits & buttonBit) {
                        // Button was up last time through, but is now down which indicates a button press
                        qCDebug(JoystickLog) << "button triggered" << buttonIndex;

                        QString buttonAction =_rgButtonActions[buttonIndex];
                        if (!buttonAction.isEmpty()) {
                            _buttonAction(buttonAction);
                        }
                    }

                    // Mark the button as pressed as long as its pressed
                    buttonPressedBits |= buttonBit;
                }
            }

            _lastButtonBits = newButtonBits;

            qCDebug(JoystickValuesLog) << "name:roll:pitch:yaw:throttle" << name() << roll << -pitch << yaw << throttle;

            // NOTE: The buttonPressedBits going to MANUAL_CONTROL are currently used by ArduSub.
            emit manualControl(roll, -pitch, yaw, throttle, buttonPressedBits, _activeVehicle->joystickMode());
        }

        // Sleep. Update rate of joystick is by default 25 Hz
        int mswait = (int)(1000.0f / _frequency);
        QGC::SLEEP::msleep(mswait);
    }

    _close();
}

void Joystick::startPolling(Vehicle* vehicle)
{
    if (vehicle) {

        // If a vehicle is connected, disconnect it
        if (_activeVehicle) {
            UAS* uas = _activeVehicle->uas();
            disconnect(this, &Joystick::manualControl, uas, &UAS::setExternalControlSetpoint);
        }

        // Always set up the new vehicle
        _activeVehicle = vehicle;

        // If joystick is not calibrated, disable it
        if ( !_calibrated ) {
            vehicle->setJoystickEnabled(false);
        }

        // Update qml in case of joystick transition
        emit calibratedChanged(_calibrated);

        // Only connect the new vehicle if it wants joystick data
        if (vehicle->joystickEnabled()) {
            _pollingStartedForCalibration = false;

            UAS* uas = _activeVehicle->uas();
            connect(this, &Joystick::manualControl, uas, &UAS::setExternalControlSetpoint);
            // FIXME: ****
            //connect(this, &Joystick::buttonActionTriggered, uas, &UAS::triggerAction);
        }
    }


    if (!isRunning()) {
        _exitThread = false;
        start();
    }
}

void Joystick::stopPolling(void)
{
    if (isRunning()) {

        if (_activeVehicle && _activeVehicle->joystickEnabled()) {
            UAS* uas = _activeVehicle->uas();
            // Neutral attitude controls
            // emit manualControl(0, 0, 0, 0.5, 0, _activeVehicle->joystickMode());
            disconnect(this, &Joystick::manualControl,          uas, &UAS::setExternalControlSetpoint);
        }
        // FIXME: ****
        //disconnect(this, &Joystick::buttonActionTriggered,  uas, &UAS::triggerAction);

        _exitThread = true;
    }
}

void Joystick::setCalibration(int axis, Calibration_t& calibration)
{
    if (!_validAxis(axis)) {
        qCWarning(JoystickLog) << "Invalid axis index" << axis;
        return;
    }

    _calibrated = true;
    _rgCalibration[axis] = calibration;
    _saveSettings();
    emit calibratedChanged(_calibrated);
}

Joystick::Calibration_t Joystick::getCalibration(int axis)
{
    if (!_validAxis(axis)) {
        qCWarning(JoystickLog) << "Invalid axis index" << axis;
    }

    return _rgCalibration[axis];
}

void Joystick::setFunctionAxis(AxisFunction_t function, int axis)
{
    if (!_validAxis(axis)) {
        qCWarning(JoystickLog) << "Invalid axis index" << axis;
        return;
    }

    _calibrated = true;
    _rgFunctionAxis[function] = axis;

    _saveSettings();
    emit calibratedChanged(_calibrated);
}

int Joystick::getFunctionAxis(AxisFunction_t function)
{
    if (function < 0 || function >= maxFunction) {
        qCWarning(JoystickLog) << "Invalid function" << function;
    }

    return _rgFunctionAxis[function];
}

QStringList Joystick::actions(void)
{
    QStringList list;
    list << _buttonActionArm << _buttonActionDisarm;
    if (_activeVehicle) {
        list << _activeVehicle->flightModes();
    }
    list << _buttonActionVTOLFixedWing << _buttonActionVTOLMultiRotor;
    list << _buttonActionZoomIn << _buttonActionZoomOut;
    list << _buttonActionNextStream << _buttonActionPreviousStream;
    list << _buttonActionNextCamera << _buttonActionPreviousCamera;
    return list;
}

void Joystick::setButtonAction(int button, const QString& action)
{
    if (!_validButton(button)) {
        qCWarning(JoystickLog) << "Invalid button index" << button;
        return;
    }

    qDebug() << "setButtonAction" << action;

    _rgButtonActions[button] = action;
    _saveSettings();
    emit buttonActionsChanged(buttonActions());
}

QString Joystick::getButtonAction(int button)
{
    if (!_validButton(button)) {
        qCWarning(JoystickLog) << "Invalid button index" << button;
    }

    return _rgButtonActions[button];
}

QVariantList Joystick::buttonActions(void)
{
    QVariantList list;

    for (int button=0; button<_totalButtonCount; button++) {
        list += QVariant::fromValue(_rgButtonActions[button]);
    }

    return list;
}

int Joystick::throttleMode(void)
{
    return _throttleMode;
}

void Joystick::setThrottleMode(int mode)
{
    if (mode < 0 || mode >= ThrottleModeMax) {
        qCWarning(JoystickLog) << "Invalid throttle mode" << mode;
        return;
    }

    _throttleMode = (ThrottleMode_t)mode;

    if (_throttleMode == ThrottleModeDownZero) {
        setAccumulator(false);
    }

    _saveSettings();
    emit throttleModeChanged(_throttleMode);
}

bool Joystick::negativeThrust(void)
{
    return _negativeThrust;
}

void Joystick::setNegativeThrust(bool allowNegative)
{
    if (_negativeThrust == allowNegative) {
        return;
    }
    _negativeThrust = allowNegative;

    _saveSettings();
    emit negativeThrustChanged(_negativeThrust);
}

float Joystick::exponential(void)
{
    return _exponential;
}

void Joystick::setExponential(float expo)
{
    _exponential = expo;

    _saveSettings();
    emit exponentialChanged(_exponential);
}

bool Joystick::accumulator(void)
{
    return _accumulator;
}

void Joystick::setAccumulator(bool accu)
{
    _accumulator = accu;

    _saveSettings();
    emit accumulatorChanged(_accumulator);
}

bool Joystick::deadband(void)
{
    return _deadband;
}

void Joystick::setDeadband(bool deadband)
{
    _deadband = deadband;

    _saveSettings();
}

bool Joystick::circleCorrection(void)
{
    return _circleCorrection;
}

void Joystick::setCircleCorrection(bool circleCorrection)
{
    _circleCorrection = circleCorrection;

    _saveSettings();
    emit circleCorrectionChanged(_circleCorrection);
}

float Joystick::frequency()
{
    return _frequency;
}

void Joystick::setFrequency(float val)
{
    //-- Arbitrary limits
    if(val < 0.25f)  val = 0.25f;
    if(val > 100.0f) val = 100.0f;
    _frequency = val;
    _saveSettings();
    emit frequencyChanged();
}

void Joystick::setCalibrationMode(bool calibrating)
{
    _calibrationMode = calibrating;

    if (calibrating && !isRunning()) {
        _pollingStartedForCalibration = true;
        startPolling(_multiVehicleManager->activeVehicle());
    }
    else if (_pollingStartedForCalibration) {
        stopPolling();
    }
}


void Joystick::_buttonAction(const QString& action)
{
    if (!_activeVehicle || !_activeVehicle->joystickEnabled()) {
        return;
    }

    if (action == _buttonActionArm) {
        _activeVehicle->setArmed(true);
    } else if (action == _buttonActionDisarm) {
        _activeVehicle->setArmed(false);
    } else if (action == _buttonActionVTOLFixedWing) {
        _activeVehicle->setVtolInFwdFlight(true);
    } else if (action == _buttonActionVTOLMultiRotor) {
        _activeVehicle->setVtolInFwdFlight(false);
    } else if (_activeVehicle->flightModes().contains(action)) {
        _activeVehicle->setFlightMode(action);
    } else if(action == _buttonActionZoomIn || action == _buttonActionZoomOut) {
        emit stepZoom(action == _buttonActionZoomIn ? 1 : -1);
    } else if(action == _buttonActionNextStream || action == _buttonActionPreviousStream) {
        emit stepStream(action == _buttonActionNextStream ? 1 : -1);
    } else if(action == _buttonActionNextCamera || action == _buttonActionPreviousCamera) {
        emit stepCamera(action == _buttonActionNextCamera ? 1 : -1);
    } else {
        qCDebug(JoystickLog) << "_buttonAction unknown action:" << action;
    }
}

bool Joystick::_validAxis(int axis)
{
    return axis >= 0 && axis < _axisCount;
}

bool Joystick::_validButton(int button)
{
    return button >= 0 && button < _totalButtonCount;
}

