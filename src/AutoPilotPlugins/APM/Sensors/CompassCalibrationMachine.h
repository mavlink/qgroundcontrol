#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(CompassCalibrationMachineLog)

class APMSensorsComponentController;

/// State machine for APM onboard compass calibration workflow.
///
/// States:
/// - Idle: Waiting for calibration to start
/// - CheckingSupport: Checking if onboard mag cal is supported
/// - Setup: Determining which compasses to calibrate and setting up parameters
/// - Rotating: Active calibration - user rotates vehicle
/// - Success: All compasses calibrated successfully (final)
/// - Failed: Calibration failed (final)
///
/// The workflow uses MAVLink MAG_CAL_PROGRESS and MAG_CAL_REPORT messages.
class CompassCalibrationMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    static constexpr int MaxCompasses = 3;

    explicit CompassCalibrationMachine(APMSensorsComponentController* controller, QObject* parent = nullptr);
    ~CompassCalibrationMachine() override = default;

    /// Start the compass calibration workflow
    void startCalibration();

    /// Cancel the calibration workflow
    void cancelCalibration();

    /// Handle compass calibration support check result
    /// @param supported true if onboard compass cal is supported
    void handleSupportCheckResult(bool supported);

    /// Handle MAG_CAL_PROGRESS message
    /// @param compassId The compass index (0-2)
    /// @param calMask Bitmask of compasses being calibrated
    /// @param completionPct Completion percentage (0-100)
    void handleMagCalProgress(uint8_t compassId, uint8_t calMask, uint8_t completionPct);

    /// Handle MAG_CAL_REPORT message
    /// @param compassId The compass index (0-2)
    /// @param calStatus Calibration status (MAG_CAL_SUCCESS, etc.)
    /// @param fitness Calibration fitness value
    void handleMagCalReport(uint8_t compassId, uint8_t calStatus, float fitness);

    /// @return true if calibration is in progress
    bool isCalibrating() const;

    /// @return Current compass calibration mask
    uint8_t compassMask() const { return _compassMask; }

    /// Get progress for a specific compass (0-100)
    uint8_t compassProgress(int compassId) const;

    /// Check if a specific compass calibration is complete
    bool compassComplete(int compassId) const;

    /// Check if a specific compass calibration succeeded
    bool compassSucceeded(int compassId) const;

    /// Get fitness for a specific compass
    float compassFitness(int compassId) const;

signals:
    void calibrationStarted();
    void calibrationCancelled();
    void calibrationComplete(bool allSucceeded);
    void progressChanged(int compassId, uint8_t progress);
    void compassCalComplete(int compassId, bool succeeded, float fitness);

private:
    void _buildStateMachine();
    void _setupTransitions();
    void _determineCompassesToCalibrate();
    void _startMagCal();
    void _restoreCompassCalFitness();
    void _checkAllCompassesComplete();
    void _updateProgressBar();

    APMSensorsComponentController* _controller;

    // States
    QGCState* _idleState = nullptr;
    QGCState* _checkingSupportState = nullptr;
    QGCState* _setupState = nullptr;
    QGCState* _rotatingState = nullptr;
    QGCFinalState* _successState = nullptr;
    QGCFinalState* _failedState = nullptr;

    // Compass tracking
    uint8_t _compassMask = 0;
    uint8_t _rgProgress[MaxCompasses] = {0, 0, 0};
    bool _rgComplete[MaxCompasses] = {false, false, false};
    bool _rgSucceeded[MaxCompasses] = {false, false, false};
    float _rgFitness[MaxCompasses] = {0.0f, 0.0f, 0.0f};

    // Previous compass cal fitness for restoration
    bool _restoreFitness = false;
    float _previousFitness = 0.0f;
};
