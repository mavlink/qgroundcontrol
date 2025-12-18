/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleRPMSensorFactGroup.h"
#include "Vehicle.h"

VehicleRPMSensorFactGroup::VehicleRPMSensorFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/RPMSensorFact.json"), parent)
{
    _addFact(&_rpm1SensorFact);
    _addFact(&_rpm2SensorFact);

    _rpm1SensorFact.setRawValue(qQNaN());
    _rpm2SensorFact.setRawValue(qQNaN());
}

void VehicleRPMSensorFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_RPM:
        _handleRPMSensor(message);
        break;
    default:
        break;
    }
}

void VehicleRPMSensorFactGroup::_handleRPMSensor(const mavlink_message_t &message)
{
    mavlink_rpm_t rpm{};
    mavlink_msg_rpm_decode(&message, &rpm);

    _rpm1SensorFact.setRawValue(rpm.rpm1);
    _rpm2SensorFact.setRawValue(rpm.rpm2);

    _setTelemetryAvailable(true);
}
