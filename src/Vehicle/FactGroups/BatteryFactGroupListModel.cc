#include "BatteryFactGroupListModel.h"

BatteryFactGroupListModel::BatteryFactGroupListModel(QObject* parent)
    : FactGroupListModel("battery", parent)
{

}

bool BatteryFactGroupListModel::_shouldHandleMessage(const mavlink_message_t &message, QList<uint32_t> &ids) const
{
    ids.clear();

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HIGH_LATENCY:
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        ids.append(0); // High latency messages do not have a battery id
        return true;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
    {
        mavlink_battery_status_t batteryStatus{};
        mavlink_msg_battery_status_decode(&message, &batteryStatus);
        ids.append(batteryStatus.id);
        return true;
    }
    case MAVLINK_MSG_ID_BATTERY_STATUS_V2:
    {
        mavlink_battery_status_v2_t bs{};
        mavlink_msg_battery_status_v2_decode(&message, &bs);
        ids.append(bs.id);
        return true;
    }
    case MAVLINK_MSG_ID_BATTERY_INFO:
    {
        mavlink_battery_info_t bi{};
        mavlink_msg_battery_info_decode(&message, &bi);
        ids.append(bi.id);
        return true;
    }
    default:
        return false; // Not a message we care about
    }
}

FactGroupWithId *BatteryFactGroupListModel::_createFactGroupWithId(uint32_t id)
{
    return new BatteryFactGroup(id, this);
}

BatteryFactGroup::BatteryFactGroup(uint32_t batteryId, QObject *parent)
    : FactGroupWithId(1000, QStringLiteral(":/json/Vehicle/BatteryFact.json"), parent)
{
    _addFact(&_batteryFunctionFact);
    _addFact(&_batteryTypeFact);
    _addFact(&_voltageFact);
    _addFact(&_currentFact);
    _addFact(&_mahConsumedFact);
    _addFact(&_temperatureFact);
    _addFact(&_percentRemainingFact);
    _addFact(&_timeRemainingFact);
    _addFact(&_timeRemainingStrFact);
    _addFact(&_chargeStateFact);
    _addFact(&_instantPowerFact);
    _addFact(&_capacityRemainingFact);
    _addFact(&_capacityRemainingIsInferredFact);
    _addFact(&_statusFlagsFact);
    _addFact(&_batteryNameFact);
    _addFact(&_serialNumberFact);
    _addFact(&_manufactureDateFact);
    _addFact(&_fullChargeCapacityFact);
    _addFact(&_designCapacityFact);
    _addFact(&_nominalVoltageFact);
    _addFact(&_dischargeMinimumVoltageFact);
    _addFact(&_chargingMinimumVoltageFact);
    _addFact(&_restingMinimumVoltageFact);
    _addFact(&_chargingMaximumVoltageFact);
    _addFact(&_chargingMaximumCurrentFact);
    _addFact(&_dischargeMaximumCurrentFact);
    _addFact(&_dischargeMaximumBurstCurrentFact);
    _addFact(&_cycleCountFact);
    _addFact(&_weightFact);
    _addFact(&_stateOfHealthFact);
    _addFact(&_cellsInSeriesFact);

    _idFact.setRawValue(batteryId);
    _batteryFunctionFact.setRawValue(MAV_BATTERY_FUNCTION_UNKNOWN);
    _batteryTypeFact.setRawValue(MAV_BATTERY_TYPE_UNKNOWN);
    _voltageFact.setRawValue(qQNaN());
    _currentFact.setRawValue(qQNaN());
    _mahConsumedFact.setRawValue(qQNaN());
    _temperatureFact.setRawValue(qQNaN());
    _percentRemainingFact.setRawValue(qQNaN());
    _timeRemainingFact.setRawValue(qQNaN());
    _chargeStateFact.setRawValue(MAV_BATTERY_CHARGE_STATE_UNDEFINED);
    _instantPowerFact.setRawValue(qQNaN());
    _capacityRemainingFact.setRawValue(qQNaN());
    _capacityRemainingIsInferredFact.setRawValue(false);
    _statusFlagsFact.setRawValue(0U);
    _batteryNameFact.setRawValue(QString());
    _serialNumberFact.setRawValue(QString());
    _manufactureDateFact.setRawValue(QString());
    _fullChargeCapacityFact.setRawValue(qQNaN());
    _designCapacityFact.setRawValue(qQNaN());
    _nominalVoltageFact.setRawValue(qQNaN());
    _dischargeMinimumVoltageFact.setRawValue(qQNaN());
    _chargingMinimumVoltageFact.setRawValue(qQNaN());
    _restingMinimumVoltageFact.setRawValue(qQNaN());
    _chargingMaximumVoltageFact.setRawValue(qQNaN());
    _chargingMaximumCurrentFact.setRawValue(qQNaN());
    _dischargeMaximumCurrentFact.setRawValue(qQNaN());
    _dischargeMaximumBurstCurrentFact.setRawValue(qQNaN());
    _cycleCountFact.setRawValue(qQNaN());
    _weightFact.setRawValue(qQNaN());
    _stateOfHealthFact.setRawValue(qQNaN());
    _cellsInSeriesFact.setRawValue(qQNaN());

    (void) connect(&_timeRemainingFact, &Fact::rawValueChanged, this, &BatteryFactGroup::_timeRemainingChanged);
}

void BatteryFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
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
    case MAVLINK_MSG_ID_BATTERY_STATUS_V2:
        _handleBatteryStatusV2(vehicle, message);
        break;
    case MAVLINK_MSG_ID_BATTERY_INFO:
        _handleBatteryInfo(vehicle, message);
        break;
    default:
        break;
    }
}

void BatteryFactGroup::_handleHighLatency(Vehicle * /*vehicle*/, const mavlink_message_t &message)
{
    mavlink_high_latency_t highLatency{};
    mavlink_msg_high_latency_decode(&message, &highLatency);

    percentRemaining()->setRawValue((highLatency.battery_remaining == UINT8_MAX) ? qQNaN() : highLatency.battery_remaining);

    _setTelemetryAvailable(true);
}

void BatteryFactGroup::_handleHighLatency2(Vehicle * /*vehicle*/, const mavlink_message_t &message)
{
    mavlink_high_latency2_t highLatency2{};
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    percentRemaining()->setRawValue((highLatency2.battery == -1) ? qQNaN() : highLatency2.battery);

    _setTelemetryAvailable(true);
}

void BatteryFactGroup::_handleBatteryStatus(Vehicle * /*vehicle*/, const mavlink_message_t &message)
{
    mavlink_battery_status_t batteryStatus{};
    mavlink_msg_battery_status_decode(&message, &batteryStatus);

    if (batteryStatus.id != id()->rawValue().toUInt()) {
        // Disregard battery status messages which are not targeted at this battery id
        return;
    }

    double totalVoltage = qQNaN();
    for (int i = 0; i < 10; i++) {
        const double cellVoltage = ((batteryStatus.voltages[i] == UINT16_MAX)) ? qQNaN() : (static_cast<double>(batteryStatus.voltages[i]) / 1000.0);
        if (qIsNaN(cellVoltage)) {
            break;
        }
        if (i == 0) {
            totalVoltage = cellVoltage;
        } else {
            totalVoltage += cellVoltage;
        }
    }

    for (int i = 0; i < 4; i++) {
        const double cellVoltage = ((batteryStatus.voltages_ext[i] == 0)) ? qQNaN() : (static_cast<double>(batteryStatus.voltages_ext[i]) / 1000.0);
        if (qIsNaN(cellVoltage)) {
            break;
        }
        totalVoltage += cellVoltage;
    }

    function()->setRawValue(batteryStatus.battery_function);
    type()->setRawValue(batteryStatus.type);
    temperature()->setRawValue((batteryStatus.temperature == INT16_MAX) ? qQNaN() : (static_cast<double>(batteryStatus.temperature) / 100.0));
    voltage()->setRawValue(totalVoltage);
    current()->setRawValue((batteryStatus.current_battery == -1) ? qQNaN() : (static_cast<double>(batteryStatus.current_battery) / 100.0));
    mahConsumed()->setRawValue((batteryStatus.current_consumed == -1) ? qQNaN() : batteryStatus.current_consumed);
    percentRemaining()->setRawValue((batteryStatus.battery_remaining == -1) ? qQNaN() : batteryStatus.battery_remaining);
    timeRemaining()->setRawValue((batteryStatus.time_remaining == 0) ? qQNaN() : batteryStatus.time_remaining);
    chargeState()->setRawValue(batteryStatus.charge_state);
    instantPower()->setRawValue(totalVoltage * current()->rawValue().toDouble());

    _setTelemetryAvailable(true);
}

void BatteryFactGroup::_timeRemainingChanged(const QVariant &value)
{
    if (qIsNaN(value.toDouble())) {
        _timeRemainingStrFact.setRawValue("––:––:––");
    } else {
        const int totalSeconds = value.toInt();
        const int hours = totalSeconds / 3600;
        const int minutes = (totalSeconds % 3600) / 60;
        const int seconds = totalSeconds % 60;

        _timeRemainingStrFact.setRawValue(QString::asprintf("%02dH:%02dM:%02dS", hours, minutes, seconds));
    }
}

void BatteryFactGroup::_handleBatteryStatusV2(Vehicle * /*vehicle*/, const mavlink_message_t &message)
{
    mavlink_battery_status_v2_t bs{};
    mavlink_msg_battery_status_v2_decode(&message, &bs);

    if (bs.id != id()->rawValue().toUInt()) {
        return;
    }

    temperature()->setRawValue((bs.temperature == INT16_MAX) ? qQNaN() : static_cast<double>(bs.temperature) / 100.0);

    const double v = qIsNaN(bs.voltage)  ? qQNaN() : static_cast<double>(bs.voltage);
    const double i = qIsNaN(bs.current)  ? qQNaN() : static_cast<double>(bs.current);
    voltage()->setRawValue(v);
    current()->setRawValue(i);

    double consumed  = qIsNaN(bs.capacity_consumed)  ? qQNaN() : static_cast<double>(bs.capacity_consumed);
    double remaining = qIsNaN(bs.capacity_remaining) ? qQNaN() : static_cast<double>(bs.capacity_remaining);
    const double fcc = fullChargeCapacity()->rawValue().toDouble(); // NaN if BATTERY_INFO not yet received

    bool remainingInferred = false;
    if (qIsNaN(remaining) && !qIsNaN(consumed) && !qIsNaN(fcc)) {
        remaining = fcc - consumed;
        remainingInferred = true;
    } else if (qIsNaN(consumed) && !qIsNaN(remaining) && !qIsNaN(fcc)) {
        consumed = fcc - remaining;
        // remaining was measured directly — remainingInferred stays false
    }

    mahConsumed()->setRawValue(qIsNaN(consumed) ? qQNaN() : consumed * 1000.0);
    capacityRemaining()->setRawValue(remaining);
    capacityRemainingIsInferred()->setRawValue(remainingInferred);

    percentRemaining()->setRawValue((bs.percent_remaining == UINT8_MAX) ? qQNaN() : static_cast<double>(bs.percent_remaining));

    statusFlags()->setRawValue(bs.status_flags);

    // Derive a MAV_BATTERY_CHARGE_STATE from the status_flags bitmask for display and audio alerts.
    // Note: LOW/CRITICAL states have no equivalent in BATTERY_STATUS_V2 status_flags.
    constexpr uint32_t faultMask =
        MAV_BATTERY_STATUS_FLAGS_FAULT_BATTERY_OVER_TEMPERATURE  |
        MAV_BATTERY_STATUS_FLAGS_FAULT_BATTERY_UNDER_TEMPERATURE |
        MAV_BATTERY_STATUS_FLAGS_FAULT_BATTERY_OVER_VOLTAGE      |
        MAV_BATTERY_STATUS_FLAGS_FAULT_BATTERY_UNDER_VOLTAGE     |
        MAV_BATTERY_STATUS_FLAGS_FAULT_BATTERY_OVER_CURRENT      |
        MAV_BATTERY_STATUS_FLAGS_FAULT_SHORT_CIRCUIT             |
        MAV_BATTERY_STATUS_FLAGS_FAULT_INCOMPATIBLE_VOLTAGE      |
        MAV_BATTERY_STATUS_FLAGS_FAULT_INCOMPATIBLE_FIRMWARE     |
        MAV_BATTERY_STATUS_FLAGS_FAULT_INCOMPATIBLE_CELLS_LEGACY;

    uint8_t derivedChargeState = MAV_BATTERY_CHARGE_STATE_OK;
    if (bs.status_flags & faultMask) {
        derivedChargeState = MAV_BATTERY_CHARGE_STATE_FAILED;
    } else if (bs.status_flags & MAV_BATTERY_STATUS_FLAGS_NOT_READY_TO_USE) {
        derivedChargeState = MAV_BATTERY_CHARGE_STATE_EMERGENCY;
    } else if (bs.status_flags & MAV_BATTERY_STATUS_FLAGS_CHARGING) {
        derivedChargeState = MAV_BATTERY_CHARGE_STATE_CHARGING;
    }
    chargeState()->setRawValue(derivedChargeState);

    instantPower()->setRawValue(v * i);

    _setTelemetryAvailable(true);
}

void BatteryFactGroup::_handleBatteryInfo(Vehicle * /*vehicle*/, const mavlink_message_t &message)
{
    mavlink_battery_info_t bi{};
    mavlink_msg_battery_info_decode(&message, &bi);

    if (bi.id != id()->rawValue().toUInt()) {
        return;
    }

    function()->setRawValue(bi.battery_function);
    type()->setRawValue(bi.type);

    // Fields using 0 as "not provided" are converted to NaN for consistent invalid-value handling
    auto zeroAsNaN = [](float v) -> double { return (v == 0.0f) ? qQNaN() : static_cast<double>(v); };

    fullChargeCapacity()->setRawValue(qIsNaN(bi.full_charge_capacity) ? qQNaN() : static_cast<double>(bi.full_charge_capacity));
    designCapacity()->setRawValue(zeroAsNaN(bi.design_capacity));
    nominalVoltage()->setRawValue(zeroAsNaN(bi.nominal_voltage));
    dischargeMinimumVoltage()->setRawValue(zeroAsNaN(bi.discharge_minimum_voltage));
    chargingMinimumVoltage()->setRawValue(zeroAsNaN(bi.charging_minimum_voltage));
    restingMinimumVoltage()->setRawValue(zeroAsNaN(bi.resting_minimum_voltage));
    chargingMaximumVoltage()->setRawValue(zeroAsNaN(bi.charging_maximum_voltage));
    chargingMaximumCurrent()->setRawValue(zeroAsNaN(bi.charging_maximum_current));
    dischargeMaximumCurrent()->setRawValue(zeroAsNaN(bi.discharge_maximum_current));
    dischargeMaximumBurstCurrent()->setRawValue(zeroAsNaN(bi.discharge_maximum_burst_current));
    cycleCount()->setRawValue((bi.cycle_count == UINT16_MAX) ? qQNaN() : static_cast<double>(bi.cycle_count));
    weight()->setRawValue(zeroAsNaN(static_cast<float>(bi.weight)));
    stateOfHealth()->setRawValue((bi.state_of_health == 255) ? qQNaN() : static_cast<double>(bi.state_of_health));
    cellsInSeries()->setRawValue((bi.cells_in_series == 0) ? qQNaN() : static_cast<double>(bi.cells_in_series));

    // String fields: fromLatin1 stops at the null terminator; all-zero yields an empty string
    batteryName()->setRawValue(QString::fromLatin1(bi.name));
    serialNumber()->setRawValue(QString::fromLatin1(bi.serial_number));
    manufactureDate()->setRawValue(QString::fromLatin1(bi.manufacture_date));

    _setTelemetryAvailable(true);
}
