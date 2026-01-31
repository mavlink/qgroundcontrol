#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

class PowerCalibrationStateMachine;

/// Power Component MVC Controller for PowerComponent.qml.
class PowerComponentController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT

    friend class PowerCalibrationStateMachine;

public:
    PowerComponentController();

    Q_INVOKABLE void calibrateEsc();
    Q_INVOKABLE void startBusConfigureActuators();
    Q_INVOKABLE void stopBusConfigureActuators();

signals:
    void oldFirmware();
    void newerFirmware();
    void incorrectFirmwareRevReporting();
    void connectBattery();
    void disconnectBattery();
    void batteryConnected();
    void calibrationFailed(const QString& errorMessage);
    void calibrationSuccess(const QStringList& warningMessages);

private slots:
    void _handleVehicleTextMessage(int vehicleId, int compId, int severity, QString text, const QString& description);

private:
    PowerCalibrationStateMachine* _stateMachine = nullptr;
};
