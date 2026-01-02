#include "APMSubMotorComponentController.h"
#include "Vehicle.h"

APMSubMotorComponentController::APMSubMotorComponentController(QObject *parent)
    : FactPanelController(parent)
{
    (void) connect(_vehicle, &Vehicle::textMessageReceived, this, &APMSubMotorComponentController::_handleNewMessages);
}

void APMSubMotorComponentController::_handleNewMessages(int sysid, int componentid, int severity, const QString &text, const QString &description)
{
    Q_UNUSED(sysid); Q_UNUSED(componentid); Q_UNUSED(severity); Q_UNUSED(description);

    if ((_vehicle->flightMode() == _vehicle->motorDetectionFlightMode()) && (text.toLower().contains("thruster") || text.toLower().contains("motor"))) {
        _motorDetectionMessages += text + QStringLiteral("\n");
        emit motorDetectionMessagesChanged();
    }
}
