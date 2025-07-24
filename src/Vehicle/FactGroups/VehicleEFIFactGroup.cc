/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleEFIFactGroup.h"
#include "Vehicle.h"

VehicleEFIFactGroup::VehicleEFIFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/EFIFact.json"), parent)
{
    _addFact(&_healthFact);
    _addFact(&_ecuIndexFact);
    _addFact(&_rpmFact);
    _addFact(&_fuelConsumedFact);
    _addFact(&_fuelFlowFact);
    _addFact(&_engineLoadFact);
    _addFact(&_sparkTimeFact);
    _addFact(&_throttlePosFact);
    _addFact(&_baroPressFact);
    _addFact(&_intakePressFact);
    _addFact(&_intakeTempFact);
    _addFact(&_cylinderTempFact);
    _addFact(&_ignTimeFact);
    _addFact(&_exGasTempFact);
    _addFact(&_injTimeFact);
    _addFact(&_throttleOutFact);
    _addFact(&_ptCompFact);

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

void VehicleEFIFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_EFI_STATUS:
        _handleEFIStatus(message);
        break;
    default:
        break;
    }
}

void VehicleEFIFactGroup::_handleEFIStatus(const mavlink_message_t &message)
{
    mavlink_efi_status_t efi{};
    mavlink_msg_efi_status_decode(&message, &efi);

    health()->setRawValue((efi.health == INT8_MAX) ? qQNaN() : efi.health);
    ecuIndex()->setRawValue(efi.ecu_index);
    rpm()->setRawValue(efi.rpm);
    fuelConsumed()->setRawValue(efi.fuel_consumed);
    fuelFlow()->setRawValue(efi.fuel_flow);
    engineLoad()->setRawValue(efi.engine_load);
    throttlePos()->setRawValue(efi.throttle_position);
    sparkTime()->setRawValue(efi.spark_dwell_time);
    baroPress()->setRawValue(efi.barometric_pressure);
    intakePress()->setRawValue(efi.intake_manifold_pressure);
    intakeTemp()->setRawValue(efi.intake_manifold_temperature);
    cylinderTemp()->setRawValue(efi.cylinder_head_temperature);
    ignTime()->setRawValue(efi.ignition_timing);
    injTime()->setRawValue(efi.injection_time);
    exGasTemp()->setRawValue(efi.exhaust_gas_temperature);
    throttleOut()->setRawValue(efi.throttle_out);
    ptComp()->setRawValue(efi.pt_compensation);

    _setTelemetryAvailable(true);
}
