/****************************************************************************
 *
 *   (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMSubMotorComponentController.h"
#include "Vehicle.h"


APMSubMotorComponentController::APMSubMotorComponentController(void)
{   
    connect(_vehicle, &Vehicle::textMessageReceived, this, &APMSubMotorComponentController::handleNewMessages);
}

void APMSubMotorComponentController::handleNewMessages(int componentid, int severity, QString text, QString description)
{
    Q_UNUSED(componentid);
    Q_UNUSED(severity);
    Q_UNUSED(description);
    if (_vehicle->flightMode() == "Motor Detection"
        && (text.toLower().contains("thruster") || text.toLower().contains("motor"))) {
        _motorDetectionMessages += text + QStringLiteral("\n");
        emit motorDetectionMessagesChanged();
    }
}
