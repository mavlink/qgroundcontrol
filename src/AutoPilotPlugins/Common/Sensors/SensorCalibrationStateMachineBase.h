#pragma once

#include "QGCStateMachine.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(SensorCalibrationStateMachineBaseLog)

class SensorCalibrationControllerBase;

/// Base class for APM and PX4 sensor calibration state machines.
///
/// Provides common infrastructure for sensor calibration workflows:
/// - Common calibration type tracking
/// - Common state structure (idle, simpleCal, cancelling)
/// - Common signals for calibration start/complete
/// - Helper methods for calibration lifecycle
///
/// Subclasses implement platform-specific calibration methods and
/// message handling (MAVLink for APM, text messages for PX4).
class SensorCalibrationStateMachineBase : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit SensorCalibrationStateMachineBase(const QString& name,
                                                SensorCalibrationControllerBase* controller,
                                                QObject* parent = nullptr);
    ~SensorCalibrationStateMachineBase() override = default;

    /// Start compass/magnetometer calibration
    virtual void calibrateCompass() = 0;

    /// Start gyroscope calibration
    virtual void calibrateGyro() = 0;

    /// Cancel current calibration
    virtual void cancelCalibration() = 0;

    /// @return true if any calibration is in progress
    bool isCalibrating() const;

    /// @return The current calibration type in progress
    QGCMAVLink::CalibrationType currentCalibrationType() const { return _calType; }

signals:
    /// Emitted when calibration starts
    void calibrationStarted(QGCMAVLink::CalibrationType calType);

    /// Emitted when calibration completes
    void calibrationComplete(QGCMAVLink::CalibrationType calType, bool success);

protected:
    /// Called when starting a calibration that only shows log messages
    virtual void startLogCalibration();

    /// Called when starting a calibration that shows visual orientation UI
    virtual void startVisualCalibration();

    /// Called when calibration ends
    /// @param success true if calibration succeeded
    virtual void stopCalibration(bool success);

    /// Set up the common base states (idle, simpleCal, cancelling)
    /// Call this from subclass _buildStateMachine() before adding custom states
    void buildBaseStates();

    /// Enable/disable calibration buttons on the controller
    void setButtonsEnabled(bool enabled);

    /// Append message to the status log
    void appendStatusLog(const QString& text);

    /// Hide all calibration areas
    void hideAllCalAreas();

    /// Show/hide orientation calibration area
    void setShowOrientationCalArea(bool show);

    SensorCalibrationControllerBase* _controller;
    QGCMAVLink::CalibrationType _calType = QGCMAVLink::CalibrationNone;

    // Common states
    QGCState* _idleState = nullptr;
    QGCState* _simpleCalState = nullptr;
    QGCState* _cancellingState = nullptr;
};
