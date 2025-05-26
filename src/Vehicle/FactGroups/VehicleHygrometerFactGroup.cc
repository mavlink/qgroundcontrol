/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleHygrometerFactGroup.h"
#include "Vehicle.h"

VehicleHygrometerFactGroup::VehicleHygrometerFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/HygrometerFact.json"), parent)
{
    _addFact(&_hygroTempFact);
    _addFact(&_hygroHumiFact);
    _addFact(&_hygroIDFact);

    _hygroTempFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _hygroHumiFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _hygroIDFact.setRawValue(std::numeric_limits<unsigned int>::quiet_NaN());
}

void VehicleHygrometerFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HYGROMETER_SENSOR:
       _handleHygrometerSensor(message);
       break;
    default:
        break;
    }
}

void VehicleHygrometerFactGroup::_handleHygrometerSensor(const mavlink_message_t &message)
{
    mavlink_hygrometer_sensor_t hygrometer{};
    mavlink_msg_hygrometer_sensor_decode(&message, &hygrometer);

    _hygroTempFact.setRawValue(hygrometer.temperature / 100.f);
    _hygroHumiFact.setRawValue(hygrometer.humidity);
    _hygroIDFact.setRawValue(hygrometer.id);

    _setTelemetryAvailable(true);
}
