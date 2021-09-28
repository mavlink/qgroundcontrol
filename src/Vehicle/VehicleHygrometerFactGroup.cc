/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleHygrometerFactGroup.h"
#include "Vehicle.h"
#include "QGCGeo.h"

const char* VehicleHygrometerFactGroup::_hygroHumiFactName =      "humidity";
const char* VehicleHygrometerFactGroup::_hygroTempFactName =    "temperature";
const char* VehicleHygrometerFactGroup::_hygroIDFactName =    "hygrometerid";

VehicleHygrometerFactGroup::VehicleHygrometerFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/HygrometerFact.json", parent)
    , _hygroTempFact             (0, _hygroTempFactName,         FactMetaData::valueTypeDouble)
    , _hygroHumiFact             (0, _hygroHumiFactName,         FactMetaData::valueTypeDouble)
    , _hygroIDFact               (0, _hygroIDFactName,           FactMetaData::valueTypeUint16)
{   
    _addFact(&_hygroTempFact,               _hygroTempFactName);
    _addFact(&_hygroHumiFact,               _hygroHumiFactName);
    _addFact(&_hygroIDFact,                 _hygroIDFactName);

    _hygroTempFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _hygroHumiFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _hygroIDFact.setRawValue(std::numeric_limits<unsigned int>::quiet_NaN());
}

void VehicleHygrometerFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_HYGROMETER_SENSOR:
       _handleHygrometerSensor(message);
       break;
    default:
        break;
    }
}

void VehicleHygrometerFactGroup::_handleHygrometerSensor(mavlink_message_t& message)
{
    mavlink_hygrometer_sensor_t hygrometer;
    mavlink_msg_hygrometer_sensor_decode(&message, &hygrometer);

    _hygroTempFact.setRawValue(hygrometer.temperature);
    _hygroHumiFact.setRawValue(hygrometer.humidity);
    _hygroIDFact.setRawValue(hygrometer.id);
}
