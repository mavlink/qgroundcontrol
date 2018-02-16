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

const char*  JoystickConfigController::_imageFilePrefix =   "calibration/joystick/";
const char*  JoystickConfigController::_imageFileMode1Dir = "mode1/";
const char*  JoystickConfigController::_imageFileMode2Dir = "mode2/";
const char*  JoystickConfigController::_imageFileMode3Dir = "mode3/";
const char*  JoystickConfigController::_imageFileMode4Dir = "mode4/";
const char*  JoystickConfigController::_imageCenter =       "joystickCenter.png";
const char*  JoystickConfigController::_imageThrottleUp =   "joystickThrottleUp.png";
const char*  JoystickConfigController::_imageThrottleDown = "joystickThrottleDown.png";
const char*  JoystickConfigController::_imageYawLeft =      "joystickYawLeft.png";
const char*  JoystickConfigController::_imageYawRight =     "joystickYawRight.png";
const char*  JoystickConfigController::_imageRollLeft =     "joystickRollLeft.png";
const char*  JoystickConfigController::_imageRollRight =    "joystickRollRight.png";
const char*  JoystickConfigController::_imagePitchUp =      "joystickPitchUp.png";
const char*  JoystickConfigController::_imagePitchDown =    "joystickPitchDown.png";

JoystickConfigController::JoystickConfigController(void)
    : _activeJoystick(NULL)
    , _transmitterMode(2)
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

void JoystickConfigController::setDeadbandValue(int axis, int value)
{
    _axisDeadbandChanged(axis,value);
    Joystick* joystick = _joystickManager->activeJoystick();
    Joystick::Calibration_t calibration = joystick->getCalibration(axis);
    calibration.deadband = value;
    joystick->setCalibration(axis,calibration);
}

JoystickConfigController::~JoystickConfigController()
{
    if(_activeJoystick) {
        _activeJoystick->setCalibrationMode(false);
    }
}

/// @brief Returns the state machine entry for the specified state.
const JoystickConfigController::stateMachineEntry* JoystickConfigController::_getStateMachineEntry(int step)
{
    static const char* msgBegin =           "Allow all sticks to center as shown in diagram.\n\nClick Next to continue";
    static const char* msgThrottleUp =      "Move the Throttle stick all the way up and hold it there...";
    static const char* msgThrottleDown =    "Move the Throttle stick all the way down and hold it there...";
    static const char* msgYawLeft =         "Move the Yaw stick all the way to the left and hold it there...";
    static const char* msgYawRight =        "Move the Yaw stick all the way to the right and hold it there...";
    static const char* msgRollLeft =        "Move the Roll stick all the way to the left and hold it there...";
    static const char* msgRollRight =       "Move the Roll stick all the way to the right and hold it there...";
    static const char* msgPitchDown =       "Move the Pitch stick all the way down and hold it there...";
    static const char* msgPitchUp =         "Move the Pitch stick all the way up and hold it there...";
    static const char* msgPitchCenter =     "Allow the Pitch stick to move back to center...";
    static const char* msgComplete =        "All settings have been captured. Click Next to enable the joystick.";
    
    static const stateMachineEntry rgStateMachine[] = {
        //Function
        { Joystick::maxFunction,       msgBegin,           _imageCenter,       &JoystickConfigController::_inputCenterWaitBegin,   &JoystickConfigController::_saveAllTrims,       NULL },
        { Joystick::throttleFunction,  msgThrottleUp,      _imageThrottleUp,   &JoystickConfigController::_inputStickDetect,       NULL,                                           NULL },
        { Joystick::throttleFunction,  msgThrottleDown,    _imageThrottleDown, &JoystickConfigController::_inputStickMin,          NULL,                                           NULL },
        { Joystick::yawFunction,       msgYawRight,        _imageYawRight,     &JoystickConfigController::_inputStickDetect,       NULL,                                           NULL },
        { Joystick::yawFunction,       msgYawLeft,         _imageYawLeft,      &JoystickConfigController::_inputStickMin,          NULL,                                           NULL },
        { Joystick::rollFunction,      msgRollRight,       _imageRollRight,    &JoystickConfigController::_inputStickDetect,       NULL,                                           NULL },
        { Joystick::rollFunction,      msgRollLeft,        _imageRollLeft,     &JoystickConfigController::_inputStickMin,          NULL,                                           NULL },
        { Joystick::pitchFunction,     msgPitchUp,         _imagePitchUp,      &JoystickConfigController::_inputStickDetect,       NULL,                                           NULL },
        { Joystick::pitchFunction,     msgPitchDown,       _imagePitchDown,    &JoystickConfigController::_inputStickMin,          NULL,                                           NULL },
        { Joystick::pitchFunction,     msgPitchCenter,     _imageCenter,       &JoystickConfigController::_inputCenterWait,        NULL,                                           NULL },
        { Joystick::maxFunction,       msgComplete,        _imageCenter,       NULL,                                               &JoystickConfigController::_writeCalibration,   NULL },
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
    
    _stickDetectAxis = _axisNoAxis;
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
                (this->*state->rcInputFn)(state->function, axis, value);
            }
        }
    }
}

void JoystickConfigController::nextButtonClicked(void)
{
    if (_currentStep == -1) {
        // Need to have enough channels
        if (_axisCount < _axisMinimum) {
            qgcApp()->showMessage(tr("Detected %1 joystick axes. To operate PX4, you need at least %2 axes.").arg(_axisCount).arg(_axisMinimum));
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

    _signalAllAttitudeValueChanges();

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
    emit axisDeadbandChanged(axis,value);
    qCDebug(JoystickConfigControllerLog) << "Axis:" << axis << "Deadband:" << _rgAxisInfo[axis].deadband;
}

/// @brief Waits for the sticks to be centered, enabling Next when done.
void JoystickConfigController::_inputCenterWaitBegin(Joystick::AxisFunction_t function, int axis, int value)
{
    Q_UNUSED(function);

    //sensing deadband
    if ((abs(value)*1.1f>_rgAxisInfo[axis].deadband)&&(_activeJoystick->deadband())) {   //add 10% on top of existing deadband
        _axisDeadbandChanged(axis,abs(value)*1.1f);
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

void JoystickConfigController::_inputStickDetect(Joystick::AxisFunction_t function, int axis, int value)
{
    qCDebug(JoystickConfigControllerLog) << "_inputStickDetect function:axis:value" << function << axis << value;
    
    if (!_validAxis(axis)) {
        qCWarning(JoystickConfigControllerLog) << "Invalid axis axis:_axisCount" << axis << _axisCount;
        return;
    }

    // If this axis is already used in a mapping we can't use it again
    if (_rgAxisInfo[axis].function != Joystick::maxFunction) {
        return;
    }
    
    if (_stickDetectAxis == _axisNoAxis) {
        // We have not detected enough movement on a axis yet
        
        if (abs(_axisValueSave[axis] - value) > _calMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function
            
            qCDebug(JoystickConfigControllerLog) << "_inputStickDetect starting settle wait, function:axis:value" << function << axis << value;
            
            // Setup up to detect stick being pegged to min or max value
            _stickDetectAxis = axis;
            _stickDetectInitialValue = value;
            _stickDetectValue = value;
        }
    } else if (axis == _stickDetectAxis) {
        if (_stickSettleComplete(axis, value)) {
            AxisInfo* info = &_rgAxisInfo[axis];
            
            // Stick detection is complete. Stick should be at max position.
            // Map the axis to the function
            _rgFunctionAxisMapping[function] = axis;
            info->function = function;
            
            // Axis should be at max value, if it is below initial set point the the axis is reversed.
            info->reversed = value < _axisValueSave[axis];
            
            if (info->reversed) {
                _rgAxisInfo[axis].axisMin = value;
            } else {
                _rgAxisInfo[axis].axisMax = value;
            }
            
            qCDebug(JoystickConfigControllerLog) << "_inputStickDetect saving values, function:axis:value:reversed:_axisValueSave" << function << axis << value << info->reversed << _axisValueSave[axis];
            
            _signalAllAttitudeValueChanges();
            
            _advanceState();
        }
    }
}

void JoystickConfigController::_inputStickMin(Joystick::AxisFunction_t function, int axis, int value)
{
    qCDebug(JoystickConfigControllerLog) << "_inputStickMin function:axis:value" << function << axis << value;
    
    if (!_validAxis(axis)) {
        qCWarning(JoystickConfigControllerLog) << "Invalid axis axis:_axisCount" << axis << _axisCount;
        return;
    }

    // We only care about the axis mapped to the function we are working on
    if (_rgFunctionAxisMapping[function] != axis) {
        return;
    }

    if (_stickDetectAxis == _axisNoAxis) {
        // Setup up to detect stick being pegged to extreme position
        if (_rgAxisInfo[axis].reversed) {
            if (value > _calCenterPoint + _calMoveDelta) {
                _stickDetectAxis = axis;
                _stickDetectInitialValue = value;
                _stickDetectValue = value;
                qCDebug(JoystickConfigControllerLog) << "_inputStickMin detected movement _stickDetectAxis:_stickDetectInitialValue" << _stickDetectAxis << _stickDetectInitialValue;
            }
        } else {
            if (value < _calCenterPoint - _calMoveDelta) {
                _stickDetectAxis = axis;
                _stickDetectInitialValue = value;
                _stickDetectValue = value;
                qCDebug(JoystickConfigControllerLog) << "_inputStickMin detected movement _stickDetectAxis:_stickDetectInitialValue" << _stickDetectAxis << _stickDetectInitialValue;
            }
        }
    } else {
        // We are waiting for the selected axis to settle out
        
        if (_stickSettleComplete(axis, value)) {
            AxisInfo* info = &_rgAxisInfo[axis];
            
            // Stick detection is complete. Stick should be at min position.
            if (info->reversed) {
                _rgAxisInfo[axis].axisMax = value;
            } else {
                _rgAxisInfo[axis].axisMin = value;
            }
            
            qCDebug(JoystickConfigControllerLog) << "_inputStickMin saving values, function:axis:value:reversed" << function << axis << value << info->reversed;
            
            _advanceState();
        }
    }
}

void JoystickConfigController::_inputCenterWait(Joystick::AxisFunction_t function, int axis, int value)
{
    qCDebug(JoystickConfigControllerLog) << "_inputCenterWait function:axis:value" << function << axis << value;
    
    if (!_validAxis(axis)) {
        qCWarning(JoystickConfigControllerLog) << "Invalid axis axis:_axisCount" << axis << _axisCount;
        return;
    }

    // We only care about the axis mapped to the function we are working on
    if (_rgFunctionAxisMapping[function] != axis) {
        return;
    }
    
    if (_stickDetectAxis == _axisNoAxis) {
        // Sticks have not yet moved close enough to center
        int roughCenter = getDeadbandToggle() ? std::max(_rgAxisInfo[axis].deadband,_calRoughCenterDelta) : _calRoughCenterDelta;
        if (abs(_calCenterPoint - value) < roughCenter) {
            // Stick has moved close enough to center that we can start waiting for it to settle
            _stickDetectAxis = axis;
            _stickDetectInitialValue = value;
            _stickDetectValue = value;
            qCDebug(JoystickConfigControllerLog) << "_inputStickMin detected possible center _stickDetectAxis:_stickDetectInitialValue" << _stickDetectAxis << _stickDetectInitialValue;
        }
    } else {
        if (_stickSettleComplete(axis, value)) {
            _advanceState();
        }
    }
}

/// @brief Resets internal calibration values to their initial state in preparation for a new calibration sequence.
void JoystickConfigController::_resetInternalCalibrationValues(void)
{
    // Set all raw axiss to not reversed and center point values
    for (int i=0; i<_axisCount; i++) {
        struct AxisInfo* info = &_rgAxisInfo[i];
        info->function = Joystick::maxFunction;
        info->reversed = false;
        info->deadband = 0;
        emit axisDeadbandChanged(i,info->deadband);
        info->axisMin = JoystickConfigController::_calCenterPoint;
        info->axisMax = JoystickConfigController::_calCenterPoint;
        info->axisTrim = JoystickConfigController::_calCenterPoint;
    }
    
    // Initialize attitude function mapping to function axis not set
    for (size_t i=0; i<Joystick::maxFunction; i++) {
        _rgFunctionAxisMapping[i] = _axisNoAxis;
    }
    
    _signalAllAttitudeValueChanges();
}

/// @brief Sets internal calibration values from the stored settings
void JoystickConfigController::_setInternalCalibrationValuesFromSettings(void)
{
    Joystick* joystick = _joystickManager->activeJoystick();
    
    // Initialize all function mappings to not set
    
    for (int i=0; i<_axisCount; i++) {
        struct AxisInfo* info = &_rgAxisInfo[i];
        info->function = Joystick::maxFunction;
    }
    
    for (size_t i=0; i<Joystick::maxFunction; i++) {
        _rgFunctionAxisMapping[i] = _axisNoAxis;
    }
    
    for (int axis=0; axis<_axisCount; axis++) {
        struct AxisInfo* info = &_rgAxisInfo[axis];
        
        Joystick::Calibration_t calibration = joystick->getCalibration(axis);
        info->axisTrim = calibration.center;
        info->axisMin = calibration.min;
        info->axisMax = calibration.max;
        info->reversed = calibration.reversed;
        info->deadband = calibration.deadband;
        emit axisDeadbandChanged(axis,info->deadband);

        qCDebug(JoystickConfigControllerLog) << "Read settings name:axis:min:max:trim:reversed" << joystick->name() << axis << info->axisMin << info->axisMax << info->axisTrim << info->reversed;
    }
    
    for (int function=0; function<Joystick::maxFunction; function++) {
        int paramAxis;
        
        paramAxis = joystick->getFunctionAxis((Joystick::AxisFunction_t)function);
        if(paramAxis >= 0) {
            _rgFunctionAxisMapping[function] = paramAxis;
            _rgAxisInfo[paramAxis].function = (Joystick::AxisFunction_t)function;
        }
    }

    _transmitterMode = joystick->getTXMode();
    
    _signalAllAttitudeValueChanges();
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
            // Unavailable axiss are set to defaults
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
    
    // Write function mapping parameters
    for (int function=0; function<Joystick::maxFunction; function++) {
        joystick->setFunctionAxis((Joystick::AxisFunction_t)function, _rgFunctionAxisMapping[function]);
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
    _activeJoystick->setCalibrationMode(true);
    _resetInternalCalibrationValues();
    
    _nextButton->setProperty("text", "Next");
    _cancelButton->setEnabled(true);
    
    _currentStep = 0;
    _setupCurrentState();
    emit calibratingChanged();
}

/// @brief Cancels the calibration process, setting things back to initial state.
void JoystickConfigController::_stopCalibration(void)
{
    _currentStep = -1;
    
    _activeJoystick->setCalibrationMode(false);
    _setInternalCalibrationValuesFromSettings();
    
    _statusText->setProperty("text", "");

    _nextButton->setProperty("text", tr("Calibrate"));
    _nextButton->setEnabled(true);
    _cancelButton->setEnabled(false);
    _skipButton->setEnabled(false);
    
    _setHelpImage(_imageCenter);
    emit calibratingChanged();
}

/// @brief Saves the current axis values, so that we can detect when the use moves an input.
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
                             tr("The current calibration settings are now displayed for each axis on screen.\n\n"
                                "Click the Next button to upload calibration to board. Click Cancel if you don't want to save these values."));

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
    
    switch(_transmitterMode) {
    case 1:
        file += _imageFileMode1Dir;
        break;
    case 2:
        file += _imageFileMode2Dir;
        break;
    case 3:
        file += _imageFileMode3Dir;
        break;
    case 4:
        file += _imageFileMode4Dir;
        break;
    default:
        Q_ASSERT(false);
    }

    file += imageFile;
    
    qCDebug(JoystickConfigControllerLog) << "_setHelpImage" << file;
    
    _imageHelp = file;
    emit imageHelpChanged(file);
}

int JoystickConfigController::axisCount(void)
{
    return _axisCount;
}



bool JoystickConfigController::rollAxisMapped(void)
{
    return _rgFunctionAxisMapping[Joystick::rollFunction] != _axisNoAxis;
}

bool JoystickConfigController::pitchAxisMapped(void)
{
    return _rgFunctionAxisMapping[Joystick::pitchFunction] != _axisNoAxis;
}

bool JoystickConfigController::yawAxisMapped(void)
{
    return _rgFunctionAxisMapping[Joystick::yawFunction] != _axisNoAxis;
}

bool JoystickConfigController::throttleAxisMapped(void)
{
    return _rgFunctionAxisMapping[Joystick::throttleFunction] != _axisNoAxis;
}

bool JoystickConfigController::rollAxisReversed(void)
{
    if (_rgFunctionAxisMapping[Joystick::rollFunction] != _axisNoAxis) {
        return _rgAxisInfo[_rgFunctionAxisMapping[Joystick::rollFunction]].reversed;
    } else {
        return false;
    }
}

bool JoystickConfigController::pitchAxisReversed(void)
{
    if (_rgFunctionAxisMapping[Joystick::pitchFunction] != _axisNoAxis) {
        return _rgAxisInfo[_rgFunctionAxisMapping[Joystick::pitchFunction]].reversed;
    } else {
        return false;
    }
}

bool JoystickConfigController::yawAxisReversed(void)
{
    if (_rgFunctionAxisMapping[Joystick::yawFunction] != _axisNoAxis) {
        return _rgAxisInfo[_rgFunctionAxisMapping[Joystick::yawFunction]].reversed;
    } else {
        return false;
    }
}

bool JoystickConfigController::throttleAxisReversed(void)
{
    if (_rgFunctionAxisMapping[Joystick::throttleFunction] != _axisNoAxis) {
        return _rgAxisInfo[_rgFunctionAxisMapping[Joystick::throttleFunction]].reversed;
    } else {
        return false;
    }
}

void JoystickConfigController::setTransmitterMode(int mode)
{
    if (mode > 0 && mode <= 4) {
        _transmitterMode = mode;
        if (_currentStep != -1) { // This should never be true, mode selection is disabled during calibration
            const stateMachineEntry* state = _getStateMachineEntry(_currentStep);
            _setHelpImage(state->image);
        } else {
            _activeJoystick->setTXMode(mode);
            _setInternalCalibrationValuesFromSettings();
        }
    }
}

void JoystickConfigController::_signalAllAttitudeValueChanges(void)
{
    emit rollAxisMappedChanged(rollAxisMapped());
    emit pitchAxisMappedChanged(pitchAxisMapped());
    emit yawAxisMappedChanged(yawAxisMapped());
    emit throttleAxisMappedChanged(throttleAxisMapped());
    
    emit rollAxisReversedChanged(rollAxisReversed());
    emit pitchAxisReversedChanged(pitchAxisReversed());
    emit yawAxisReversedChanged(yawAxisReversed());
    emit throttleAxisReversedChanged(throttleAxisReversed());

    emit transmitterModeChanged(_transmitterMode);
}

void JoystickConfigController::_activeJoystickChanged(Joystick* joystick)
{
    bool joystickTransition = false;
    if (_activeJoystick) {
        joystickTransition = true;
        disconnect(_activeJoystick, &Joystick::rawAxisValueChanged, this, &JoystickConfigController::_axisValueChanged);
        // This will reset _rgFunctionAxis values to -1 to prevent out-of-bounds accesses
        _resetInternalCalibrationValues();
        delete[] _rgAxisInfo;
        delete[] _axisValueSave;
        delete[] _axisRawValue;
        _axisCount = 0;
        _activeJoystick = NULL;
    }
    
    if (joystick) {
        _activeJoystick = joystick;
        if (joystickTransition) {
            _stopCalibration();
        }
        _activeJoystick->setCalibrationMode(false);
        _axisCount = _activeJoystick->axisCount();
        _rgAxisInfo = new struct AxisInfo[_axisCount];
        _axisValueSave = new int[_axisCount];
        _axisRawValue = new int[_axisCount];
        _setInternalCalibrationValuesFromSettings();
        connect(_activeJoystick, &Joystick::rawAxisValueChanged, this, &JoystickConfigController::_axisValueChanged);
    }
}

bool JoystickConfigController::_validAxis(int axis)
{
    return axis >= 0 && axis < _axisCount;
}
