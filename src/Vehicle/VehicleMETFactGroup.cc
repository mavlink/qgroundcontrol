// not fully tested, NJD 1/8/24
// super crude with no safeties and may not capture the desired architecture

#include "VehicleMETFactGroup.h"
#include "Vehicle.h"

const char* VehicleMETFactGroup::_temperatureFactName =                      "air_temperature";
const char* VehicleMETFactGroup::_rhumFactName =                             "relative_humidity";

VehicleMETFactGroup::VehicleMETFactGroup(QObject* parent)
    : FactGroup                         (1000, ":/json/Vehicle/VehicleMETFactGroup.json", parent) // there's a entry in this file for each fact group!
    // one entry here per fact
    , _temperatureFact                  (0, _temperatureFactName,                  FactMetaData::valueTypeFloat)
    , _rhumFact                         (0, _rhumFactName,                         FactMetaData::valueTypeFloat)
{
    // one entry here per fact
    _addFact(&_temperatureFact,                       _temperatureFactName);
    _addFact(&_rhumFact,                              _rhumFactName);
}

void VehicleMETFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    if (!(message.msgid == MAVLINK_MSG_ID_CASS_SENSOR_RAW
        || message.msgid == MAVLINK_MSG_ID_AHRS2 // add lines for additional message IDs of interest
          ))
    {
        return;
    }

    uint8_t messageByte = 0;
    uint32_t messageInt = 0;
    //float messageFloat = 0;

    switch(message.msgid)
    {
    case MAVLINK_MSG_ID_CASS_SENSOR_RAW:
        // decode based on extracted parameter
        // 0 means temp, 1 means RH, 2 means tempR (not what the .xml defines it as)
        // decoding determined largely by trial and error
        messageInt = static_cast<uint32_t>(message.payload64[3] & 0xFFFFFFFF);
        messageByte = static_cast<uint8_t>(messageInt & 0xFF);

        if (messageByte == 0)
        {
            messageInt = static_cast<uint32_t>((message.payload64[0] & 0xFFFFFFFF00000000) >> 32);
            _currentContents.temperature1 = *reinterpret_cast<float*>(&messageInt);

            messageInt = static_cast<uint32_t>((message.payload64[1] & 0xFFFFFFFF00000000) >> 32);
            _currentContents.temperature2 = *reinterpret_cast<float*>(&messageInt);

            messageInt = static_cast<uint32_t>(message.payload64[1] & 0xFFFFFFFF);
            _currentContents.temperature3 = *reinterpret_cast<float*>(&messageInt);
        }
        if (messageByte == 1)
        {
            messageInt = static_cast<uint32_t>((message.payload64[0] & 0xFFFFFFFF00000000) >> 32);
            _currentContents.rhum1 = *reinterpret_cast<float*>(&messageInt);

            messageInt = static_cast<uint32_t>((message.payload64[1] & 0xFFFFFFFF00000000) >> 32);
            _currentContents.rhum2 = *reinterpret_cast<float*>(&messageInt);

            messageInt = static_cast<uint32_t>(message.payload64[1] & 0xFFFFFFFF);
            _currentContents.rhum3 = *reinterpret_cast<float*>(&messageInt);
        }
        if (messageByte == 2)
        {
            messageInt = static_cast<uint32_t>((message.payload64[0] & 0xFFFFFFFF00000000) >> 32);
            _currentContents.tempR1 = *reinterpret_cast<float*>(&messageInt);

            messageInt = static_cast<uint32_t>((message.payload64[1] & 0xFFFFFFFF00000000) >> 32);
            _currentContents.tempR2 = *reinterpret_cast<float*>(&messageInt);

            messageInt = static_cast<uint32_t>(message.payload64[1] & 0xFFFFFFFF);
            _currentContents.tempR3 = *reinterpret_cast<float*>(&messageInt);
        }
        break;

    case MAVLINK_MSG_ID_AHRS2:
        // placeholder example of where would catch another message for parsing
        break;

    default:
        break;
    }

    // this where the fact actually gets updated
    // one entry here per fact
    // better to only call the ones that get updated
    temperature()->setRawValue((_currentContents.temperature1 + _currentContents.temperature2 + _currentContents.temperature3)/3);
    rhum()->setRawValue((_currentContents.rhum1 + _currentContents.rhum2 + _currentContents.rhum3)/3);
}
