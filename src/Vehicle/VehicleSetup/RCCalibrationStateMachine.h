#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>

class RemoteControlCalibrationController;

/// State machine for RC/Joystick calibration workflow.
///
/// States:
/// - Idle: Waiting for calibration to start
/// - StickNeutral: Wait for sticks centered (or throttle down)
/// - ThrottleUp: Detect throttle max
/// - ThrottleDown: Detect throttle min
/// - YawRight: Detect yaw max
/// - YawLeft: Detect yaw min
/// - RollRight: Detect roll max
/// - RollLeft: Detect roll min
/// - PitchUp: Detect pitch max
/// - PitchDown: Detect pitch min
/// - PitchCenter: Wait for pitch to return to center
/// - SwitchMinMax: Capture min/max for all switches
/// - Complete: Calibration finished
class RCCalibrationStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    /// Stick function enumeration (matching controller)
    enum StickFunction {
        stickFunctionRoll,
        stickFunctionPitch,
        stickFunctionYaw,
        stickFunctionThrottle,
        stickFunctionMax,
    };

    explicit RCCalibrationStateMachine(RemoteControlCalibrationController* controller, QObject* parent = nullptr);
    ~RCCalibrationStateMachine() override = default;

    /// Start the calibration workflow
    void startCalibration();

    /// Cancel the calibration workflow
    void cancelCalibration();

    /// Handle next button press from UI
    void nextButtonPressed();

    /// Process channel input from controller
    /// @param channel Channel index
    /// @param value Raw channel value
    void processChannelInput(int channel, int value);

    /// @return true if calibration is in progress
    bool isCalibrating() const;

signals:
    void calibrationStarted();
    void calibrationCancelled();
    void calibrationComplete();
    void stepChanged();

private:
    void _buildStateMachine();
    void _setupTransitions();

    // Input processing methods (moved from controller)
    void _inputCenterWaitBegin(StickFunction stickFunction, int channel, int value);
    void _inputStickDetect(StickFunction stickFunction, int channel, int value);
    void _inputStickMin(StickFunction stickFunction, int channel, int value);
    void _inputCenterWait(StickFunction stickFunction, int channel, int value);
    void _inputSwitchMinMax(StickFunction stickFunction, int channel, int value);

    /// Check if stick has settled at a position
    bool _stickSettleComplete(int value);

    /// Advance to the next state after stick detection
    void _advanceState();

    /// Save trims for all channels
    void _saveAllTrims();

    /// Save calibration values and complete
    void _saveCalibrationValues();

    RemoteControlCalibrationController* _controller;

    // Stick detection state
    int _stickDetectChannel;
    int _stickDetectValue;
    bool _stickDetectSettleStarted;
    QElapsedTimer _stickDetectSettleElapsed;

    // States (owned by state machine)
    QGCState* _idleState = nullptr;
    QGCState* _stickNeutralState = nullptr;
    QGCState* _throttleUpState = nullptr;
    QGCState* _throttleDownState = nullptr;
    QGCState* _yawRightState = nullptr;
    QGCState* _yawLeftState = nullptr;
    QGCState* _rollRightState = nullptr;
    QGCState* _rollLeftState = nullptr;
    QGCState* _pitchUpState = nullptr;
    QGCState* _pitchDownState = nullptr;
    QGCState* _pitchCenterState = nullptr;
    QGCState* _switchMinMaxState = nullptr;
    QGCFinalState* _completeState = nullptr;

    // Current state info for input routing
    StickFunction _currentStickFunction = stickFunctionMax;
    enum class InputMode {
        None,
        CenterWaitBegin,
        StickDetect,
        StickMin,
        CenterWait,
        SwitchMinMax
    };
    InputMode _currentInputMode = InputMode::None;
};
