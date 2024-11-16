#include "VehicleLTEFactGroup.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"
#include <QDebug>

QGC_LOGGING_CATEGORY(VehicleLTEFactGroupLog, "VehicleLTE")

VehicleLTEFactGroup::VehicleLTEFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/LTEFact.json", parent)
    , _rsrqFact                   (0, _rsrqFactName,                FactMetaData::valueTypeDouble)
    , _rsrpFact                   (0, _rsrpFactName,                FactMetaData::valueTypeDouble)
    , _rssiFact                   (0, _rssiFactName,                FactMetaData::valueTypeDouble)
    , _rssnrFact                  (0, _rssnrFactName,               FactMetaData::valueTypeDouble)
    , _earfcnFact                 (0, _earfcnFactName,              FactMetaData::valueTypeInt32)
    , _lteTxFact                  (0, _lteTxFactName,               FactMetaData::valueTypeInt32)
    , _isRoutableFact             (0, _isRoutableFactName,          FactMetaData::valueTypeString)
    , _latencyMsFact              (0, _latencyMsFactName,           FactMetaData::valueTypeDouble)
    , _lossPercentFact            (0, _lossPercentFactName,         FactMetaData::valueTypeDouble)
{
    _addFact(&_rsrqFact,                  _rsrqFactName);
    _addFact(&_rsrpFact,                  _rsrpFactName);
    _addFact(&_rssiFact,                  _rssiFactName);
    _addFact(&_rssnrFact,                 _rssnrFactName);
    _addFact(&_earfcnFact,                _earfcnFactName);
    _addFact(&_lteTxFact,                 _lteTxFactName);
    _addFact(&_isRoutableFact,            _isRoutableFactName);
    _addFact(&_latencyMsFact,             _latencyMsFactName);
    _addFact(&_lossPercentFact,           _lossPercentFactName);

    _rsrqFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rsrpFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rssiFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _rssnrFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _earfcnFact.setRawValue(std::numeric_limits<int32_t>::quiet_NaN());
    _lteTxFact.setRawValue(std::numeric_limits<int32_t>::quiet_NaN());
    _isRoutableFact.setRawValue(QVariant(QString("Disconnected")));
    _latencyMsFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _lossPercentFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

void VehicleLTEFactGroup::handleMessage(Vehicle*  vehicle, const mavlink_message_t message)
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

void VehicleLTEFactGroup::_handleCommandLong(mavlink_message_t message)
{
    mavlink_command_long_t commandLong;
    mavlink_msg_command_long_decode(&message, &commandLong);

    if (commandLong.command != MAV_CMD_USER_1) {
        return;
    }

    switch (static_cast<int>(commandLong.param1)) {
        case 31014:
            // LTE radio telemetry
//            qCWarning(VehicleLTEFactGroupLog) << "Received COMMAND_LONG 31014 (LTE radio telemetry)";
            rssi()->setRawValue(commandLong.param2);
            rsrq()->setRawValue(commandLong.param3);
            rsrp()->setRawValue(commandLong.param4);
            rssnr()->setRawValue(commandLong.param5);
            earfcn()->setRawValue(commandLong.param6);
            lteTx()->setRawValue(commandLong.param7);
            break;
        case 31015:
            // LTE IP telemetry
//            qCWarning(VehicleLTEFactGroupLog) << "Received COMMAND_LONG 31015 (LTE IP telemetry)";
            isRoutable()->setRawValue(commandLong.param2 > 0);
            latencyMs()->setRawValue(commandLong.param3);
            lossPercent()->setRawValue(commandLong.param4);
            break;
    }
}
