/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "JoystickConfigController.h"
#include "JoystickManager.h"
#include "QGCApplication.h"

#include <QSettings>

QGC_LOGGING_CATEGORY(JoystickConfigControllerLog, "JoystickConfigControllerLog")

const int JoystickConfigController::_updateInterval =       150;        ///< Interval for timer which updates radio channel widgets
const int JoystickConfigController::_calCenterPoint =       0;
const int JoystickConfigController::_calValidMinValue =     -32768;     ///< Largest valid minimum axis value
const int JoystickConfigController::_calValidMaxValue =     32767;      ///< Smallest valid maximum axis value
const int JoystickConfigController::_calDefaultMinValue =   -32768;     ///< Default value for Min if not set
const int JoystickConfigController::_calDefaultMaxValue =   32767;      ///< Default value for Max if not set
const int JoystickConfigController::_calRoughCenterDelta =  500;        ///< Delta around center point which is considered to be roughly centered
const int JoystickConfigController::_calMoveDelta =         32768/2;    ///< Amount of delta past center which is considered stick movement
const int JoystickConfigController::_calSettleDelta =       600;        ///< Amount of delta which is considered no stick movement
const int JoystickConfigController::_calMinDelta =          1000;       ///< Amount of delta allowed around min value to consider channel at min

const int JoystickConfigController::_stickDetectSettleMSecs = 500;

const char*  JoystickConfigController::_imageFilePrefix =       "calibration/";
const char*  JoystickConfigController::_imageFileMode2Dir =     "joystick/";
const char*  JoystickConfigController::_imageCenter =           "joystickCenter.png";
const char*  JoystickConfigController::_imageLeftStickUp =      "joystickThrottleUp.png";
const char*  JoystickConfigController::_imageLeftStickDown =    "joystickThrottleDown.png";
const char*  JoystickConfigController::_imageLeftStickLeft =    "joystickYawLeft.png";
const char*  JoystickConfigController::_imageLeftStickRight =   "joystickYawRight.png";
const char*  JoystickConfigController::_imageRightStickLeft =   "joystickRollLeft.png";
const char*  JoystickConfigController::_imageRightStickRight =  "joystickRollRight.png";
const char*  JoystickConfigController::_imageRightStickUp =     "joystickPitchUp.png";
const char*  JoystickConfigController::_imageRightStickDown =   "joystickPitchDown.png";

JoystickConfigController::JoystickConfigController(void)
    : _activeJoystick(NULL)
    , _currentStep(-1)
    , _axisCount(0)
    , _rgAxisInfo(NULL)
    , _axisValueSave(NULL)
    , _axisRawValue(NULL)
    , _calState(calStateAxisWait)
    , _statusText(NULL)
    , _cancelButton(NULL)
    , _nextButton(NULL)
    , _skipButton(NULL)
    , _joystickManager(qgcApp()->toolbox()->joystickManager())
{
    
    connect(_joystickManager, &JoystickManager::activeJoystickChanged, this, &JoystickConfigController::_activeJoystickChanged);
    
    _activeJoystickChanged(_joystickManager->activeJoystick());
    _resetInternalCalibrationValues();
}

void JoystickConfigController::start(void)
{
    _stopCalibration();
}

JoystickConfigController::~JoystickConfigController()
{
    _activeJoystick->stopCalibrationMode(Joystick::CalibrationModeMonitor);
}

/// @brief Returns the state machine entry for the specified state.
const JoystickConfigController::stateMachineEntry* JoystickConfigController::_getStateMachineEntry(int step)
{
    static const char* msgBegin =         "Allow all sticks to center as shown in diagram.\n\nClick Next to continue";
    static const char* msgLStickUp =      "Move the LEFT stick all the way up and hold it there...";
    static const char* msgLStickDown =    "Move the LEFT stick all the way down and hold it there...";
    static const char* msgLStickLeft =    "Move the LEFT stick all the way to the left and hold it there...";
    static const char* msgLStickRight =   "Move the LEFT stick all the way to the right and hold it there...";
    static const char* msgRStickLeft =    "Move the RIGHT stick all the way to the left and hold it there...";
    static const char* msgRStickRight =   "Move the RIGHT stick all the way to the right and hold it there...";
    static const char* msgRStickDown =    "Move the RIGHT stick all the way down and hold it there...";
    static const char* msgRStickUp =      "Move the RIGHT stick all the way up and hold it there...";
    static const char* msgRStickCenter =  "Allow the RIGHT stick to move back to center...";
    static const char* msgComplete =      "All settings have been captured. Click Next to enable the joystick.";
    
    static const stateMachineEntry rgStateMachine[] = {
        //Axis                   Message           Image                    RCInputFunction                                     NextFunction                                    SkipFunction
        { Joystick::maxAxis,     msgBegin,         _imageCenter,            &JoystickConfigController::_inputCenterWaitBegin,   &JoystickConfigController::_saveAllTrims,       NULL },
        { Joystick::stickLeftY,  msgLStickUp,      _imageLeftStickUp,       &JoystickConfigController::_inputStickDetect,       NULL,                                           NULL },
        { Joystick::stickLeftY,  msgLStickDown,    _imageLeftStickDown,     &JoystickConfigController::_inputStickMin,          NULL,                                           NULL },
        { Joystick::stickLeftX,  msgLStickRight,   _imageLeftStickRight,    &JoystickConfigController::_inputStickDetect,       NULL,                                           NULL },
        { Joystick::stickLeftX,  msgLStickLeft,    _imageLeftStickLeft,     &JoystickConfigController::_inputStickMin,          NULL,                                           NULL },
        { Joystick::stickRightX, msgRStickRight,   _imageRightStickRight,   &JoystickConfigController::_inputStickDetect,       NULL,                                           NULL },
        { Joystick::stickRightX, msgRStickLeft,    _imageRightStickLeft,    &JoystickConfigController::_inputStickMin,          NULL,                                           NULL },
        { Joystick::stickRightY, msgRStickUp,      _imageRightStickUp,      &JoystickConfigController::_inputStickDetect,       NULL,                                           NULL },
        { Joystick::stickRightY, msgRStickDown,    _imageRightStickDown,    &JoystickConfigController::_inputStickMin,          NULL,                                           NULL },
        { Joystick::stickRightY, msgRStickCenter,  _imageCenter,            &JoystickConfigController::_inputCenterWait,        NULL,                                           NULL },
        { Joystick::maxAxis,     msgComplete,      _imageCenter,            NULL,                                               &JoystickConfigController::_writeCalibration,   NULL },
    };
    
    Q_ASSERT(step >=0 && step < (int)(sizeof(rgStateMachine) / sizeof(rgStateMachine[0])));
    
    return &rgStateMachine[step];
}

void JoystickConfigController::_advanceState(void)
{
    _currentStep++;
    _setupCurrentState();
}


/// @brief Sets up the state machine according to the current step from _currentStep.
void JoystickConfigController::_setupCurrentState(void)
{
   const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
    
    _statusText->setProperty("text", state->instructions);
    
    _setHelpImage(state->image);
    
    _stickDetectAxis = Joystick::maxAxis;
    _stickDetectSettleStarted = false;
    
    _calSaveCurrentValues();
    
    _nextButton->setEnabled(state->nextFn != NULL);
    _skipButton->setEnabled(state->skipFn != NULL);
}

void JoystickConfigController::_axisValueChanged(int axis, int value)
{
    if (_validAxis(axis)) {
        // We always update raw values
        _axisRawValue[axis] = value;
        emit axisValueChanged(axis, _axisRawValue[axis]);
        
        // Signal attitude axis values to Qml if mapped
        if (_rgAxisInfo[axis].axis != Joystick::maxAxis) {
            switch (_rgAxisInfo[axis].function) {
                case Joystick::rollFunction:
                    emit rollAxisValueChanged(_axisRawValue[axis]);
                    break;
                case Joystick::pitchFunction:
                    emit pitchAxisValueChanged(_axisRawValue[axis]);
                    break;
                case Joystick::yawFunction:
                    emit yawAxisValueChanged(_axisRawValue[axis]);
                    break;
                case Joystick::throttleFunction:
                    emit throttleAxisValueChanged(_axisRawValue[axis]);
                    break;
                default:
                    break;
            }
        }
            
        //qCDebug(JoystickConfigControllerLog) << "Raw value" << axis << value;
        
        if (_currentStep == -1) {
            // Track the axis count by keeping track of how many axes we see
            if (axis + 1 > (int)_axisCount) {
                _axisCount = axis + 1;
            }
        }
        
        if (_currentStep != -1) {
            const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
            Q_ASSERT(state);
            if (state->rcInputFn) {
                (this->*state->rcInputFn)(state->axis, axis, value);
            }
        }
    }
}

void JoystickConfigController::nextButtonClicked(void)
{
    if (_currentStep == -1) {
        // Need to have enough channels
        if (_axisCount < _axisMinimum) {
            qgcApp()->showMessage(QString("Detected %1 joystick axes. To operate PX4, you need at least %2 axes.").arg(_axisCount).arg(_axisMinimum));
            return;
        }
        _startCalibration();
    } else {
        const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
        Q_ASSERT(state);
        Q_ASSERT(state->nextFn);
        (this->*state->nextFn)();
    }
}

void JoystickConfigController::skipButtonClicked(void)
{
    Q_ASSERT(_currentStep != -1);
    
    const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
    Q_ASSERT(state);
    Q_ASSERT(state->skipFn);
    (this->*state->skipFn)();
}

void JoystickConfigController::cancelButtonClicked(void)
{
    _stopCalibration();
}

bool JoystickConfigController::getDeadbandToggle() {
    return _activeJoystick->deadband();
}

void JoystickConfigController::setDeadbandToggle(bool deadband) {
    _activeJoystick->setDeadband(deadband);

    _signalAllAttiudeValueChanges();

    emit deadbandToggled(deadband);
}

void JoystickConfigController::_saveAllTrims(void)
{
    // We save all trims as the first step. At this point no axes are mapped but it should still
    // allow us to get good trims for the roll/pitch/yaw/throttle even though we don't know which
    // axis they are yet. As we continue through the process the other axes will get their
    // trims reset to correct values.
    
    for (int i=0; i<_axisCount; i++) {
        qCDebug(JoystickConfigControllerLog) << "_saveAllTrims trim" << _axisRawValue[i];
        _rgAxisInfo[i].axisTrim = _axisRawValue[i];
    }
    _advanceState();
}

void JoystickConfigController::_axisDeadbandChanged(int axis, int value)
{
    value = abs(value)<_calValidMaxValue?abs(value):_calValidMaxValue;

    _rgAxisInfo[axis].deadband = value;

    qCDebug(JoystickConfigControllerLog) << "Axis:" << axis << "Deadband:" << _rgAxisInfo[axis].deadband;
}

/// @brief Waits for the sticks to be centered, enabling Next when done.
void JoystickConfigController::_inputCenterWaitBegin(Joystick::Axis_t axis, int mappedAxis, int value)
{
    Q_UNUSED(axis);

    //sensing deadband
    if (abs(value)*1.1f>_rgAxisInfo[mappedAxis].deadband) {   //add 10% on top of existing deadband
        _axisDeadbandChanged(mappedAxis,abs(value)*1.1f);
    }

    _nextButton->setEnabled(true);

    // FIXME: Doesn't wait for center
}

bool JoystickConfigController::_stickSettleComplete(int axis, int value)
{
    if (!_validAxis(axis)) {
        qCWarning(JoystickConfigControllerLog) << "Invalid axis axis:_axisCount" << axis << _axisCount;
        return false;
    }

    // We are waiting for the stick to settle out to a max position
    
    if (abs(_stickDetectValue - value) > _calSettleDelta) {
        // Stick is moving too much to consider stopped
        
        qCDebug(JoystickConfigControllerLog) << "_stickSettleComplete still moving, axis:_stickDetectValue:value" << axis << _stickDetectValue << value;

        _stickDetectValue = value;
        _stickDetectSettleStarted = false;
    } else {
        // Stick is still positioned within the specified small range
        
        if (_stickDetectSettleStarted) {
            // We have already started waiting
            
            if (_stickDetectSettleElapsed.elapsed() > _stickDetectSettleMSecs) {
                // Stick has stayed positioned in one place long enough, detection is complete.
                qCDebug(JoystickConfigControllerLog) << "_stickSettleComplete detection complete, axis:_stickDetectValue:value" << axis << _stickDetectValue << value;
                return true;
            }
        } else {
            // Start waiting for the stick to stay settled for _stickDetectSettleWaitMSecs msecs
            
            qCDebug(JoystickConfigControllerLog) << "_stickSettleComplete starting settle timer, axis:_stickDetectValue:value" << axis << _stickDetectValue << value;
            
            _stickDetectSettleStarted = true;
            _stickDetectSettleElapsed.start();
        }
    }
    
    return false;
}

void JoystickConfigController::_inputStickDetect(Joystick::Axis_t axis, int mappedAxis, int value)
{
	qCDebug(JoystickConfigControllerLog) << "_inputStickDetect axis:mappedAxis:value" << axis << mappedAxis << value;
	
    if (!_validAxis(mappedAxis)) {
        qCWarning(JoystickConfigControllerLog) << "Invalid axis mappedAxis:_axisCount" << mappedAxis << _axisCount;
        return;
    }

    // If this axis is already used in a mapping we can't use it again
    if (_rgAxisInfo[mappedAxis].axis != Joystick::maxAxis) {
        return;
    }
    
    if (_stickDetectAxis == Joystick::maxAxis) {
        // We have not detected enough movement on a axis yet
        
        if (abs(_axisValueSave[mappedAxis] - value) > _calMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function
            
            qCDebug(JoystickConfigControllerLog) << "_inputStickDetect starting settle wait, axis:mappedAxis:value" << axis << mappedAxis << value;
            
            // Setup up to detect stick being pegged to min or max value
            _stickDetectAxis = mappedAxis;
            _stickDetectInitialValue = value;
            _stickDetectValue = value;
        }
    } else if (mappedAxis == _stickDetectAxis) {
        if (_stickSettleComplete(mappedAxis, value)) {
            AxisInfo* info = &_rgAxisInfo[mappedAxis];
            
            // Stick detection is complete. Stick should be at max position.
            for(int i = 0; i < Joystick::maxFunction; i++) {
                if(_rgFunctionAxisMapping[i] == axis)
                    info->function = (Joystick::AxisFunction_t)i;
            }

            info->axis = axis;
            _rgAxisMapping[axis] = mappedAxis;

            qDebug(JoystickConfigControllerLog) << "stickLeftX is mapped to raw axis:" << _rgAxisMapping[Joystick::stickLeftX];
            qDebug(JoystickConfigControllerLog) << "stickLeftY is mapped to raw axis:" << _rgAxisMapping[Joystick::stickLeftY];
            qDebug(JoystickConfigControllerLog) << "stickRightX is mapped to raw axis:" << _rgAxisMapping[Joystick::stickRightX];
            qDebug(JoystickConfigControllerLog) << "stickRightY is mapped to raw axis:" << _rgAxisMapping[Joystick::stickRightY];

            // Axis should be at max value, if it is below initial set point the the axis is reversed.
            info->reversed = value < _axisValueSave[mappedAxis];
            
            if (info->reversed) {
                _rgAxisInfo[mappedAxis].axisMin = value;
            } else {
                _rgAxisInfo[mappedAxis].axisMax = value;
            }
            
            qCDebug(JoystickConfigControllerLog) << "_inputStickDetect saving values, axis:mappedAxis:value:reversed:_axisValueSave" << axis << mappedAxis << value << info->reversed << _axisValueSave[mappedAxis];
            
            _signalAllAttiudeValueChanges();
            
            _advanceState();
        }
    }
}

void JoystickConfigController::_inputStickMin(Joystick::Axis_t axis, int mappedAxis, int value)
{
    qCDebug(JoystickConfigControllerLog) << "_inputStickMin axis:mappedAxis:value" << axis << mappedAxis << value;

    if (!_validAxis(mappedAxis)) {
        qCWarning(JoystickConfigControllerLog) << "Invalid axis mappedAxis:_axisCount" << mappedAxis << _axisCount;
        return;
    }

    // We only care about the axis mapped to the function we are working on
    if (_rgAxisMapping[axis] != mappedAxis) {
        return;
    }

    if (_stickDetectAxis == Joystick::maxAxis) {
        // Setup up to detect stick being pegged to extreme position
        if (_rgAxisInfo[mappedAxis].reversed) {
            if (value > _calCenterPoint + _calMoveDelta) {
                _stickDetectAxis = mappedAxis;
                _stickDetectInitialValue = value;
                _stickDetectValue = value;
                qCDebug(JoystickConfigControllerLog) << "_inputStickMin detected movement _stickDetectAxis:_stickDetectInitialValue" << _stickDetectAxis << _stickDetectInitialValue;
            }
        } else {
            if (value < _calCenterPoint - _calMoveDelta) {
                _stickDetectAxis = mappedAxis;
                _stickDetectInitialValue = value;
                _stickDetectValue = value;
                qCDebug(JoystickConfigControllerLog) << "_inputStickMin detected movement _stickDetectAxis:_stickDetectInitialValue" << _stickDetectAxis << _stickDetectInitialValue;
            }
        }
    } else {
        // We are waiting for the selected axis to settle out
        
        if (_stickSettleComplete(mappedAxis, value)) {
            AxisInfo* info = &_rgAxisInfo[mappedAxis];
            
            // Stick detection is complete. Stick should be at min position.
            if (info->reversed) {
                _rgAxisInfo[mappedAxis].axisMax = value;
            } else {
                _rgAxisInfo[mappedAxis].axisMin = value;
            }

            qCDebug(JoystickConfigControllerLog) << "_inputStickMin saving values, axis:mappedAxis:value:reversed" << axis << mappedAxis << value << info->reversed;
            
            _advanceState();
        }
    }
}

void JoystickConfigController::_inputCenterWait(Joystick::Axis_t axis, int mappedAxis, int value)
{
    qCDebug(JoystickConfigControllerLog) << "_inputCenterWait axis:mappedAxis:value" << axis << mappedAxis << value;

    if (!_validAxis(mappedAxis)) {
        qCWarning(JoystickConfigControllerLog) << "Invalid axis mappedAxis:_axisCount" << mappedAxis << _axisCount;
        return;
    }

    // We only care about the axis mapped to the function we are working on
    if (_rgAxisMapping[axis] != mappedAxis) {
        return;
    }
    
    if (_stickDetectAxis == Joystick::maxAxis) {
        // Sticks have not yet moved close enough to center
        
        if (abs(_calCenterPoint - value) < _calRoughCenterDelta) {
            // Stick has moved close enough to center that we can start waiting for it to settle
            _stickDetectAxis = mappedAxis;
            _stickDetectInitialValue = value;
            _stickDetectValue = value;
            qCDebug(JoystickConfigControllerLog) << "_inputStickMin detected possible center _stickDetectAxis:_stickDetectInitialValue" << _stickDetectAxis << _stickDetectInitialValue;
        }
    } else {
        if (_stickSettleComplete(mappedAxis, value)) {
            _advanceState();
        }
    }
}

/// @brief Resets internal calibration values to their initial state in preparation for a new calibration sequence.
void JoystickConfigController::_resetInternalCalibrationValues(void)
{
    // Set all raw axes to not reversed and center point values
    for (int i=0; i<_axisCount; i++) {
        struct AxisInfo* info = &_rgAxisInfo[i];
        info->function = Joystick::maxFunction;
        info->axis = Joystick::maxAxis;
        info->reversed = false;
        info->deadband = 0;
        info->axisMin = JoystickConfigController::_calCenterPoint;
        info->axisMax = JoystickConfigController::_calCenterPoint;
        info->axisTrim = JoystickConfigController::_calCenterPoint;
    }
    
    for (int function=0; function<Joystick::maxFunction; function++) {
        Joystick::Axis_t realAxis;

        realAxis = _activeJoystick->getFunctionAxis((Joystick::AxisFunction_t)function);

        _rgFunctionAxisMapping[function] = realAxis;
    }

    for (size_t i=0; i<Joystick::maxAxis; i++) {
        _rgAxisMapping[i] = Joystick::maxAxis;
    }
    
    _signalAllAttiudeValueChanges();
}

/// @brief Sets internal calibration values from the stored settings
void JoystickConfigController::_setInternalCalibrationValuesFromSettings(void)
{
    Joystick* joystick = _joystickManager->activeJoystick();
    
    // Initialize all function mappings to not set
    
    for (int i=0; i<_axisCount; i++) {
        struct AxisInfo* info = &_rgAxisInfo[i];
        info->function = Joystick::maxFunction;
        info->axis = Joystick::maxAxis;
    }

    for (size_t i=0; i<Joystick::maxFunction; i++) {
        _rgFunctionAxisMapping[i] = (Joystick::Axis_t)Joystick::maxAxis;
    }

    for (size_t i=0; i<Joystick::maxAxis; i++) {
        _rgAxisMapping[i] = Joystick::maxAxis;
    }
    
    for (int axis=0; axis<_axisCount; axis++) {
        struct AxisInfo* info = &_rgAxisInfo[axis];
        
        Joystick::Calibration_t calibration = joystick->getCalibration(axis);
        info->axisTrim = calibration.center;
        info->axisMin = calibration.min;
        info->axisMax = calibration.max;
        info->reversed = calibration.reversed;
        info->deadband = calibration.deadband;

        qCDebug(JoystickConfigControllerLog) << "Read settings name:axis:min:max:trim:reversed" << joystick->name() << axis << info->axisMin << info->axisMax << info->axisTrim << info->reversed;
    }
    

    for (int axis=0; axis<Joystick::maxAxis; axis++) {
        int mappedAxis = joystick->getMappedAxis((Joystick::Axis_t)axis);
        _rgAxisMapping[axis] = mappedAxis;
        _rgAxisInfo[mappedAxis].axis = (Joystick::Axis_t)axis;
    }

    for (int function=0; function<Joystick::maxFunction; function++) {
        Joystick::Axis_t realAxis;

        realAxis = joystick->getFunctionAxis((Joystick::AxisFunction_t)function);

        _rgFunctionAxisMapping[function] = realAxis;

        int mappedAxis = _rgAxisMapping[realAxis];
        _rgAxisInfo[mappedAxis].function = (Joystick::AxisFunction_t)function;
    }

    qDebug() << "\nstickLeftX is mapped to raw axis:" << _rgAxisMapping[Joystick::stickLeftX];
    qDebug() << "stickLeftY is mapped to raw axis:" << _rgAxisMapping[Joystick::stickLeftY];
    qDebug() << "stickRightX is mapped to raw axis:" << _rgAxisMapping[Joystick::stickRightX];
    qDebug() << "stickRightY is mapped to raw axis:" << _rgAxisMapping[Joystick::stickRightY];

    _signalAllAttiudeValueChanges();
}

/// @brief Validates the current settings against the calibration rules resetting values as necessary.
void JoystickConfigController::_validateCalibration(void)
{
    for (int chan = 0; chan<_axisCount; chan++) {
        struct AxisInfo* info = &_rgAxisInfo[chan];
        
        if (chan < _axisCount) {
            // Validate Min/Max values. Although the axis appears as available we still may
            // not have good min/max/trim values for it. Set to defaults if needed.
            if (info->axisMin < _calValidMinValue || info->axisMax > _calValidMaxValue) {
                qCDebug(JoystickConfigControllerLog) << "_validateCalibration resetting axis" << chan;
                info->axisMin = _calDefaultMinValue;
                info->axisMax = _calDefaultMaxValue;
                info->axisTrim = info->axisMin + ((info->axisMax - info->axisMin) / 2);
            }
            
            switch (_rgAxisInfo[chan].function) {
                case Joystick::throttleFunction:
                case Joystick::yawFunction:
                case Joystick::rollFunction:
                case Joystick::pitchFunction:
                    // Make sure trim is within min/max
                    if (info->axisTrim < info->axisMin) {
                        info->axisTrim = info->axisMin;
                    } else if (info->axisTrim > info->axisMax) {
                        info->axisTrim = info->axisMax;
                    }
                    break;
                default:
                    // Non-attitude control axis have calculated trim
                    info->axisTrim = info->axisMin + ((info->axisMax - info->axisMin) / 2);
                    break;
            }
        } else {
            // Unavailable axes are set to defaults
            qCDebug(JoystickConfigControllerLog) << "_validateCalibration resetting unavailable axis" << chan;
            info->axisMin = _calDefaultMinValue;
            info->axisMax = _calDefaultMaxValue;
            info->axisTrim = info->axisMin + ((info->axisMax - info->axisMin) / 2);
            info->deadband = 0;
            info->reversed = false;
        }
    }
}


/// @brief Saves the rc calibration values to the board parameters.
void JoystickConfigController::_writeCalibration(void)
{
    Joystick* joystick = _joystickManager->activeJoystick();

    _validateCalibration();
    
    for (int axis=0; axis<_axisCount; axis++) {
        Joystick::Calibration_t calibration;
        
        struct AxisInfo* info = &_rgAxisInfo[axis];
        
        calibration.center = info->axisTrim;
        calibration.min = info->axisMin;
        calibration.max = info->axisMax;
        calibration.reversed = info->reversed;
        calibration.deadband = info->deadband;
        
        joystick->setCalibration(axis, calibration);
    }

	// Write axis mappings
    for (int axis=0; axis<Joystick::maxAxis; axis++) {
        joystick->setAxisMapping((Joystick::Axis_t)axis, _rgAxisMapping[axis]);
    }
    
    _stopCalibration();
    _setInternalCalibrationValuesFromSettings();

    Vehicle* vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    if (vehicle) {
        vehicle->setJoystickEnabled(true);
    }
}

/// @brief Starts the calibration process
void JoystickConfigController::_startCalibration(void)
{
    _activeJoystick->startCalibrationMode(Joystick::CalibrationModeCalibrating);
    _resetInternalCalibrationValues();
    
    _nextButton->setProperty("text", "Next");
    _cancelButton->setEnabled(true);
    
    _currentStep = 0;
    _setupCurrentState();
}

/// @brief Cancels the calibration process, setting things back to initial state.
void JoystickConfigController::_stopCalibration(void)
{
    _currentStep = -1;
    
    _activeJoystick->stopCalibrationMode(Joystick::CalibrationModeCalibrating);
    _setInternalCalibrationValuesFromSettings();
    
    _statusText->setProperty("text", "");

    _nextButton->setProperty("text", "Calibrate");
    _nextButton->setEnabled(true);
    _cancelButton->setEnabled(false);
    _skipButton->setEnabled(false);
    
    _setHelpImage(_imageCenter);
}

/// @brief Saves the current axis values, so that we can detect when the user moves an input.
void JoystickConfigController::_calSaveCurrentValues(void)
{
    qCDebug(JoystickConfigControllerLog) << "_calSaveCurrentValues";
    for (int i = 0; i < _axisCount; i++) {
        _axisValueSave[i] = _axisRawValue[i];
    }
}

/// @brief Set up the Save state of calibration.
void JoystickConfigController::_calSave(void)
{
    _calState = calStateSave;
    
    _statusText->setProperty("text",
                             "The current calibration settings are now displayed for each axis on screen.\n\n"
                                "Click the Next button to upload calibration to board. Click Cancel if you don't want to save these values.");

    _nextButton->setEnabled(true);
    _skipButton->setEnabled(false);
    _cancelButton->setEnabled(true);
    
    // This updates the internal values according to the validation rules. Then _updateView will tick and update ui
    // such that the settings that will be written our are displayed.
    _validateCalibration();
}

void JoystickConfigController::_setHelpImage(const char* imageFile)
{
    QString file = _imageFilePrefix;
    
    file += _imageFileMode2Dir;
    file += imageFile;
    
    qCDebug(JoystickConfigControllerLog) << "_setHelpImage" << file;
    
    _imageHelp = file;
    emit imageHelpChanged(file);
}

int JoystickConfigController::axisCount(void)
{
    return _axisCount;
}

int JoystickConfigController::rollAxisValue(void)
{    
    if (_rgFunctionAxisMapping[Joystick::rollFunction] != Joystick::maxAxis) {
        return _axisRawValue[Joystick::rollFunction];
    } else {
        return 1500;
    }
}

int JoystickConfigController::pitchAxisValue(void)
{
    if (_rgFunctionAxisMapping[Joystick::pitchFunction] != Joystick::maxAxis) {
        return _axisRawValue[Joystick::pitchFunction];
    } else {
        return 1500;
    }
}

int JoystickConfigController::yawAxisValue(void)
{
    if (_rgFunctionAxisMapping[Joystick::yawFunction] != Joystick::maxAxis) {
        return _axisRawValue[Joystick::yawFunction];
    } else {
        return 1500;
    }
}

int JoystickConfigController::throttleAxisValue(void)
{
    if (_rgFunctionAxisMapping[Joystick::throttleFunction] != Joystick::maxAxis) {
        return _axisRawValue[Joystick::throttleFunction];
    } else {
        return 1500;
    }
}

int JoystickConfigController::rollAxisDeadband(void)
{
    if ((_rgFunctionAxisMapping[Joystick::rollFunction] != Joystick::maxAxis) && (_activeJoystick->deadband())) {
        return _rgAxisInfo[_rgFunctionAxisMapping[Joystick::rollFunction]].deadband;
    } else {
        return 0;
    }
}

int JoystickConfigController::pitchAxisDeadband(void)
{
    if ((_rgFunctionAxisMapping[Joystick::pitchFunction] != Joystick::maxAxis) && (_activeJoystick->deadband())) {
        return _rgAxisInfo[_rgFunctionAxisMapping[Joystick::pitchFunction]].deadband;
    } else {
        return 0;
    }
}

int JoystickConfigController::yawAxisDeadband(void)
{
    if ((_rgFunctionAxisMapping[Joystick::yawFunction] != Joystick::maxAxis) && (_activeJoystick->deadband())) {
        return _rgAxisInfo[_rgFunctionAxisMapping[Joystick::yawFunction]].deadband;
    } else {
        return 0;
    }
}

int JoystickConfigController::throttleAxisDeadband(void)
{
    if ((_rgFunctionAxisMapping[Joystick::throttleFunction] != Joystick::maxAxis) && (_activeJoystick->deadband())) {
        return _rgAxisInfo[_rgFunctionAxisMapping[Joystick::throttleFunction]].deadband;
    } else {
        return 0;
    }
}

bool JoystickConfigController::rollAxisMapped(void)
{
    int axis = _rgFunctionAxisMapping[Joystick::rollFunction];
    int mappedAxis = _rgAxisMapping[axis];
    if (mappedAxis == Joystick::maxAxis) {
        return false;
    }
    return _rgAxisInfo[mappedAxis].axis != Joystick::maxAxis;
}

bool JoystickConfigController::pitchAxisMapped(void)
{
    int axis = _rgFunctionAxisMapping[Joystick::pitchFunction];
    int mappedAxis = _rgAxisMapping[axis];
    if (mappedAxis == Joystick::maxAxis) {
        return false;
    }
    return _rgAxisInfo[mappedAxis].axis != Joystick::maxAxis;
}

bool JoystickConfigController::yawAxisMapped(void)
{
    int axis = _rgFunctionAxisMapping[Joystick::yawFunction];
    int mappedAxis = _rgAxisMapping[axis];
    if (mappedAxis == Joystick::maxAxis) {
        return false;
    }
    return _rgAxisInfo[mappedAxis].axis != Joystick::maxAxis;
}

bool JoystickConfigController::throttleAxisMapped(void)
{
    int axis = _rgFunctionAxisMapping[Joystick::throttleFunction];
    int mappedAxis = _rgAxisMapping[axis];
    if (mappedAxis == Joystick::maxAxis) {
        return false;
    }
    return _rgAxisInfo[mappedAxis].axis != Joystick::maxAxis;
}

bool JoystickConfigController::rollAxisReversed(void)
{
    if (_rgFunctionAxisMapping[Joystick::rollFunction] != Joystick::maxAxis) {
        return _rgAxisInfo[_rgAxisMapping[_rgFunctionAxisMapping[Joystick::rollFunction]]].reversed;
    } else {
        return false;
    }
}

bool JoystickConfigController::pitchAxisReversed(void)
{
    if (_rgFunctionAxisMapping[Joystick::pitchFunction] != Joystick::maxAxis) {
        return _rgAxisInfo[_rgAxisMapping[_rgFunctionAxisMapping[Joystick::pitchFunction]]].reversed;
    } else {
        return false;
    }
}

bool JoystickConfigController::yawAxisReversed(void)
{
    if (_rgFunctionAxisMapping[Joystick::yawFunction] != Joystick::maxAxis) {
        return _rgAxisInfo[_rgAxisMapping[_rgFunctionAxisMapping[Joystick::yawFunction]]].reversed;
    } else {
        return false;
    }
}

bool JoystickConfigController::throttleAxisReversed(void)
{
    if (_rgFunctionAxisMapping[Joystick::throttleFunction] != Joystick::maxAxis) {
        return _rgAxisInfo[_rgAxisMapping[_rgFunctionAxisMapping[Joystick::throttleFunction]]].reversed;
    } else {
        return false;
    }
}

void JoystickConfigController::_signalAllAttiudeValueChanges(void)
{
    emit rollAxisMappedChanged(rollAxisMapped());
    emit pitchAxisMappedChanged(pitchAxisMapped());
    emit yawAxisMappedChanged(yawAxisMapped());
    emit throttleAxisMappedChanged(throttleAxisMapped());
    
    emit rollAxisReversedChanged(rollAxisReversed());
    emit pitchAxisReversedChanged(pitchAxisReversed());
    emit yawAxisReversedChanged(yawAxisReversed());
    emit throttleAxisReversedChanged(throttleAxisReversed());

    emit rollAxisDeadbandChanged(rollAxisDeadband());
    emit pitchAxisDeadbandChanged(pitchAxisDeadband());
    emit yawAxisDeadbandChanged(yawAxisDeadband());
    emit throttleAxisDeadbandChanged(throttleAxisDeadband());
}

void JoystickConfigController::_activeJoystickChanged(Joystick* joystick)
{
    bool joystickTransition = false;
    
    if (_activeJoystick) {
        joystickTransition = true;
        disconnect(_activeJoystick, &Joystick::rawAxisValueChanged, this, &JoystickConfigController::_axisValueChanged);
        disconnect(_activeJoystick, &Joystick::modeChanged, this, &JoystickConfigController::_modeChanged);
        delete _rgAxisInfo;
        delete _axisValueSave;
        delete _axisRawValue;
        _axisCount = 0;
        _activeJoystick = NULL;
    }
    
    if (joystick) {
        _activeJoystick = joystick;
        if (joystickTransition) {
            _stopCalibration();
        }
        _activeJoystick->startCalibrationMode(Joystick::CalibrationModeMonitor);
        _axisCount = _activeJoystick->axisCount();
        _rgAxisInfo = new struct AxisInfo[_axisCount];
        _axisValueSave = new int[_axisCount];
        _axisRawValue = new int[_axisCount];
        connect(_activeJoystick, &Joystick::rawAxisValueChanged, this, &JoystickConfigController::_axisValueChanged);
        connect(_activeJoystick, &Joystick::modeChanged, this, &JoystickConfigController::_modeChanged);
    }
}

void JoystickConfigController::_modeChanged(int mode)
{
    qCDebug(JoystickConfigControllerLog) << "Mode changed:" << mode;
    for (int function=0; function<Joystick::maxFunction; function++) {
        Joystick::Axis_t realAxis;

        // Get axis for this function (stickRightX etc.)
        realAxis = _activeJoystick->getFunctionAxis((Joystick::AxisFunction_t)function);

        _rgFunctionAxisMapping[function] = realAxis;

        int mappedAxis = _rgAxisMapping[realAxis];
        _rgAxisInfo[mappedAxis].function = (Joystick::AxisFunction_t)function;
    }
    _signalAllAttiudeValueChanges();
}

bool JoystickConfigController::_validAxis(int axis)
{
    return axis >= 0 && axis < _axisCount;
}
