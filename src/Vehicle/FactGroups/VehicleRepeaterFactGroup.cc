#include "VehicleRepeaterFactGroup.h"
#include "Vehicle.h"
#include "QGCGeo.h"

VehicleRepeaterFactGroup::VehicleRepeaterFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/RepeaterFact.json", parent)
    , _rssiFact                   (0, _rssiFactName,                   FactMetaData::valueTypeDouble)
    , _rssnrFact                  (0, _rssnrFactName,                  FactMetaData::valueTypeDouble)
    , _freqFact                   (0, _freqFactName,                   FactMetaData::valueTypeInt32)
    , _videoKbpsFact              (0, _videoKbpsFactName,              FactMetaData::valueTypeDouble)
    , _mavlinkToDroneKbpsFact     (0, _mavlinkToDroneKbpsFactName,     FactMetaData::valueTypeDouble)
    , _mavlinkFromDroneKbpsFact   (0, _mavlinkFromDroneKbpsFactName,   FactMetaData::valueTypeDouble)
    , _isDroneConnectedFact       (0, _isDroneConnectedFactName,       FactMetaData::valueTypeBool)
    , _isDroneStatsFact           (0, _isDroneStatsFactName,           FactMetaData::valueTypeBool)
    , _isRoutableFact             (0, _isRoutableFactName,          FactMetaData::valueTypeString)
    , _latencyMsFact              (0, _latencyMsFactName,           FactMetaData::valueTypeDouble)
    , _lossPercentFact            (0, _lossPercentFactName,         FactMetaData::valueTypeDouble)
{
    _addFact(&_rssiFact,                     _rssiFactName);
    _addFact(&_rssnrFact,                    _rssnrFactName);
    _addFact(&_freqFact,                     _freqFactName);
    _addFact(&_videoKbpsFact,                _videoKbpsFactName);
    _addFact(&_mavlinkToDroneKbpsFact,       _mavlinkToDroneKbpsFactName);
    _addFact(&_mavlinkFromDroneKbpsFact,     _mavlinkFromDroneKbpsFactName);
    _addFact(&_isDroneConnectedFact,         _isDroneConnectedFactName);
    _addFact(&_isDroneStatsFact,             _isDroneStatsFactName);
    _addFact(&_isRoutableFact,               _isRoutableFactName);
    _addFact(&_latencyMsFact,                _latencyMsFactName);
    _addFact(&_lossPercentFact,              _lossPercentFactName);

    _rssiFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rssnrFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _freqFact.setRawValue(std::numeric_limits<int32_t>::quiet_NaN());
    _videoKbpsFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _mavlinkToDroneKbpsFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _mavlinkFromDroneKbpsFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _isDroneConnectedFact.setRawValue(std::numeric_limits<bool>::quiet_NaN());
    _isDroneStatsFact.setRawValue(std::numeric_limits<bool>::quiet_NaN());
    _isRoutableFact.setRawValue(QVariant(QString("Disconnected")));
    _latencyMsFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _lossPercentFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

void VehicleRepeaterFactGroup::handleMessage(Vehicle*  vehicle, const mavlink_message_t &message)
{
    if (message.sysid != vehicle->id() || message.compid != 10) {
        return;
    }

    switch (message.msgid) {
        case MAVLINK_MSG_ID_COMMAND_LONG:
//            qCWarning(VehicleLTEFactGroupLog) << "Received COMMAND_LONG";
            _handleCommandLong(message);
            break;
        default:
            break;
    }
}

void VehicleRepeaterFactGroup::_handleCommandLong(const mavlink_message_t &message)
{
    mavlink_command_long_t commandLong;
    mavlink_msg_command_long_decode(&message, &commandLong);

    if (commandLong.command != MAV_CMD_USER_1) {
        return;
    }

    bool is_on_drone_side = true;
    switch (static_cast<int>(commandLong.param1)) {
        case -31016:
            is_on_drone_side = false;
        case 31016:
            // LM radio telemetry
            rssi()->setRawValue(commandLong.param2);
            rssnr()->setRawValue(commandLong.param3);
            freq()->setRawValue(commandLong.param4);
            videoKbps()->setRawValue(commandLong.param5);
            mavlinkToDroneKbps()->setRawValue(commandLong.param6);
            mavlinkFromDroneKbps()->setRawValue(commandLong.param7);
            isDroneStats()->setRawValue(is_on_drone_side);
            isDroneConnected()->setRawValue(commandLong.param5 > 0);
            break;
        case 31017:
            // LM IP telemetry
            isRoutable()->setRawValue(commandLong.param2 > 0);
            latencyMs()->setRawValue(commandLong.param3);
            lossPercent()->setRawValue(commandLong.param4);
            break;
//TODO repeater wfb stats
    }
}
