/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleBatteryFactGroup.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

const char* VehicleBatteryFactGroup::_batteryFactGroupNamePrefix    = "battery";

const char* VehicleBatteryFactGroup::_batteryIdFactName             = "id";
const char* VehicleBatteryFactGroup::_batteryFunctionFactName       = "batteryFunction";
const char* VehicleBatteryFactGroup::_batteryTypeFactName           = "batteryType";
const char* VehicleBatteryFactGroup::_voltageFactName               = "voltage";
const char* VehicleBatteryFactGroup::_percentRemainingFactName      = "percentRemaining";
const char* VehicleBatteryFactGroup::_mahConsumedFactName           = "mahConsumed";
const char* VehicleBatteryFactGroup::_currentFactName               = "current";
const char* VehicleBatteryFactGroup::_temperatureFactName           = "temperature";
const char* VehicleBatteryFactGroup::_instantPowerFactName          = "instantPower";
const char* VehicleBatteryFactGroup::_timeRemainingFactName         = "timeRemaining";
const char* VehicleBatteryFactGroup::_timeRemainingStrFactName      = "timeRemainingStr";
const char* VehicleBatteryFactGroup::_chargeStateFactName           = "chargeState";

const char* VehicleBatteryFactGroup::_settingsGroup =                       "Vehicle.battery";

VehicleBatteryFactGroup::VehicleBatteryFactGroup(uint8_t batteryId, QObject* parent)
    : FactGroup             (1000, ":/json/Vehicle/BatteryFact.json", parent)
    , _batteryIdFact        (0, _batteryIdFactName,                 FactMetaData::valueTypeUint8)
    , _batteryFunctionFact  (0, _batteryFunctionFactName,           FactMetaData::valueTypeUint8)
    , _batteryTypeFact      (0, _batteryTypeFactName,               FactMetaData::valueTypeUint8)
    , _voltageFact          (0, _voltageFactName,                   FactMetaData::valueTypeDouble)
    , _currentFact          (0, _currentFactName,                   FactMetaData::valueTypeDouble)
    , _mahConsumedFact      (0, _mahConsumedFactName,               FactMetaData::valueTypeDouble)
    , _temperatureFact      (0, _temperatureFactName,               FactMetaData::valueTypeDouble)
    , _percentRemainingFact (0, _percentRemainingFactName,          FactMetaData::valueTypeDouble)
    , _timeRemainingFact    (0, _timeRemainingFactName,             FactMetaData::valueTypeDouble)
    , _timeRemainingStrFact (0, _timeRemainingStrFactName,          FactMetaData::valueTypeString)
    , _chargeStateFact      (0, _chargeStateFactName,               FactMetaData::valueTypeUint8)
    , _instantPowerFact     (0, _instantPowerFactName,              FactMetaData::valueTypeDouble)
{
    _addFact(&_batteryIdFact,               _batteryIdFactName);
    _addFact(&_batteryFunctionFact,         _batteryFunctionFactName);
    _addFact(&_batteryTypeFact,             _batteryTypeFactName);
    _addFact(&_voltageFact,                 _voltageFactName);
    _addFact(&_currentFact,                 _currentFactName);
    _addFact(&_mahConsumedFact,             _mahConsumedFactName);
    _addFact(&_temperatureFact,             _temperatureFactName);
    _addFact(&_percentRemainingFact,        _percentRemainingFactName);
    _addFact(&_timeRemainingFact,           _timeRemainingFactName);
    _addFact(&_timeRemainingStrFact,        _timeRemainingStrFactName);
    _addFact(&_chargeStateFact,             _chargeStateFactName);
    _addFact(&_instantPowerFact,            _instantPowerFactName);

    _batteryIdFact.setRawValue          (batteryId);
    _batteryFunctionFact.setRawValue    (MAV_BATTERY_FUNCTION_UNKNOWN);
    _batteryTypeFact.setRawValue        (MAV_BATTERY_TYPE_UNKNOWN);
    _voltageFact.setRawValue            (qQNaN());
    _currentFact.setRawValue            (qQNaN());
    _mahConsumedFact.setRawValue        (qQNaN());
    _temperatureFact.setRawValue        (qQNaN());
    _percentRemainingFact.setRawValue   (qQNaN());
    _timeRemainingFact.setRawValue      (qQNaN());
    _chargeStateFact.setRawValue        (MAV_BATTERY_CHARGE_STATE_UNDEFINED);
    _instantPowerFact.setRawValue       (qQNaN());

    connect(&_timeRemainingFact, &Fact::rawValueChanged, this, &VehicleBatteryFactGroup::_timeRemainingChanged);
}

void VehicleBatteryFactGroup::handleMessageForFactGroupCreation(Vehicle* vehicle, mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_HIGH_LATENCY:
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _findOrAddBatteryGroupById(vehicle, 0);
        break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
    {
        mavlink_battery_status_t batteryStatus;
        mavlink_msg_battery_status_decode(&message, &batteryStatus);
        _findOrAddBatteryGroupById(vehicle, batteryStatus.id);
    }
        break;
    }
}

void VehicleBatteryFactGroup::handleMessage(Vehicle* vehicle, mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(vehicle, message);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(vehicle, message);
        break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
        _handleBatteryStatus(vehicle, message);
        break;
    }
}

void VehicleBatteryFactGroup::_handleHighLatency(Vehicle* vehicle, mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);

    VehicleBatteryFactGroup* group = _findOrAddBatteryGroupById(vehicle, 0);
    group->percentRemaining()->setRawValue(highLatency.battery_remaining == UINT8_MAX ? qQNaN() : highLatency.battery_remaining);
    group->_setTelemetryAvailable(true);
}

void VehicleBatteryFactGroup::_handleHighLatency2(Vehicle* vehicle, mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    VehicleBatteryFactGroup* group = _findOrAddBatteryGroupById(vehicle, 0);
    group->percentRemaining()->setRawValue(highLatency2.battery == -1 ? qQNaN() : highLatency2.battery);
    group->_setTelemetryAvailable(true);
}

void VehicleBatteryFactGroup::_handleBatteryStatus(Vehicle* vehicle, mavlink_message_t& message)
{
    mavlink_battery_status_t batteryStatus;
    mavlink_msg_battery_status_decode(&message, &batteryStatus);

    VehicleBatteryFactGroup* group = _findOrAddBatteryGroupById(vehicle, batteryStatus.id);

    double totalVoltage = qQNaN();
    for (int i=0; i<10; i++) {
        double cellVoltage = batteryStatus.voltages[i] == UINT16_MAX ? qQNaN() : static_cast<double>(batteryStatus.voltages[i]) / 1000.0;
        if (qIsNaN(cellVoltage)) {
            break;
        }
        if (i == 0) {
            totalVoltage = cellVoltage;
        } else {
            totalVoltage += cellVoltage;
        }
    }
    for (int i=0; i<4; i++) {
        double cellVoltage = batteryStatus.voltages_ext[i] == UINT16_MAX ? qQNaN() : static_cast<double>(batteryStatus.voltages_ext[i]) / 1000.0;
        if (qIsNaN(cellVoltage)) {
            break;
        }
        totalVoltage += cellVoltage;
    }

    group->function()->setRawValue          (batteryStatus.battery_function);
    group->type()->setRawValue              (batteryStatus.type);
    group->temperature()->setRawValue       (batteryStatus.temperature == INT16_MAX ?   qQNaN() : static_cast<double>(batteryStatus.temperature) / 100.0);
    group->voltage()->setRawValue           (totalVoltage);
    group->current()->setRawValue           (batteryStatus.current_battery == -1 ?      qQNaN() : static_cast<double>(batteryStatus.current_battery) / 100.0);
    group->mahConsumed()->setRawValue       (batteryStatus.current_consumed == -1  ?    qQNaN() : batteryStatus.current_consumed);
    group->percentRemaining()->setRawValue  (batteryStatus.battery_remaining == -1 ?    qQNaN() : batteryStatus.battery_remaining);
    group->timeRemaining()->setRawValue     (batteryStatus.time_remaining == 0 ?        qQNaN() : batteryStatus.time_remaining);
    group->chargeState()->setRawValue       (batteryStatus.charge_state);
    group->instantPower()->setRawValue      (totalVoltage * group->current()->rawValue().toDouble());
    group->_setTelemetryAvailable(true);
}

VehicleBatteryFactGroup* VehicleBatteryFactGroup::_findOrAddBatteryGroupById(Vehicle* vehicle, uint8_t batteryId)
{
    QmlObjectListModel* batteries = vehicle->batteries();

    // We maintain the list in order sorted by battery id so the ui shows them sorted.
    for (int i=0; i<batteries->count(); i++) {
        VehicleBatteryFactGroup* group = batteries->value<VehicleBatteryFactGroup*>(i);
        int listBatteryId = group->id()->rawValue().toInt();
        if (listBatteryId >  batteryId) {
            VehicleBatteryFactGroup* newBatteryGroup = new VehicleBatteryFactGroup(batteryId, batteries);
            batteries->insert(i, newBatteryGroup);
            vehicle->_addFactGroup(newBatteryGroup, QStringLiteral("%1%2").arg(_batteryFactGroupNamePrefix).arg(batteryId));
            return newBatteryGroup;
        } else if (listBatteryId == batteryId) {
            return group;
        }
    }

    VehicleBatteryFactGroup* newBatteryGroup = new VehicleBatteryFactGroup(batteryId, batteries);
    batteries->append(newBatteryGroup);
    vehicle->_addFactGroup(newBatteryGroup, QStringLiteral("%1%2").arg(_batteryFactGroupNamePrefix).arg(batteryId));

    return newBatteryGroup;
}

void VehicleBatteryFactGroup::_timeRemainingChanged(QVariant value)
{
    if (qIsNaN(value.toDouble())) {
        _timeRemainingStrFact.setRawValue("--:--:--");
    } else {
        int totalSeconds    = value.toInt();
        int hours           = totalSeconds / 3600;
        int minutes         = (totalSeconds % 3600) / 60;
        int seconds         = totalSeconds % 60;

        _timeRemainingStrFact.setRawValue(QString::asprintf("%02dH:%02dM:%02dS", hours, minutes, seconds));
    }
}
