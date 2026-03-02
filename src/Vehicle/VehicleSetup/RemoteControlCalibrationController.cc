#include "RemoteControlCalibrationController.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QSettings>
#include <algorithm>

QGC_LOGGING_CATEGORY(RemoteControlCalibrationControllerLog, "RemoteControl.RemoteControlCalibrationController")
QGC_LOGGING_CATEGORY(RemoteControlCalibrationControllerVerboseLog, "RemoteControl.RemoteControlCalibrationController:verbose")

static constexpr const char *msgBeginThrottleDown = QT_TR_NOOP(
        "* Lower the Throttle stick all the way down as shown in diagram\n"
        "* Please ensure all motor power is disconnected AND all props are removed from the vehicle.\n"
        "* Click Next to continue"
);
static constexpr const char *msgBeginThrottleCenter = QT_TR_NOOP(
    "* Center all sticks as shown in diagram.\n"
    "* Please ensure all motor power is disconnected from the vehicle.\n"
    "* Click Next to continue"
);
static constexpr const char *msgThrottleUp =            QT_TR_NOOP("Move the Throttle stick all the way up and hold it there...");
static constexpr const char *msgThrottleDown =          QT_TR_NOOP("Move the Throttle stick all the way down and leave it there...");
static constexpr const char *msgYawLeft =               QT_TR_NOOP("Move the Yaw stick all the way to the left and hold it there...");
static constexpr const char *msgYawRight =              QT_TR_NOOP("Move the Yaw stick all the way to the right and hold it there...");
static constexpr const char *msgRollLeft =              QT_TR_NOOP("Move the Roll stick all the way to the left and hold it there...");
static constexpr const char *msgRollRight =             QT_TR_NOOP("Move the Roll stick all the way to the right and hold it there...");
static constexpr const char *msgPitchDown =             QT_TR_NOOP("Move the Pitch stick all the way down and hold it there...");
static constexpr const char *msgPitchUp =               QT_TR_NOOP("Move the Pitch stick all the way up and hold it there...");
static constexpr const char *msgPitchCenter =           QT_TR_NOOP("Allow the Pitch stick to move back to center...");
static constexpr const char *msgExtensionHigh =         QT_TR_NOOP("Move the %1 Extension stick to its high value position and hold it there...");
static constexpr const char *msgExtensionLow =          QT_TR_NOOP("Move the %1 Extension stick to its low value position and hold it there...");
static constexpr const char *msgSwitchMinMaxRC =        QT_TR_NOOP("Move all the transmitter switches and/or dials back and forth to their extreme positions.");
static constexpr const char *msgComplete =              QT_TR_NOOP("All settings have been captured. Click Next to write the new parameters to your board.");

RemoteControlCalibrationController::RemoteControlCalibrationController(QObject *parent)
    : FactPanelController(parent)
    , _stateMachine{
        // stickFunction,               stepFunction,                   channelInputFn,                                             nextButtonFn
        { stickFunctionMax,             StateMachineStepStickNeutral,   &RemoteControlCalibrationController::_inputCenterWaitBegin, &RemoteControlCalibrationController::_saveAllTrims },
        { stickFunctionThrottle,        StateMachineStepThrottleUp,     &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionThrottle,        StateMachineStepThrottleDown,   &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionYaw,             StateMachineStepYawRight,       &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionYaw,             StateMachineStepYawLeft,        &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionRoll,            StateMachineStepRollRight,      &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionRoll,            StateMachineStepRollLeft,       &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionPitch,           StateMachineStepPitchUp,        &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionPitch,           StateMachineStepPitchDown,      &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionPitchExtension,  StateMachineStepExtensionHighVert, &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionPitchExtension,  StateMachineStepExtensionLowVert,  &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionRollExtension,   StateMachineStepExtensionHighHorz, &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionRollExtension,   StateMachineStepExtensionLowHorz,  &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionAux1Extension,   StateMachineStepExtensionHighHorz, &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionAux1Extension,   StateMachineStepExtensionLowHorz,  &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionAux2Extension,   StateMachineStepExtensionHighHorz, &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionAux2Extension,   StateMachineStepExtensionLowHorz,  &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionAux3Extension,   StateMachineStepExtensionHighHorz, &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionAux3Extension,   StateMachineStepExtensionLowHorz,  &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionAux4Extension,   StateMachineStepExtensionHighHorz, &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionAux4Extension,   StateMachineStepExtensionLowHorz,  &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionAux5Extension,   StateMachineStepExtensionHighHorz, &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionAux5Extension,   StateMachineStepExtensionLowHorz,  &RemoteControlCalibrationController::_inputStickMin,        nullptr },
        { stickFunctionAux6Extension,   StateMachineStepExtensionHighHorz, &RemoteControlCalibrationController::_inputStickDetect,     nullptr },
        { stickFunctionAux6Extension,   StateMachineStepExtensionLowHorz,  &RemoteControlCalibrationController::_inputStickMin,        nullptr },

        { stickFunctionMax,             StateMachineStepSwitchMinMax,   &RemoteControlCalibrationController::_inputSwitchMinMax,    &RemoteControlCalibrationController::_advanceState },
        { stickFunctionMax,             StateMachineStepComplete,       nullptr,                                                    &RemoteControlCalibrationController::_saveCalibrationValues },
    }
{
    _resetInternalCalibrationValues();
    _loadCalibrationUISettings();

    _stickDisplayPositions = { _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical,
                               _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical };

    if (_vehicle->rover()) {
        _centeredThrottle = true;
    }

    _stepFunctionToMsgStringMap = {
        { StateMachineStepStickNeutral,      msgBeginThrottleCenter }, // Adjusted based on throttle centered or not
        { StateMachineStepThrottleUp,        msgThrottleUp },
        { StateMachineStepThrottleDown,      msgThrottleDown },
        { StateMachineStepYawRight,          msgYawRight },
        { StateMachineStepYawLeft,           msgYawLeft },
        { StateMachineStepRollRight,         msgRollRight },
        { StateMachineStepRollLeft,          msgRollLeft },
        { StateMachineStepPitchUp,           msgPitchUp },
        { StateMachineStepPitchDown,         msgPitchDown },
        { StateMachineStepExtensionHighHorz, msgExtensionHigh },
        { StateMachineStepExtensionLowHorz,  msgExtensionLow },
        { StateMachineStepExtensionHighVert, msgExtensionHigh },
        { StateMachineStepExtensionLowVert,  msgExtensionLow },
        { StateMachineStepPitchCenter,       msgPitchCenter },
        { StateMachineStepSwitchMinMax,      msgSwitchMinMaxRC },
        { StateMachineStepComplete,          msgComplete },
    };

    // Map for throttle centered neutral position
    _bothStickDisplayPositionThrottleCenteredMap = {
        { StateMachineStepStickNeutral, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepThrottleUp, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 2, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 4, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
        }},
        { StateMachineStepThrottleDown, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 2, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 4, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepYawRight, {
            { 1, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 4, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
        }},
        { StateMachineStepYawLeft, {
            { 1, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
        }},
        { StateMachineStepRollRight, {
            { 1, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 2, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 3, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
        }},
        { StateMachineStepRollLeft, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 3, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepPitchUp, {
            { 1, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 3, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
        }},
        { StateMachineStepPitchDown, {
            { 1, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 3, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
        }},
        { StateMachineStepPitchCenter, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepExtensionHighHorz, {
            { 1, { _stickDisplayPositionXRightYCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXRightYCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionXRightYCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXRightYCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepExtensionHighVert, {
            { 1, { _stickDisplayPositionXCenteredYUp, _stickDisplayPositionXCenteredYUp } },
            { 2, { _stickDisplayPositionXCenteredYUp, _stickDisplayPositionXCenteredYUp } },
            { 3, { _stickDisplayPositionXCenteredYUp, _stickDisplayPositionXCenteredYUp } },
            { 4, { _stickDisplayPositionXCenteredYUp, _stickDisplayPositionXCenteredYUp } },
        }},
        { StateMachineStepExtensionLowHorz, {
            { 1, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepExtensionLowVert, {
            { 1, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepSwitchMinMax, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepComplete, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
    };

    // Map for throttle down neutral position
    _bothStickDisplayPositionThrottleDownMap = {
        { StateMachineStepStickNeutral, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 2, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 4, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepThrottleUp, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 2, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 4, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
        }},
        { StateMachineStepThrottleDown, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 2, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 4, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepYawRight, {
            { 1, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 4, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
        }},
        { StateMachineStepYawLeft, {
            { 1, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
        }},
        { StateMachineStepRollRight, {
            { 1, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 2, { _stickDisplayPositionCentered,           _stickDisplayPositionXRightYCentered } },
            { 3, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXRightYCentered,    _stickDisplayPositionCentered } },
        }},
        { StateMachineStepRollLeft, {
            { 1, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXLeftYCentered } },
            { 3, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepPitchUp, {
            { 1, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
            { 3, { _stickDisplayPositionXCenteredYUp,   _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYUp } },
        }},
        { StateMachineStepPitchDown, {
            { 1, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
            { 3, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered,       _stickDisplayPositionXCenteredYDown } },
        }},
        { StateMachineStepPitchCenter, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepExtensionHighHorz, {
            { 1, { _stickDisplayPositionXRightYCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXRightYCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionXRightYCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXRightYCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepExtensionHighVert, {
            { 1, { _stickDisplayPositionXCenteredYUp, _stickDisplayPositionXCenteredYUp } },
            { 2, { _stickDisplayPositionXCenteredYUp, _stickDisplayPositionXCenteredYUp } },
            { 3, { _stickDisplayPositionXCenteredYUp, _stickDisplayPositionXCenteredYUp } },
            { 4, { _stickDisplayPositionXCenteredYUp, _stickDisplayPositionXCenteredYUp } },
        }},
        { StateMachineStepExtensionLowHorz, {
            { 1, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXLeftYCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepExtensionLowVert, {
            { 1, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionXCenteredYDown, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepSwitchMinMax, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
        { StateMachineStepComplete, {
            { 1, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 2, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 3, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
            { 4, { _stickDisplayPositionCentered, _stickDisplayPositionCentered } },
        }},
    };
}

RemoteControlCalibrationController::~RemoteControlCalibrationController()
{
    _saveCalibrationUISettings();

    // qCDebug(RemoteControlCalibrationControllerLog) << Q_FUNC_INFO << this;
}

void RemoteControlCalibrationController::start()
{
    _stopCalibration();
    _readStoredCalibrationValues();
}

const RemoteControlCalibrationController::StateMachineEntry &RemoteControlCalibrationController::_getStateMachineEntry(int step) const
{
    if (step < 0 || step >= _stateMachine.size()) {
        qCWarning(RemoteControlCalibrationControllerLog) << "Bad step value" << step;
        step = 0;
    }

    return _stateMachine[step];
}

void RemoteControlCalibrationController::_advanceState()
{
    _currentStep++;
    if (_currentStep >= _stateMachine.size()) {
        _stopCalibration();
        return;
    }

    _setupCurrentState();
}

void RemoteControlCalibrationController::_setupCurrentState()
{
    auto state = _getStateMachineEntry(_currentStep);

    // If the stick function for this step is not enabled, skip to next step
    if (state.stickFunction != stickFunctionMax && !_stickFunctionEnabled(state.stickFunction)) {
        qCDebug(RemoteControlCalibrationControllerLog) << "Skipping step" << _currentStep << "for disabled stick function" << _stickFunctionToString(state.stickFunction);
        _advanceState();
        return;
    }

    // StateMachineStepSwitchMinMax is only used for RC transmitters
    if (_joystickMode && state.stepFunction == StateMachineStepSwitchMinMax) {
        _advanceState();
        return;
    }

    _stepFunctionToMsgStringMap[StateMachineStepStickNeutral] = _centeredThrottle ? msgBeginThrottleCenter : msgBeginThrottleDown;

    BothSticksDisplayPositions defaultPositions = { _stickDisplayPositionCentered, _stickDisplayPositionCentered };
    BothSticksDisplayPositions bothStickPositions = _centeredThrottle
        ? _bothStickDisplayPositionThrottleCenteredMap.value(state.stepFunction).value(_transmitterMode, defaultPositions)
        : _bothStickDisplayPositionThrottleDownMap.value(state.stepFunction).value(_transmitterMode, defaultPositions);

    QString msg = _stepFunctionToMsgStringMap.value(state.stepFunction, QString());
    if (state.stepFunction == StateMachineStepExtensionHighHorz || state.stepFunction == StateMachineStepExtensionHighVert ||
        state.stepFunction == StateMachineStepExtensionLowHorz  || state.stepFunction == StateMachineStepExtensionLowVert) {
        static const QMap <StickFunction, QString> extensionNameMap = {
            { stickFunctionPitchExtension,  QT_TR_NOOP("Pitch") },
            { stickFunctionRollExtension,   QT_TR_NOOP("Roll") },
            { stickFunctionAux1Extension,   QT_TR_NOOP("Aux 1") },
            { stickFunctionAux2Extension,   QT_TR_NOOP("Aux 2") },
            { stickFunctionAux3Extension,   QT_TR_NOOP("Aux 3") },
            { stickFunctionAux4Extension,   QT_TR_NOOP("Aux 4") },
            { stickFunctionAux5Extension,   QT_TR_NOOP("Aux 5") },
            { stickFunctionAux6Extension,   QT_TR_NOOP("Aux 6") },
        };
        msg = msg.arg(extensionNameMap.value(state.stickFunction, QT_TR_NOOP("Unknown")));
    }

    _setSingleStickDisplay(state.stepFunction == StateMachineStepExtensionHighHorz || state.stepFunction == StateMachineStepExtensionHighVert ||
                           state.stepFunction == StateMachineStepExtensionLowHorz  || state.stepFunction == StateMachineStepExtensionLowVert);

    _statusText->setProperty("text", msg);
    _stickDisplayPositions = { bothStickPositions.leftStick.horizontal, bothStickPositions.leftStick.vertical,
                               bothStickPositions.rightStick.horizontal, bothStickPositions.rightStick.vertical };
    emit stickDisplayPositionsChanged();

    _stickDetectChannel = _chanMax;
    _stickDetectSettleStarted = false;

    _saveCurrentRawValues();

    _nextButton->setEnabled(state.nextButtonFn != nullptr);
}

void RemoteControlCalibrationController::rawChannelValuesChanged(QVector<int> channelValues)
{
    auto channelCount = channelValues.size();
    if (channelCount > _chanMax) {
        qCWarning(RemoteControlCalibrationControllerLog) << "Too many channels:" << channelCount << ", max is" << _chanMax;
        channelCount = _chanMax;
    }

    qCDebug(RemoteControlCalibrationControllerVerboseLog) << "channelValues" << channelValues;

    for (int channel=0; channel<channelCount; channel++) {
        const int channelValue = channelValues[channel];
        const ChannelInfo &channelInfo = _rgChannelInfo[channel];
        const int adjustedValue = _adjustChannelRawValue(channelInfo, channelValue);

        _channelRawValue[channel] = channelValue;
        emit rawChannelValueChanged(channel, channelValue);

        // Signal attitude rc values to Qml if mapped
        if (channelInfo.stickFunction != stickFunctionMax) {
            switch (channelInfo.stickFunction) {
            case stickFunctionRoll:
                emit adjustedRollChannelValueChanged(adjustedValue);
                break;
            case stickFunctionPitch:
                emit adjustedPitchChannelValueChanged(adjustedValue);
                break;
            case stickFunctionYaw:
                emit adjustedYawChannelValueChanged(adjustedValue);
                break;
            case stickFunctionThrottle:
                emit adjustedThrottleChannelValueChanged(adjustedValue);
                break;
            case stickFunctionRollExtension:
                emit adjustedRollExtensionChannelValueChanged(adjustedValue);
                break;
            case stickFunctionPitchExtension:
                emit adjustedPitchExtensionChannelValueChanged(adjustedValue);
                break;
            case stickFunctionAux1Extension:
                emit adjustedAux1ExtensionChannelValueChanged(adjustedValue);
                break;
            case stickFunctionAux2Extension:
                emit adjustedAux2ExtensionChannelValueChanged(adjustedValue);
                break;
            case stickFunctionAux3Extension:
                emit adjustedAux3ExtensionChannelValueChanged(adjustedValue);
                break;
            case stickFunctionAux4Extension:
                emit adjustedAux4ExtensionChannelValueChanged(adjustedValue);
                break;
            case stickFunctionAux5Extension:
                emit adjustedAux5ExtensionChannelValueChanged(adjustedValue);
                break;
            case stickFunctionAux6Extension:
                emit adjustedAux6ExtensionChannelValueChanged(adjustedValue);
                break;
            default:
                break;
            }
        }

        if (_currentStep == -1) {
            if (_chanCount != channelCount) {
                _chanCount = channelCount;
                emit channelCountChanged(_chanCount);
            }
        } else {
            auto state = _getStateMachineEntry(_currentStep);
            if (state.channelInputFn) {
                (this->*state.channelInputFn)(state.stickFunction, channel, channelValue);
            }
        }
    }
}

void RemoteControlCalibrationController::nextButtonClicked()
{
    if (_currentStep == -1) {
        // Need to have enough channels
        if (_chanCount < _chanMinimum) {
            qgcApp()->showAppMessage(QStringLiteral("Detected %1 channels. To operate vehicle, you need at least %2 channels.").arg(_chanCount).arg(_chanMinimum));
            return;
        }
        _startCalibration();
    } else {
        auto state = _getStateMachineEntry(_currentStep);
        if (state.nextButtonFn) {
            (this->*state.nextButtonFn)();
        }
    }
}

void RemoteControlCalibrationController::cancelButtonClicked()
{
    _stopCalibration();
}

void RemoteControlCalibrationController::_saveAllTrims()
{
    // We save all trims as the first step. At this point no channels are mapped but it should still
    // allow us to get good trims for the roll/pitch/yaw/throttle even though we don't know which
    // channels they are yet. AS we continue through the process the other channels will get their
    // trims reset to correct values.

    for (int i=0; i<_chanCount; i++) {
        qCDebug(RemoteControlCalibrationControllerLog) << "_saveAllTrims channel:trim" << i << _channelRawValue[i];
        _rgChannelInfo[i].channelTrim = _channelRawValue[i];
    }
    _advanceState();
}

void RemoteControlCalibrationController::_inputCenterWaitBegin(StickFunction /*stickFunction*/, int channel, int value)
{
    if (_joystickMode) {
        // Track deadband adjustments in joystick mode
        int newDeadband = abs(value) * 1.1; // add 10% on top for fudge factor
        if (newDeadband > _rgChannelInfo[channel].deadband) {
            _rgChannelInfo[channel].deadband = qMin(newDeadband, _calValidMaxValue  );
            qCDebug(RemoteControlCalibrationControllerLog) << "Channel:" << channel << "Deadband:" << _rgChannelInfo[channel].deadband;
            StickFunction stickFunction = _rgChannelInfo[channel].stickFunction;
            if (stickFunction != stickFunctionMax) {
                _emitDeadbandChanged(stickFunction);
            }
        }
    }

    _nextButton->setEnabled(true);
}

bool RemoteControlCalibrationController::_stickSettleComplete(int value)
{
    // We are waiting for the stick to settle out to a max position

    if (abs(_stickDetectValue - value) > _calSettleDelta) {
        // Stick is moving too much to consider stopped

        qCDebug(RemoteControlCalibrationControllerLog) << "Still moving, _stickDetectValue:value" << _stickDetectValue << value;

        _stickDetectValue = value;
        _stickDetectSettleStarted = false;
    } else {
        // Stick is still positioned within the specified small range

        if (_stickDetectSettleStarted) {
            // We have already started waiting

            if (_stickDetectSettleElapsed.elapsed() > _stickDetectSettleMSecs) {
                // Stick has stayed positioned in one place long enough, detection is complete.
                return true;
            }
        } else {
            // Start waiting for the stick to stay settled for _stickDetectSettleWaitMSecs msecs

            qCDebug(RemoteControlCalibrationControllerLog) << "Starting settle timer, _stickDetectValue:value" << _stickDetectValue << value;

            _stickDetectSettleStarted = true;
            _stickDetectSettleElapsed.start();
        }
    }

    return false;
}

void RemoteControlCalibrationController::_inputStickDetect(StickFunction stickFunction, int channel, int value)
{
    // If this channel is already used in a mapping we can't use it again
    if (_rgChannelInfo[channel].stickFunction != stickFunctionMax) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerVerboseLog) << "_inputStickDetect function:channel:value" << _stickFunctionToString(stickFunction) << channel << value;

    if (_stickDetectChannel == _chanMax) {
        // We have not detected enough movement on a channel yet
        // Note: We intentionally require movement here (not just being at extreme) because
        // this function is called for ALL channels, and some (like triggers) may rest at
        // extreme positions. Requiring movement ensures we detect the correct channel.

        if (abs(_channelValueSave[channel] - value) > _calMoveDelta) {
            // Stick has moved far enough to consider it as being selected for the function

            qCDebug(RemoteControlCalibrationControllerLog) << "Starting settle wait - function:channel" << _stickFunctionToString(stickFunction) << channel;

            // Setup up to detect stick being pegged to min or max value
            _stickDetectChannel = channel;
            _stickDetectValue = value;
        }
    } else if (channel == _stickDetectChannel) {
        if (_stickSettleComplete(value)) {
            ChannelInfo *const info = &_rgChannelInfo[channel];

            // Map the channel to the function
            _rgFunctionChannelMapping[stickFunction] = channel;
            info->stickFunction = stickFunction;

            // A non-reversed channel should show a higher value than center.
            info->channelReversed = value < _channelValueSave[channel];
            if (info->channelReversed) {
                _rgChannelInfo[channel].channelMin = value;
            } else {
                _rgChannelInfo[channel].channelMax = value;
            }

            qCDebug(RemoteControlCalibrationControllerLog) << "Stick detected - function:channel:reversed:" << _stickFunctionToString(stickFunction) << channel << info->channelReversed;

            _signalAllAttitudeValueChanges();

            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputStickMin(StickFunction stickFunction, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_rgFunctionChannelMapping[stickFunction] != channel) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerVerboseLog) << "_inputStickMin function:channel:value" << _stickFunctionToString(stickFunction) << channel << value;

    if (_stickDetectChannel == _chanMax) {
        if (_rgChannelInfo[channel].channelReversed) {
            if (value > _calCenterPoint + _calMoveDelta) {
                qCDebug(RemoteControlCalibrationControllerLog) << "Movement detected, starting settle wait";
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        } else {
            if (value < _calCenterPoint - _calMoveDelta) {
                qCDebug(RemoteControlCalibrationControllerLog) << "Movement detected, starting settle wait";
                _stickDetectChannel = channel;
                _stickDetectValue = value;
            }
        }
    } else {
        // We are waiting for the selected channel to settle out
        if (_stickSettleComplete(value)) {
            auto& channelInfo = _rgChannelInfo[channel];

            // Stick detection is complete. Stick should be at extreme position.
            if (channelInfo.channelReversed) {
                channelInfo.channelMax = value;
            } else {
                channelInfo.channelMin = value;
            }

            // Check if this is throttle and set trim accordingly
            if (!_joystickMode && stickFunction == stickFunctionThrottle) {
                channelInfo.channelTrim = value;
            }

            qCDebug(RemoteControlCalibrationControllerLog) << "Settle complete - function:channel:min:max:trim" << _stickFunctionToString(stickFunction) << channel << channelInfo.channelMin << channelInfo.channelMax << channelInfo.channelTrim;

            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputCenterWait(StickFunction stickFunction, int channel, int value)
{
    // We only care about the channel mapped to the function we are working on
    if (_rgFunctionChannelMapping[stickFunction] != channel) {
        return;
    }

    qCDebug(RemoteControlCalibrationControllerLog) << "_inputCenterWait function:channel:value" << _stickFunctionToString(stickFunction) << channel << value;
    if (_stickDetectChannel == _chanMax) {
        // Sticks have not yet moved close enough to center

        if (abs(_calCenterPoint - value) < _calRoughCenterDelta) {
            // Stick has moved close enough to center that we can start waiting for it to settle
            qCDebug(RemoteControlCalibrationControllerLog) << "_inputCenterWait Center detected. Waiting for settle.";
            _stickDetectChannel = channel;
            _stickDetectValue = value;
        }
    } else {
        if (_stickSettleComplete(value)) {
            _advanceState();
        }
    }
}

void RemoteControlCalibrationController::_inputSwitchMinMax(StickFunction /*stickFunction*/, int channel, int value)
{
    // If the channel is mapped we already have min/max
    if (_rgChannelInfo[channel].stickFunction != stickFunctionMax) {
        return;
    }

    if (abs(_calCenterPoint - value) > _calMoveDelta) {
        // Stick has moved far enough from center to consider for min/max
        if (value < _calCenterPoint) {
            const int minValue = qMin(_rgChannelInfo[channel].channelMin, value);

            qCDebug(RemoteControlCalibrationControllerLog) << "setting min channel:min" << channel << minValue;

            _rgChannelInfo[channel].channelMin = minValue;
        } else {
            int maxValue = qMax(_rgChannelInfo[channel].channelMax, value);

            qCDebug(RemoteControlCalibrationControllerLog) << "setting max channel:max" << channel << maxValue;

            _rgChannelInfo[channel].channelMax = maxValue;
        }
    }
}

void RemoteControlCalibrationController::_resetInternalCalibrationValues()
{
    // Set all raw channels to not reversed and center point values
    for (int i = 0; i < _chanMax; i++) {
        ChannelInfo *const info = &_rgChannelInfo[i];
        info->stickFunction = stickFunctionMax;
        info->channelReversed = false;
        info->channelMin = RemoteControlCalibrationController::_calCenterPoint;
        info->channelMax = RemoteControlCalibrationController::_calCenterPoint;
        info->channelTrim = RemoteControlCalibrationController::_calCenterPoint;
        info->deadband = 0;
    }

    // Initialize attitude function mapping to function channel not set
    for (size_t i = 0; i < stickFunctionMax; i++) {
        _rgFunctionChannelMapping[i] = _chanMax;
    }

    _signalAllAttitudeValueChanges();
}

void RemoteControlCalibrationController::_validateAndAdjustCalibrationValues()
{
    for (int chan = 0; chan<_chanMax; chan++) {
        auto& channelInfo = _rgChannelInfo[chan];

        if (chan < _chanCount) {
            // Validate Min/Max values. Although the channel appears as available we still may
            // not have good min/max/trim values for it. Set to defaults if needed.
            if (channelInfo.channelMin > _calValidMinValue || channelInfo.channelMax < _calValidMaxValue) {
                qCDebug(RemoteControlCalibrationControllerLog) << "resetting channel invalid min/max - chan:channelMin:calValidMinValue:channelMax:calValidMaxValue"
                    << chan << channelInfo.channelMin << _calValidMinValue << channelInfo.channelMax << _calValidMaxValue;
                channelInfo.channelMin = _calDefaultMinValue;
                channelInfo.channelMax = _calDefaultMaxValue;
                channelInfo.channelTrim = channelInfo.channelMin + ((channelInfo.channelMax - channelInfo.channelMin) / 2);
            } else {
                switch (channelInfo.stickFunction) {
                case stickFunctionThrottle:
                case stickFunctionYaw:
                case stickFunctionRoll:
                case stickFunctionPitch:
                    // Make sure trim is within min/max
                    channelInfo.channelTrim = std::clamp(channelInfo.channelTrim, channelInfo.channelMin, channelInfo.channelMax);
                    break;
                default:
                    // Non-attitude control channels have calculated trim
                    channelInfo.channelTrim = channelInfo.channelMin + ((channelInfo.channelMax - channelInfo.channelMin) / 2);
                    break;
                }

            }
        } else {
            // Unavailable channels are set to defaults
            qCDebug(RemoteControlCalibrationControllerLog) << "resetting unavailable channel" << chan;
            channelInfo.channelMin = _calDefaultMinValue;
            channelInfo.channelMax = _calDefaultMaxValue;
            channelInfo.channelTrim = channelInfo.channelMin + ((channelInfo.channelMax - channelInfo.channelMin) / 2);
            channelInfo.channelReversed = false;
            channelInfo.deadband = 0;
            channelInfo.stickFunction = stickFunctionMax;

        }
    }
}

void RemoteControlCalibrationController::_startCalibration()
{
    if (_chanCount < _chanMinimum) {
        qCWarning(RemoteControlCalibrationControllerLog) << "Call to RemoteControlCalibrationController::_startCalibration with _chanCount < _chanMinimum";
        return;
    }

    _resetInternalCalibrationValues();

    if (!_calibrating) {
        _calibrating = true;
        emit calibratingChanged(true);
    }

    _nextButton->setProperty("text", tr("Next"));
    _cancelButton->setEnabled(true);

    _currentStep = 0;
    _setupCurrentState();
}

void RemoteControlCalibrationController::_setSingleStickDisplay(bool singleStickDisplay)
{
    if (_singleStickDisplay != singleStickDisplay) {
        _singleStickDisplay = singleStickDisplay;
        emit singleStickDisplayChanged(singleStickDisplay);
    }
}

void RemoteControlCalibrationController::_stopCalibration()
{
    _currentStep = -1;

    if (_vehicle) {
        _readStoredCalibrationValues();
    }

    if (_calibrating) {
        _calibrating = false;
        emit calibratingChanged(false);
    }

    if (_statusText) {
        _statusText->setProperty("text", "");
    }
    if (_nextButton) {
        _nextButton->setProperty("text", tr("Calibrate"));
    }
    if (_nextButton) {
        _nextButton->setEnabled(true);
    }
    if (_cancelButton) {
        _cancelButton->setEnabled(false);
    }

    _stickDisplayPositions = { _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical,
                               _stickDisplayPositionCentered.horizontal, _stickDisplayPositionCentered.vertical };
    emit stickDisplayPositionsChanged();
}

void RemoteControlCalibrationController::_saveCurrentRawValues()
{
    for (int i = 0; i < _chanMax; i++) {
        _channelValueSave[i] = _channelRawValue[i];
        qCDebug(RemoteControlCalibrationControllerVerboseLog) << "_saveCurrentRawValues channel:value" << i << _channelValueSave[i];
    }
}

/// Adjust raw channel value for reversal if needed
int RemoteControlCalibrationController::_adjustChannelRawValue(const ChannelInfo& info, int rawValue) const
{
    if (!info.channelReversed) {
        return rawValue;
    }

    const int invertedValue = info.channelMin + info.channelMax - rawValue;
    return std::clamp(invertedValue, info.channelMin, info.channelMax);
}

void RemoteControlCalibrationController::_loadCalibrationUISettings()
{
    QSettings settings;

    settings.beginGroup(_settingsGroup);
    _transmitterMode = settings.value(_settingsKeyTransmitterMode, 2).toInt();
    settings.endGroup();

    if (_transmitterMode < 1 || _transmitterMode > 4) {
        _transmitterMode = 2;
    }
}

void RemoteControlCalibrationController::_saveCalibrationUISettings()
{
    QSettings settings;

    settings.beginGroup(_settingsGroup);
    settings.setValue(_settingsKeyTransmitterMode, _transmitterMode);
    settings.endGroup();
}

int RemoteControlCalibrationController::adjustedRollChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionRoll];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedPitchChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionPitch];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedYawChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionYaw];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedThrottleChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionThrottle];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedRollExtensionChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionRollExtension];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedPitchExtensionChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionPitchExtension];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedAux1ExtensionChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionAux1Extension];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedAux2ExtensionChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionAux2Extension];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedAux3ExtensionChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionAux3Extension];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedAux4ExtensionChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionAux4Extension];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedAux5ExtensionChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionAux5Extension];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

int RemoteControlCalibrationController::adjustedAux6ExtensionChannelValue()
{
    int channel = _rgFunctionChannelMapping[stickFunctionAux6Extension];
    if (channel != _chanMax) {
        return _adjustChannelRawValue(_rgChannelInfo[channel], _channelRawValue[channel]);
    } else {
        return _calCenterPoint;
    }
}

bool RemoteControlCalibrationController::rollChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionRoll] != _chanMax);
}

bool RemoteControlCalibrationController::pitchChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionPitch] != _chanMax);
}

bool RemoteControlCalibrationController::rollExtensionChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionRollExtension] != _chanMax);
}

bool RemoteControlCalibrationController::pitchExtensionChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionPitchExtension] != _chanMax);
}

bool RemoteControlCalibrationController::aux1ExtensionChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionAux1Extension] != _chanMax);
}

bool RemoteControlCalibrationController::aux2ExtensionChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionAux2Extension] != _chanMax);
}

bool RemoteControlCalibrationController::aux3ExtensionChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionAux3Extension] != _chanMax);
}

bool RemoteControlCalibrationController::aux4ExtensionChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionAux4Extension] != _chanMax);
}

bool RemoteControlCalibrationController::aux5ExtensionChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionAux5Extension] != _chanMax);
}

bool RemoteControlCalibrationController::aux6ExtensionChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionAux6Extension] != _chanMax);
}

bool RemoteControlCalibrationController::yawChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionYaw] != _chanMax);
}

bool RemoteControlCalibrationController::throttleChannelMapped()
{
    return (_rgFunctionChannelMapping[stickFunctionThrottle] != _chanMax);
}

bool RemoteControlCalibrationController::pitchExtensionEnabled()
{
    return _stickFunctionEnabled(stickFunctionPitchExtension);
}

bool RemoteControlCalibrationController::rollExtensionEnabled()
{
    return _stickFunctionEnabled(stickFunctionRollExtension);
}

bool RemoteControlCalibrationController::aux1ExtensionEnabled()
{
    return _stickFunctionEnabled(stickFunctionAux1Extension);
}

bool RemoteControlCalibrationController::aux2ExtensionEnabled()
{
    return _stickFunctionEnabled(stickFunctionAux2Extension);
}

bool RemoteControlCalibrationController::aux3ExtensionEnabled()
{
    return _stickFunctionEnabled(stickFunctionAux3Extension);
}

bool RemoteControlCalibrationController::aux4ExtensionEnabled()
{
    return _stickFunctionEnabled(stickFunctionAux4Extension);
}

bool RemoteControlCalibrationController::aux5ExtensionEnabled()
{
    return _stickFunctionEnabled(stickFunctionAux5Extension);
}

bool RemoteControlCalibrationController::aux6ExtensionEnabled()
{
    return _stickFunctionEnabled(stickFunctionAux6Extension);
}

bool RemoteControlCalibrationController::anyExtensionEnabled()
{
    return pitchExtensionEnabled() || rollExtensionEnabled() ||
           aux1ExtensionEnabled() || aux2ExtensionEnabled() ||
           aux3ExtensionEnabled() || aux4ExtensionEnabled() ||
           aux5ExtensionEnabled() || aux6ExtensionEnabled();
}

bool RemoteControlCalibrationController::rollChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionRoll] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionRoll]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::pitchChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionPitch] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionPitch]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::rollExtensionChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionRollExtension] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionRollExtension]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::pitchExtensionChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionPitchExtension] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionPitchExtension]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::aux1ExtensionChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionAux1Extension] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionAux1Extension]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::aux2ExtensionChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionAux2Extension] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionAux2Extension]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::aux3ExtensionChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionAux3Extension] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionAux3Extension]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::aux4ExtensionChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionAux4Extension] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionAux4Extension]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::aux5ExtensionChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionAux5Extension] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionAux5Extension]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::aux6ExtensionChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionAux6Extension] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionAux6Extension]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::yawChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionYaw] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionYaw]].channelReversed;
    } else {
        return false;
    }
}

bool RemoteControlCalibrationController::throttleChannelReversed()
{
    if (_rgFunctionChannelMapping[stickFunctionThrottle] != _chanMax) {
        return _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionThrottle]].channelReversed;
    } else {
        return false;
    }
}

void RemoteControlCalibrationController::setTransmitterMode(int mode)
{
    if (mode < 1 || mode > 4) {
        qCWarning(RemoteControlCalibrationControllerLog) << "Invalid transmitter mode set:" << mode;
        mode = 2;
    }
    if (_transmitterMode != mode) {
        _transmitterMode = mode;
        emit transmitterModeChanged();
    }
}

void RemoteControlCalibrationController::_signalAllAttitudeValueChanges()
{
    emit rollChannelMappedChanged(rollChannelMapped());
    emit pitchChannelMappedChanged(pitchChannelMapped());
    emit rollExtensionChannelMappedChanged(rollExtensionChannelMapped());
    emit pitchExtensionChannelMappedChanged(pitchExtensionChannelMapped());
    emit aux1ExtensionChannelMappedChanged(aux1ExtensionChannelMapped());
    emit aux2ExtensionChannelMappedChanged(aux2ExtensionChannelMapped());
    emit aux3ExtensionChannelMappedChanged(aux3ExtensionChannelMapped());
    emit aux4ExtensionChannelMappedChanged(aux4ExtensionChannelMapped());
    emit aux5ExtensionChannelMappedChanged(aux5ExtensionChannelMapped());
    emit aux6ExtensionChannelMappedChanged(aux6ExtensionChannelMapped());
    emit yawChannelMappedChanged(yawChannelMapped());
    emit throttleChannelMappedChanged(throttleChannelMapped());

    emit rollChannelReversedChanged(rollChannelReversed());
    emit pitchChannelReversedChanged(pitchChannelReversed());
    emit rollExtensionChannelReversedChanged(rollExtensionChannelReversed());
    emit pitchExtensionChannelReversedChanged(pitchExtensionChannelReversed());
    emit aux1ExtensionChannelReversedChanged(aux1ExtensionChannelReversed());
    emit aux2ExtensionChannelReversedChanged(aux2ExtensionChannelReversed());
    emit aux3ExtensionChannelReversedChanged(aux3ExtensionChannelReversed());
    emit aux4ExtensionChannelReversedChanged(aux4ExtensionChannelReversed());
    emit aux5ExtensionChannelReversedChanged(aux5ExtensionChannelReversed());
    emit aux6ExtensionChannelReversedChanged(aux6ExtensionChannelReversed());
    emit yawChannelReversedChanged(yawChannelReversed());
    emit throttleChannelReversedChanged(throttleChannelReversed());

    _emitDeadbandChanged(stickFunctionRoll);
    _emitDeadbandChanged(stickFunctionPitch);
    _emitDeadbandChanged(stickFunctionRollExtension);
    _emitDeadbandChanged(stickFunctionPitchExtension);
    _emitDeadbandChanged(stickFunctionAux1Extension);
    _emitDeadbandChanged(stickFunctionAux2Extension);
    _emitDeadbandChanged(stickFunctionAux3Extension);
    _emitDeadbandChanged(stickFunctionAux4Extension);
    _emitDeadbandChanged(stickFunctionAux5Extension);
    _emitDeadbandChanged(stickFunctionAux6Extension);
    _emitDeadbandChanged(stickFunctionYaw);
    _emitDeadbandChanged(stickFunctionThrottle);
}

int RemoteControlCalibrationController::rollDeadband()
{
    return _deadbandForFunction(stickFunctionRoll);
}

int RemoteControlCalibrationController::pitchDeadband()
{
    return _deadbandForFunction(stickFunctionPitch);
}

int RemoteControlCalibrationController::rollExtensionDeadband()
{
    return _deadbandForFunction(stickFunctionRollExtension);
}

int RemoteControlCalibrationController::pitchExtensionDeadband()
{
    return _deadbandForFunction(stickFunctionPitchExtension);
}

int RemoteControlCalibrationController::aux1ExtensionDeadband()
{
    return _deadbandForFunction(stickFunctionAux1Extension);
}

int RemoteControlCalibrationController::aux2ExtensionDeadband()
{
    return _deadbandForFunction(stickFunctionAux2Extension);
}

int RemoteControlCalibrationController::aux3ExtensionDeadband()
{
    return _deadbandForFunction(stickFunctionAux3Extension);
}

int RemoteControlCalibrationController::aux4ExtensionDeadband()
{
    return _deadbandForFunction(stickFunctionAux4Extension);
}

int RemoteControlCalibrationController::aux5ExtensionDeadband()
{
    return _deadbandForFunction(stickFunctionAux5Extension);
}

int RemoteControlCalibrationController::aux6ExtensionDeadband()
{
    return _deadbandForFunction(stickFunctionAux6Extension);
}

int RemoteControlCalibrationController::yawDeadband()
{
    return _deadbandForFunction(stickFunctionYaw);
}

int RemoteControlCalibrationController::throttleDeadband()
{
    return _deadbandForFunction(stickFunctionThrottle);
}

void RemoteControlCalibrationController::copyTrims()
{
    _vehicle->startCalibration(QGCMAVLink::CalibrationCopyTrims);
}

QString RemoteControlCalibrationController::_stickFunctionToString(StickFunction stickFunction)
{
    switch (stickFunction) {
    case stickFunctionRoll:
        return tr("Roll");
    case stickFunctionPitch:
        return tr("Pitch");
    case stickFunctionYaw:
        return tr("Yaw");
    case stickFunctionThrottle:
        return tr("Throttle");
    case stickFunctionAux1Extension:
        return tr("Aux1 Extension");
    case stickFunctionAux2Extension:
        return tr("Aux2 Extension");
    case stickFunctionAux3Extension:
        return tr("Aux3 Extension");
    case stickFunctionAux4Extension:
        return tr("Aux4 Extension");
    case stickFunctionAux5Extension:
        return tr("Aux5 Extension");
    case stickFunctionAux6Extension:
        return tr("Aux6 Extension");
    case stickFunctionPitchExtension:
        return tr("Pitch Extension");
    case stickFunctionRollExtension:
        return tr("Roll Extension");
    default:
        return tr("Unknown");
    }
}

void RemoteControlCalibrationController::setCenteredThrottle(bool centered)
{
    if (_centeredThrottle != centered) {
        _centeredThrottle = centered;
        emit centeredThrottleChanged(centered);
    }
}

void RemoteControlCalibrationController::setJoystickMode(bool joystickMode)
{
    if (_joystickMode == joystickMode) {
        return;
    }

    _joystickMode = joystickMode;
    setCenteredThrottle(joystickMode);
    emit joystickModeChanged(_joystickMode);
}

int RemoteControlCalibrationController::_deadbandForFunction(StickFunction stickFunction) const
{
    if (stickFunction < 0 || stickFunction >= stickFunctionMax) {
        return 0;
    }

    int channel = _rgFunctionChannelMapping[stickFunction];
    if (channel != _chanMax) {
        return _rgChannelInfo[channel].deadband;
    }

    return 0;
}

void RemoteControlCalibrationController::_emitDeadbandChanged(StickFunction stickFunction)
{
    const int deadband = _deadbandForFunction(stickFunction);
    switch (stickFunction) {
    case stickFunctionRoll:
        emit rollDeadbandChanged(deadband);
        break;
    case stickFunctionPitch:
        emit pitchDeadbandChanged(deadband);
        break;
    case stickFunctionYaw:
        emit yawDeadbandChanged(deadband);
        break;
    case stickFunctionThrottle:
        emit throttleDeadbandChanged(deadband);
        break;
    default:
        break;
    }
}

void RemoteControlCalibrationController::_saveCalibrationValues()
{
    _saveStoredCalibrationValues();
    emit calibrationCompleted();
    _advanceState();
}

bool RemoteControlCalibrationController::_stickFunctionEnabled(StickFunction stickFunction)
{
    // By default only calibrate attitude control functions
    switch (stickFunction) {
    case stickFunctionRoll:
    case stickFunctionPitch:
    case stickFunctionYaw:
    case stickFunctionThrottle:
        return true;
    default:
        return false;
    }
}
