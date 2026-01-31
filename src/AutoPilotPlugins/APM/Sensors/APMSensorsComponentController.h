#pragma once

#include "SensorCalibrationControllerBase.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQuick/QQuickItem>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(APMSensorsComponentControllerLog)
Q_DECLARE_LOGGING_CATEGORY(APMSensorsComponentControllerVerboseLog)

class APMSensorsComponent;
class LinkInterface;
class APMSensorCalibrationStateMachine;
class AccelCalibrationMachine;
class CompassCalibrationMachine;

/// Sensors Component MVC Controller for SensorsComponent.qml.
class APMSensorsComponentController : public SensorCalibrationControllerBase
{
    Q_OBJECT
    QML_ELEMENT

    friend class APMSensorCalibrationStateMachine;
    friend class AccelCalibrationMachine;
    friend class CompassCalibrationMachine;

    Q_PROPERTY(QQuickItem* nextButton MEMBER _nextButton)

    Q_PROPERTY(bool compassSetupNeeded READ compassSetupNeeded NOTIFY setupNeededChanged)
    Q_PROPERTY(bool accelSetupNeeded READ accelSetupNeeded NOTIFY setupNeededChanged)

    Q_PROPERTY(bool compass1CalSucceeded READ compass1CalSucceeded NOTIFY compass1CalSucceededChanged)
    Q_PROPERTY(bool compass2CalSucceeded READ compass2CalSucceeded NOTIFY compass2CalSucceededChanged)
    Q_PROPERTY(bool compass3CalSucceeded READ compass3CalSucceeded NOTIFY compass3CalSucceededChanged)

    Q_PROPERTY(double compass1CalFitness READ compass1CalFitness NOTIFY compass1CalFitnessChanged)
    Q_PROPERTY(double compass2CalFitness READ compass2CalFitness NOTIFY compass2CalFitnessChanged)
    Q_PROPERTY(double compass3CalFitness READ compass3CalFitness NOTIFY compass3CalFitnessChanged)

public:
    explicit APMSensorsComponentController(QObject *parent = nullptr);
    ~APMSensorsComponentController();

    Q_INVOKABLE void calibrateCompass();
    Q_INVOKABLE void calibrateAccel(bool doSimpleAccelCal);
    Q_INVOKABLE void calibrateCompassNorth(float lat, float lon, int mask);
    Q_INVOKABLE void calibrateGyro();
    Q_INVOKABLE void calibrateMotorInterference();
    Q_INVOKABLE void levelHorizon();
    Q_INVOKABLE void calibratePressure();
    Q_INVOKABLE void cancelCalibration();
    Q_INVOKABLE void nextClicked();
    Q_INVOKABLE bool usingUDPLink() const;

    bool compassSetupNeeded() const;
    bool accelSetupNeeded() const;

    bool compass1CalSucceeded() const { return _rgCompassCalSucceeded[0]; }
    bool compass2CalSucceeded() const { return _rgCompassCalSucceeded[1]; }
    bool compass3CalSucceeded() const { return _rgCompassCalSucceeded[2]; }

    double compass1CalFitness() const { return _rgCompassCalFitness[0]; }
    double compass2CalFitness() const { return _rgCompassCalFitness[1]; }
    double compass3CalFitness() const { return _rgCompassCalFitness[2]; }

signals:
    void showGyroCalAreaChanged();
    void setupNeededChanged();
    void calibrationComplete(QGCMAVLink::CalibrationType calType);
    void compass1CalSucceededChanged(bool compass1CalSucceeded);
    void compass2CalSucceededChanged(bool compass2CalSucceeded);
    void compass3CalSucceededChanged(bool compass3CalSucceeded);
    void compass1CalFitnessChanged(double compass1CalFitness);
    void compass2CalFitnessChanged(double compass2CalFitness);
    void compass3CalFitnessChanged(double compass3CalFitness);
    void setAllCalButtonsEnabled(bool enabled);

protected:
    void setButtonsEnabled(bool enabled) override;

private slots:
    void _handleTextMessage(int sysid, int componentid, int severity, const QString &text, const QString &description);
    void _mavlinkMessageReceived(LinkInterface *link, const mavlink_message_t &message);
    void _mavCommandResult(int vehicleId, int component, int command, int result, int failureCode);

private:
    void _refreshParams();
    void _resetInternalState();

    APMSensorsComponent *_sensorsComponent = nullptr;

    QQuickItem *_nextButton = nullptr;

    QGCMAVLink::CalibrationType _calTypeInProgress = QGCMAVLink::CalibrationNone;

    // Compass calibration results (populated by CompassCalibrationMachine on completion)
    bool _rgCompassCalSucceeded[3] = {false, false, false};
    float _rgCompassCalFitness[3] = {0.0f, 0.0f, 0.0f};

    static constexpr int _supportedFirmwareCalVersion = 2;

    APMSensorCalibrationStateMachine* _stateMachine = nullptr;
};
