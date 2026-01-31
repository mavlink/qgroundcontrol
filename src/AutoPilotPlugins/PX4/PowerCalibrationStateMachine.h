#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>

Q_DECLARE_LOGGING_CATEGORY(PowerCalibrationStateMachineLog)

class PowerComponentController;
class Vehicle;

/// State machine for ESC calibration and UAVCAN bus configuration.
///
/// Uses FunctionState for terminal states, QGCState for message-driven states.
/// State transitions are triggered by parsed text messages from the vehicle.
class PowerCalibrationStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    enum CalibrationType {
        CalibrationNone,
        CalibrationEsc,
        CalibrationBusConfig
    };
    Q_ENUM(CalibrationType)

    explicit PowerCalibrationStateMachine(PowerComponentController* controller, Vehicle* vehicle, QObject* parent = nullptr);
    ~PowerCalibrationStateMachine() override;

    /// Start ESC calibration
    void startEscCalibration();

    /// Start UAVCAN bus configuration
    void startBusConfig();

    /// Stop current calibration/configuration
    void stopCalibration();

    /// Handle text message from vehicle (parses [cal] messages)
    void handleTextMessage(const QString& text);

    /// @return true if calibration is in progress
    bool isCalibrating() const { return _calibrating; }

    /// @return Current calibration type
    CalibrationType calibrationType() const { return _calibrationType; }

    /// @return Warning messages collected during calibration
    QStringList warningMessages() const { return _warningMessages; }

signals:
    void connectBattery();
    void batteryConnected();
    void disconnectBattery();
    void calibrationFailed(const QString& errorMessage);
    void calibrationSuccess(const QStringList& warningMessages);
    void oldFirmware();
    void newerFirmware();
    void incorrectFirmwareRevReporting();

private:
    void _buildStateMachine();
    void _wireTransitions();
    void _parseCalibrationMessage(const QString& text);

    // State entry functions
    void _onIdleEntered();
    void _onStartedEntered();
    void _onWaitingBatteryEntered();
    void _onBatteryConnectedEntered();
    void _onSuccess();
    void _onFailed();

    PowerComponentController* _controller;
    Vehicle* _vehicle;

    // Message-driven states
    QGCState* _idleState = nullptr;
    QGCState* _startedState = nullptr;
    QGCState* _waitingBatteryState = nullptr;
    QGCState* _batteryConnectedState = nullptr;

    // Terminal states using semantic types
    FunctionState* _successState = nullptr;
    FunctionState* _failedState = nullptr;
    QGCFinalState* _finalState = nullptr;

    // State tracking
    bool _calibrating = false;
    CalibrationType _calibrationType = CalibrationNone;
    QStringList _warningMessages;
    QString _failureMessage;

    static constexpr int _neededFirmwareRev = 1;
    static constexpr int _timeoutMs = 60000;  // 60 second timeout
};
