#pragma once

#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

/// Controller for PX4EscComponent.qml. Runs the ESC PWM calibration.
class PX4EscController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT

public:
    PX4EscController();

    Q_INVOKABLE void calibrateEsc();

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
    void _handleVehicleTextMessage(quint32 vehicleId, int compId, int severity, QString text,
                                   const QString& description);

private:
    void _stopCalibration();

    QStringList _warningMessages;
    static const int _neededFirmwareRev = 1;
};
