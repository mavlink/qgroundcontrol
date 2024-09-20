/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleGeneratorFactGroup.h"
#include "Vehicle.h"
#include <bitset>

VehicleGeneratorFactGroup::VehicleGeneratorFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/GeneratorFact.json", parent)
    , _statusFact               (0, _statusFactName,                FactMetaData::valueTypeUint64)
    , _genSpeedFact             (0, _genSpeedFactName,              FactMetaData::valueTypeUint16)
    , _batteryCurrentFact       (0, _batteryCurrentFactName,        FactMetaData::valueTypeFloat)
    , _loadCurrentFact          (0, _loadCurrentFactName,           FactMetaData::valueTypeFloat)
    , _powerGeneratedFact       (0, _powerGeneratedFactName,        FactMetaData::valueTypeFloat)
    , _busVoltageFact           (0, _busVoltageFactName,            FactMetaData::valueTypeFloat)
    , _rectifierTempFact        (0, _rectifierTempFactName,         FactMetaData::valueTypeInt16)
    , _batCurrentSetpointFact   (0, _batCurrentSetpointFactName,    FactMetaData::valueTypeFloat)
    , _genTempFact              (0, _genTempFactName,               FactMetaData::valueTypeInt16)
    , _runtimeFact              (0, _runtimeFactName,               FactMetaData::valueTypeUint32)
    , _timeMaintenanceFact      (0, _timeMaintenanceFactName,       FactMetaData::valueTypeInt32)
{
    _addFact(&_statusFact,              _statusFactName);
    _addFact(&_genSpeedFact,            _genSpeedFactName);
    _addFact(&_batteryCurrentFact,      _batteryCurrentFactName);
    _addFact(&_loadCurrentFact,         _loadCurrentFactName);
    _addFact(&_powerGeneratedFact,      _powerGeneratedFactName);
    _addFact(&_busVoltageFact,          _busVoltageFactName);
    _addFact(&_batCurrentSetpointFact,  _batCurrentSetpointFactName);
    _addFact(&_rectifierTempFact,       _rectifierTempFactName);
    _addFact(&_genTempFact,             _genTempFactName);
    _addFact(&_runtimeFact,             _runtimeFactName);
    _addFact(&_timeMaintenanceFact,     _timeMaintenanceFactName);

    // Start out as not available "--.--"
    _statusFact.setRawValue(qQNaN());
    _genSpeedFact.setRawValue(qQNaN());
    _batteryCurrentFact.setRawValue(qQNaN());
    _loadCurrentFact.setRawValue(qQNaN());
    _powerGeneratedFact.setRawValue(qQNaN());
    _busVoltageFact.setRawValue(qQNaN());
    _batCurrentSetpointFact.setRawValue(qQNaN());
    _rectifierTempFact.setRawValue(qQNaN());
    _genTempFact.setRawValue(qQNaN());
    _runtimeFact.setRawValue(qQNaN());
    _timeMaintenanceFact.setRawValue(qQNaN());
}

void VehicleGeneratorFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_GENERATOR_STATUS:
        _handleGeneratorStatus(message);
        break;
    default:
        break;
    }
}

void VehicleGeneratorFactGroup::_handleGeneratorStatus(mavlink_message_t& message)
{
    mavlink_generator_status_t generator;
    mavlink_msg_generator_status_decode(&message, &generator);

    status()->setRawValue               (generator.status == UINT16_MAX ? qQNaN() : generator.status);
    _updateGeneratorFlags();
    genSpeed()->setRawValue             (generator.generator_speed == UINT16_MAX ? qQNaN() : generator.generator_speed);
    batteryCurrent()->setRawValue       (generator.battery_current);
    loadCurrent()->setRawValue          (generator.load_current);
    powerGenerated()->setRawValue       (generator.power_generated);
    busVoltage()->setRawValue           (generator.bus_voltage);
    rectifierTemp()->setRawValue        (generator.rectifier_temperature == INT16_MAX ? qQNaN() : generator.rectifier_temperature);
    batCurrentSetpoint()->setRawValue   (generator.bat_current_setpoint);
    genTemp()->setRawValue              (generator.generator_temperature == INT16_MAX ? qQNaN() : generator.generator_temperature);
    runtime()->setRawValue              (generator.runtime == UINT32_MAX ? qQNaN() : generator.runtime);
    timeMaintenance()->setRawValue      (generator.time_until_maintenance == INT32_MAX ? qQNaN() : generator.time_until_maintenance);
}

void VehicleGeneratorFactGroup::_updateGeneratorFlags() {

    // Check the status received, and convert it to a List with the state of each flag
    int statusFlag = _statusFact.rawValue().toInt();

    // No need to update the list if we have the same flags
    if ( statusFlag == _prevFlag) {
        return;
    }

    _prevFlag = statusFlag;
    _flagsListGenerator.clear();

    std::bitset<23> bitsetFlags(statusFlag);

    for (size_t i=0; i<bitsetFlags.size(); i++) {
        if (bitsetFlags.test(i)) {
            _flagsListGenerator.append(1);
        } else {
            _flagsListGenerator.append(0);
        }
    }
    emit flagsListGeneratorChanged();
}