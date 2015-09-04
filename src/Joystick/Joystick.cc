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

#include "Joystick.h"
#include "QGC.h"
#include "MultiVehicleManager.h"
#include "AutoPilotPlugin.h"
#include "UAS.h"

#include <QSettings>

#ifndef __mobile__
    #ifdef Q_OS_MAC
        #include <SDL.h>
    #else
        #include <SDL/SDL.h>
    #endif
#endif

QGC_LOGGING_CATEGORY(JoystickLog, "JoystickLog")
QGC_LOGGING_CATEGORY(JoystickValuesLog, "JoystickValuesLog")

const char* Joystick::_settingsGroup =              "Joysticks";
const char* Joystick::_calibratedSettingsKey =      "Calibrated";
const char* Joystick::_buttonActionSettingsKey =    "ButtonAction%1";
const char* Joystick::_throttleModeSettingsKey =    "ThrottleMode";

const char* Joystick::_rgFunctionSettingsKey[Joystick::maxFunction] = {
    "RollAxis",
    "PitchAxis",
    "YawAxis",
    "ThrottleAxis"
};

Joystick::Joystick(const QString& name, int axisCount, int buttonCount, int sdlIndex)
#ifndef __mobile__
    : _sdlIndex(sdlIndex)
    , _exitThread(false)
    , _name(name)
    , _axisCount(axisCount)
    , _buttonCount(buttonCount)
    , _calibrationMode(CalibrationModeOff)
    , _lastButtonBits(0)
    , _throttleMode(ThrottleModeCenterZero)
    , _activeVehicle(NULL)
    , _pollingStartedForCalibration(false)
#endif // __mobile__
{
#ifdef __mobile__
    Q_UNUSED(name)
    Q_UNUSED(axisCount)
    Q_UNUSED(buttonCount)
    Q_UNUSED(sdlIndex)
#else
    for (int i=0; i<_cAxes; i++) {
        _rgAxisValues[i] = 0;
    }
    for (int i=0; i<_cButtons; i++) {
        _rgButtonValues[i] = false;
        _rgButtonActions[i] = -1;
    }
    
    _loadSettings();
#endif // __mobile __
}

Joystick::~Joystick()
{
    
}

#ifndef __mobile__

void Joystick::_loadSettings(void)
{
    QSettings   settings;
    
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    
    bool badSettings = false;
    bool convertOk;
    
    qCDebug(JoystickLog) << "_loadSettings " << _name;
    
    _calibrated = settings.value(_calibratedSettingsKey, false).toBool();
    
    _throttleMode = (ThrottleMode_t)settings.value(_throttleModeSettingsKey, ThrottleModeCenterZero).toInt(&convertOk);
    badSettings |= !convertOk;
    
    qCDebug(JoystickLog) << "_loadSettings calibrated:throttlemode:badsettings" << _calibrated << _throttleMode << badSettings;
    
    QString minTpl  ("Axis%1Min");
    QString maxTpl  ("Axis%1Max");
    QString trimTpl ("Axis%1Trim");
    QString revTpl  ("Axis%1Rev");
    
    for (int axis=0; axis<_cAxes; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];
        
        calibration->center = settings.value(trimTpl.arg(axis), 0).toInt(&convertOk);
        badSettings |= !convertOk;
        
        calibration->min = settings.value(minTpl.arg(axis), -32768).toInt(&convertOk);
        badSettings |= !convertOk;
        
        calibration->max = settings.value(maxTpl.arg(axis), 32768).toInt(&convertOk);
        badSettings |= !convertOk;
        
        calibration->reversed = settings.value(revTpl.arg(axis), false).toBool();
        
        qCDebug(JoystickLog) << "_loadSettings axis:min:max:trim:reversed:badsettings" << axis << calibration->min << calibration->max << calibration->center << calibration->reversed << badSettings;
    }
    
    for (int function=0; function<maxFunction; function++) {
        int functionAxis;
        
        functionAxis = settings.value(_rgFunctionSettingsKey[function], -1).toInt(&convertOk);
        badSettings |= !convertOk || (functionAxis == -1);
        
        _rgFunctionAxis[function] = functionAxis;
        
        qCDebug(JoystickLog) << "_loadSettings function:axis:badsettings" << function << functionAxis << badSettings;
    }
    
    for (int button=0; button<_cButtons; button++) {
        _rgButtonActions[button] = settings.value(QString(_buttonActionSettingsKey).arg(button), -1).toInt(&convertOk);
        badSettings |= !convertOk;
        
        qCDebug(JoystickLog) << "_loadSettings button:action:badsettings" << button << _rgButtonActions[button] << badSettings;
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
    settings.beginGroup(_name);
    
    settings.setValue(_calibratedSettingsKey, _calibrated);
    settings.setValue(_throttleModeSettingsKey, _throttleMode);
    
    qCDebug(JoystickLog) << "_saveSettings calibrated:throttlemode" << _calibrated << _throttleMode;

    QString minTpl  ("Axis%1Min");
    QString maxTpl  ("Axis%1Max");
    QString trimTpl ("Axis%1Trim");
    QString revTpl  ("Axis%1Rev");
    
    for (int axis=0; axis<_cAxes; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];
        
        settings.setValue(trimTpl.arg(axis), calibration->center);
        settings.setValue(minTpl.arg(axis), calibration->min);
        settings.setValue(maxTpl.arg(axis), calibration->max);
        settings.setValue(revTpl.arg(axis), calibration->reversed);
        
        qCDebug(JoystickLog) << "_saveSettings name:axis:min:max:trim:reversed"
                                << _name
                                << axis
                                << calibration->min
                                << calibration->max
                                << calibration->center
                                << calibration->reversed;
    }
    
    for (int function=0; function<maxFunction; function++) {
        settings.setValue(_rgFunctionSettingsKey[function], _rgFunctionAxis[function]);
        qCDebug(JoystickLog) << "_saveSettings name:function:axis" << _name << function << _rgFunctionSettingsKey[function];
    }
    
    for (int button=0; button<_cButtons; button++) {
        settings.setValue(QString(_buttonActionSettingsKey).arg(button), _rgButtonActions[button]);
        qCDebug(JoystickLog) << "_saveSettings button:action" << button << _rgButtonActions[button];
    }
}

/// Adjust the raw axis value to the -1:1 range given calibration information
float Joystick::_adjustRange(int value, Calibration_t calibration)
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
    
    float axisPercent = valueNormalized / axisLength;
    
    float correctedValue = axisBasis * axisPercent;
    
    if (calibration.reversed) {
        correctedValue *= -1.0f;
    }
    
#if 0
    qCDebug(JoystickLog) << "_adjustRange corrected:value:min:max:center:reversed:basis:normalized:length"
                            << correctedValue
                            << value
                            << calibration.min
                            << calibration.max
                            << calibration.center
                            << calibration.center
                            << axisBasis
                            << valueNormalized
                            << axisLength;
#endif

    return correctedValue;
}


void Joystick::run(void)
{
    SDL_Joystick* sdlJoystick = SDL_JoystickOpen(_sdlIndex);
    
    if (!sdlJoystick) {
        qCWarning(JoystickLog) << "SDL_JoystickOpen failed:" << SDL_GetError();
        return;
    }
    
    while (!_exitThread) {
        SDL_JoystickUpdate();

        // Update axes
        for (int axisIndex=0; axisIndex<_axisCount; axisIndex++) {
            int newAxisValue = SDL_JoystickGetAxis(sdlJoystick, axisIndex);
            // Calibration code requires signal to be emitted even if value hasn't changed
            _rgAxisValues[axisIndex] = newAxisValue;
            emit rawAxisValueChanged(axisIndex, newAxisValue);
        }
        
        // Update buttons
        for (int buttonIndex=0; buttonIndex<_buttonCount; buttonIndex++) {
            bool newButtonValue = !!SDL_JoystickGetButton(sdlJoystick, buttonIndex);
            if (newButtonValue != _rgButtonValues[buttonIndex]) {
                _rgButtonValues[buttonIndex] = newButtonValue;
                emit rawButtonPressedChanged(buttonIndex, newButtonValue);
            }
        }
        
        if (_calibrationMode != CalibrationModeCalibrating) {
            int     axis = _rgFunctionAxis[rollFunction];
            float   roll = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis]);
            
                    axis = _rgFunctionAxis[pitchFunction];
            float   pitch = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis]);
            
                    axis = _rgFunctionAxis[yawFunction];
            float   yaw = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis]);
            
                    axis = _rgFunctionAxis[throttleFunction];
            float   throttle = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis]);

            // Map from unit circle to linear range and limit
            roll =      std::max(-1.0f, std::min(tanf(asinf(roll)), 1.0f));
            pitch =     std::max(-1.0f, std::min(tanf(asinf(pitch)), 1.0f));
            yaw =       std::max(-1.0f, std::min(tanf(asinf(yaw)), 1.0f));
            throttle =  std::max(-1.0f, std::min(tanf(asinf(throttle)), 1.0f));
            
            // Adjust throttle to 0:1 range
            if (_throttleMode == ThrottleModeCenterZero) {
                throttle =  std::max(0.0f, throttle);
            } else {                
                throttle = (throttle + 1.0f) / 2.0f;
            }
            
            // Set up button pressed information
            
            // We only send the buttons the firmwware has reserved
            int reservedButtonCount = _activeVehicle->manualControlReservedButtonCount();
            if (reservedButtonCount == -1) {
                reservedButtonCount = _buttonCount;
            }
            
            quint16 newButtonBits = 0;      // New set of button which are down
            quint16 buttonPressedBits = 0;  // Buttons pressed for manualControl signal
            
            for (int buttonIndex=0; buttonIndex<_buttonCount; buttonIndex++) {
                quint16 buttonBit = 1 << buttonIndex;
                
                if (_rgButtonValues[buttonIndex]) {
                    // Button pressed down, just record it
                    newButtonBits |= buttonBit;
                } else {
                    if (_lastButtonBits & buttonBit) {
                        // Button was down last time through, but is now up which indicates a button press
                        qCDebug(JoystickLog) << "button triggered" << buttonIndex;
                        
                        
                        if (buttonIndex >= reservedButtonCount) {
                            // Button is above firmware reserved set
                            int buttonAction =_rgButtonActions[buttonIndex];
                            if (buttonAction != -1) {
                                qCDebug(JoystickLog) << "buttonActionTriggered" << buttonAction;
                                emit buttonActionTriggered(buttonAction);
                            }
                        } else {
                            // Button is within firmware reserved set
                            // Record the button press for manualControl signal
                            buttonPressedBits |= buttonBit;
                            qCDebug(JoystickLog) << "button press recorded for manualControl" << buttonIndex;
                        }
                    }
                }
            }
            
            _lastButtonBits = newButtonBits;
            
            qCDebug(JoystickValuesLog) << "roll:pitch:yaw:throttle" << roll << -pitch << yaw << throttle;
            
            emit manualControl(roll, -pitch, yaw, throttle, buttonPressedBits, _activeVehicle->joystickMode());
        }
        
        // Sleep, update rate of joystick is approx. 25 Hz (1000 ms / 25 = 40 ms)
        QGC::SLEEP::msleep(40);
    }
    
    SDL_JoystickClose(sdlJoystick);
}

void Joystick::startPolling(Vehicle* vehicle)
{
    if (isRunning()) {
        if (vehicle != _activeVehicle) {
            // Joystick was previously disabled, but now enabled from config screen
            
            if (_calibrationMode == CalibrationModeOff) {
                qWarning() << "Incorrect usage pattern";
                return;
            }
            
            _activeVehicle = vehicle;
            _pollingStartedForCalibration = false;
        }
    } else {
        _activeVehicle = vehicle;
        
        UAS* uas = _activeVehicle->uas();
        
        connect(this, &Joystick::manualControl,         uas, &UAS::setExternalControlSetpoint);
        connect(this, &Joystick::buttonActionTriggered, uas, &UAS::triggerAction);
        
        _exitThread = false;
        start();
    }
}

void Joystick::stopPolling(void)
{
    if (isRunning()) {
        UAS* uas = _activeVehicle->uas();
        
        disconnect(this, &Joystick::manualControl,          uas, &UAS::setExternalControlSetpoint);
        disconnect(this, &Joystick::buttonActionTriggered,  uas, &UAS::triggerAction);
        
        _exitThread = true;
        }
}

void Joystick::setCalibration(int axis, Calibration_t& calibration)
{
    if (axis < 0 || axis > _cAxes) {
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
    if (axis < 0 || axis > _cAxes) {
        qCWarning(JoystickLog) << "Invalid axis index" << axis;
    }
    
    return _rgCalibration[axis];
}

void Joystick::setFunctionAxis(AxisFunction_t function, int axis)
{
    if (axis < 0 || axis > _cAxes) {
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
    
    foreach(QAction* action, MultiVehicleManager::instance()->activeVehicle()->uas()->getActions()) {
        list += action->text();
    }
    
    return list;
}

void Joystick::setButtonAction(int button, int action)
{
    if (button < 0 || button > _cButtons) {
        qCWarning(JoystickLog) << "Invalid button index" << button;
        return;
    }
    
    _rgButtonActions[button] = action;
    _saveSettings();
    emit buttonActionsChanged(buttonActions());
}

int Joystick::getButtonAction(int button)
{
    if (button < 0 || button > _cButtons) {
        qCWarning(JoystickLog) << "Invalid button index" << button;
    }
    
    return _rgButtonActions[button];
}

QVariantList Joystick::buttonActions(void)
{
    QVariantList list;
    
    for (int button=0; button<_buttonCount; button++) {
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
    _saveSettings();
    emit throttleModeChanged(_throttleMode);
}

void Joystick::startCalibrationMode(CalibrationMode_t mode)
{
    if (mode == CalibrationModeOff) {
        qWarning() << "Incorrect mode CalibrationModeOff";
        return;
    }
    
    _calibrationMode = mode;
    
    if (!isRunning()) {
        _pollingStartedForCalibration = true;
        startPolling(MultiVehicleManager::instance()->activeVehicle());
    }
}

void Joystick::stopCalibrationMode(CalibrationMode_t mode)
{
    if (mode == CalibrationModeOff) {
        qWarning() << "Incorrect mode: CalibrationModeOff";
        return;
    } else if (mode != _calibrationMode) {
        qWarning() << "Incorrect mode sequence request:active" << mode << _calibrationMode;
        return;
    }
    
    if (mode == CalibrationModeCalibrating) {
        _calibrationMode = CalibrationModeMonitor;
    } else {
        _calibrationMode = CalibrationModeOff;
        if (_pollingStartedForCalibration) {
            stopPolling();
        }
    }
}

#endif // __mobile__
