#include "VehicleEFIFactGroup.h"
#include "Vehicle.h"

const char* VehicleEFIFactGroup::_healthFactName =          "health";
const char* VehicleEFIFactGroup::_ecuIndexFactName =        "ecuIndex";
const char* VehicleEFIFactGroup::_rpmFactName =             "rpm";
const char* VehicleEFIFactGroup::_fuelConsumedFactName =    "fuelConsumed";
const char* VehicleEFIFactGroup::_fuelFlowFactName =        "fuelFlow";
const char* VehicleEFIFactGroup::_engineLoadFactName =      "engineLoad";
const char* VehicleEFIFactGroup::_throttlePosFactName =     "throttlePos";
const char* VehicleEFIFactGroup::_sparkTimeFactName =       "sparkTime";
const char* VehicleEFIFactGroup::_baroPressFactName =       "baroPress";
const char* VehicleEFIFactGroup::_intakePressFactName =     "intakePress";
const char* VehicleEFIFactGroup::_intakeTempFactName =      "intakeTemp";
const char* VehicleEFIFactGroup::_cylinderTempFactName =    "cylinderTemp";
const char* VehicleEFIFactGroup::_ignTimeFactName =         "ignTime";
const char* VehicleEFIFactGroup::_injTimeFactName =         "injTime";
const char* VehicleEFIFactGroup::_exGasTempFactName =       "exGasTemp";
const char* VehicleEFIFactGroup::_throttleOutFactName =     "throttleOut";
const char* VehicleEFIFactGroup::_ptCompFactName =          "ptComp";


VehicleEFIFactGroup::VehicleEFIFactGroup(QObject* parent)
    : FactGroup(1000, ":/json/Vehicle/EFIFact.json", parent)
    , _healthFact           (0, _healthFactName,            FactMetaData::valueTypeInt8)
    , _ecuIndexFact         (0, _ecuIndexFactName,          FactMetaData::valueTypeFloat)
    , _rpmFact              (0, _rpmFactName,               FactMetaData::valueTypeFloat)
    , _fuelConsumedFact     (0, _fuelConsumedFactName,      FactMetaData::valueTypeFloat)
    , _fuelFlowFact         (0, _fuelFlowFactName,          FactMetaData::valueTypeFloat)
    , _engineLoadFact       (0, _engineLoadFactName,        FactMetaData::valueTypeFloat)
    , _throttlePosFact      (0, _throttlePosFactName,       FactMetaData::valueTypeFloat)
    , _sparkTimeFact        (0, _sparkTimeFactName,         FactMetaData::valueTypeFloat)
    , _baroPressFact        (0, _baroPressFactName,         FactMetaData::valueTypeFloat)
    , _intakePressFact      (0, _intakePressFactName,       FactMetaData::valueTypeFloat)
    , _intakeTempFact       (0, _intakeTempFactName,        FactMetaData::valueTypeFloat)
    , _cylinderTempFact     (0, _cylinderTempFactName,      FactMetaData::valueTypeFloat)
    , _ignTimeFact          (0, _ignTimeFactName,           FactMetaData::valueTypeFloat)
    , _injTimeFact          (0, _injTimeFactName,           FactMetaData::valueTypeFloat)
    , _exGasTempFact        (0, _exGasTempFactName,         FactMetaData::valueTypeFloat)
    , _throttleOutFact      (0, _throttleOutFactName,       FactMetaData::valueTypeFloat)
    , _ptCompFact           (0, _ptCompFactName,            FactMetaData::valueTypeFloat)
{
    _addFact(&_healthFact,          _healthFactName);
    _addFact(&_ecuIndexFact,        _ecuIndexFactName);
    _addFact(&_rpmFact,             _rpmFactName);
    _addFact(&_fuelConsumedFact,    _fuelConsumedFactName);
    _addFact(&_fuelFlowFact,        _fuelFlowFactName);
    _addFact(&_engineLoadFact,      _engineLoadFactName);
    _addFact(&_sparkTimeFact,       _sparkTimeFactName);
    _addFact(&_throttlePosFact,     _throttlePosFactName);
    _addFact(&_baroPressFact,       _baroPressFactName);
    _addFact(&_intakePressFact,     _intakePressFactName);
    _addFact(&_intakeTempFact,      _intakeTempFactName);
    _addFact(&_cylinderTempFact,    _cylinderTempFactName);
    _addFact(&_ignTimeFact,         _ignTimeFactName);
    _addFact(&_exGasTempFact,       _exGasTempFactName);
    _addFact(&_injTimeFact,         _injTimeFactName);
    _addFact(&_throttleOutFact,     _throttleOutFactName);
    _addFact(&_ptCompFact,          _ptCompFactName);

    // Start out as not available "--.--"
    _healthFact.setRawValue(qQNaN());
    _ecuIndexFact.setRawValue(qQNaN());
    _rpmFact.setRawValue(qQNaN());
    _fuelConsumedFact.setRawValue(qQNaN());
    _fuelFlowFact.setRawValue(qQNaN());
    _engineLoadFact.setRawValue(qQNaN());
    _sparkTimeFact.setRawValue(qQNaN());
    _throttlePosFact.setRawValue(qQNaN());
    _baroPressFact.setRawValue(qQNaN());
    _intakePressFact.setRawValue(qQNaN());
    _intakeTempFact.setRawValue(qQNaN());
    _cylinderTempFact.setRawValue(qQNaN());
    _ignTimeFact.setRawValue(qQNaN());
    _exGasTempFact.setRawValue(qQNaN());
    _injTimeFact.setRawValue(qQNaN());
    _throttleOutFact.setRawValue(qQNaN());
    _ptCompFact.setRawValue(qQNaN());
}

void VehicleEFIFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_EFI_STATUS:
        _handleEFIStatus(message);
        break;
    default:
        break;
    }
}

void VehicleEFIFactGroup::_handleEFIStatus(mavlink_message_t& message)
{
    mavlink_efi_status_t efi;
    mavlink_msg_efi_status_decode(&message, &efi);

    health()->setRawValue           (efi.health == INT8_MAX ? qQNaN() : efi.health);
    ecuIndex()->setRawValue         (efi.ecu_index);
    rpm()->setRawValue              (efi.rpm);
    fuelConsumed()->setRawValue     (efi.fuel_consumed);
    fuelFlow()->setRawValue         (efi.fuel_flow);
    engineLoad()->setRawValue       (efi.engine_load);
    throttlePos()->setRawValue      (efi.throttle_position);
    sparkTime()->setRawValue        (efi.spark_dwell_time);
    baroPress()->setRawValue        (efi.barometric_pressure);
    intakePress()->setRawValue      (efi.intake_manifold_pressure);
    intakeTemp()->setRawValue       (efi.intake_manifold_temperature);
    cylinderTemp()->setRawValue     (efi.cylinder_head_temperature);
    ignTime()->setRawValue          (efi.ignition_timing);
    injTime()->setRawValue          (efi.injection_time);
    exGasTemp()->setRawValue        (efi.exhaust_gas_temperature);
    throttleOut()->setRawValue      (efi.throttle_out);
    ptComp()->setRawValue           (efi.pt_compensation);
}
