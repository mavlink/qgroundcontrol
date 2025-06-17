/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleEscStatusFactGroup.h"
#include "Vehicle.h"

VehicleEscStatusFactGroup::VehicleEscStatusFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/EscStatusFactGroup.json"), parent)
{
    _addFact(&_countFact);
    _addFact(&_infoFact);

    _addFact(&_rpmFirstFact);
    _addFact(&_rpmSecondFact);
    _addFact(&_rpmThirdFact);
    _addFact(&_rpmFourthFact);
    _addFact(&_rpmFifthFact);
    _addFact(&_rpmSixthFact);
    _addFact(&_rpmSeventhFact);
    _addFact(&_rpmEighthFact);

    _addFact(&_currentFirstFact);
    _addFact(&_currentSecondFact);
    _addFact(&_currentThirdFact);
    _addFact(&_currentFourthFact);
    _addFact(&_currentFifthFact);
    _addFact(&_currentSixthFact);
    _addFact(&_currentSeventhFact);
    _addFact(&_currentEighthFact);

    _addFact(&_voltageFirstFact);
    _addFact(&_voltageSecondFact);
    _addFact(&_voltageThirdFact);
    _addFact(&_voltageFourthFact);
    _addFact(&_voltageFifthFact);
    _addFact(&_voltageSixthFact);
    _addFact(&_voltageSeventhFact);
    _addFact(&_voltageEighthFact);

    _addFact(&_temperatureFirstFact);
    _addFact(&_temperatureSecondFact);
    _addFact(&_temperatureThirdFact);
    _addFact(&_temperatureFourthFact);
    _addFact(&_temperatureFifthFact);
    _addFact(&_temperatureSixthFact);
    _addFact(&_temperatureSeventhFact);
    _addFact(&_temperatureEighthFact);

    _addFact(&_errorCountFirstFact);
    _addFact(&_errorCountSecondFact);
    _addFact(&_errorCountThirdFact);
    _addFact(&_errorCountFourthFact);
    _addFact(&_errorCountFifthFact);
    _addFact(&_errorCountSixthFact);
    _addFact(&_errorCountSeventhFact);
    _addFact(&_errorCountEighthFact);

    _addFact(&_failureFlagsFirstFact);
    _addFact(&_failureFlagsSecondFact);
    _addFact(&_failureFlagsThirdFact);
    _addFact(&_failureFlagsFourthFact);
    _addFact(&_failureFlagsFifthFact);
    _addFact(&_failureFlagsSixthFact);
    _addFact(&_failureFlagsSeventhFact);
    _addFact(&_failureFlagsEighthFact);
}

void VehicleEscStatusFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid == MAVLINK_MSG_ID_ESC_STATUS) {
        mavlink_esc_status_t esc_status{};
        mavlink_msg_esc_status_decode(&message, &esc_status);

        // Index - is motors 1-4, index 4 is motors 5-8
        // NOTE: we will discard messgaes with indices other than 0 and 4
        switch (esc_status.index) {
        case 0:
            rpmFirst()->setRawValue(esc_status.rpm[0]);
            rpmSecond()->setRawValue(esc_status.rpm[1]);
            rpmThird()->setRawValue(esc_status.rpm[2]);
            rpmFourth()->setRawValue(esc_status.rpm[3]);
            currentFirst()->setRawValue(esc_status.current[0]);
            currentSecond()->setRawValue(esc_status.current[1]);
            currentThird()->setRawValue(esc_status.current[2]);
            currentFourth()->setRawValue(esc_status.current[3]);
            voltageFirst()->setRawValue(esc_status.voltage[0]);
            voltageSecond()->setRawValue(esc_status.voltage[1]);
            voltageThird()->setRawValue(esc_status.voltage[2]);
            voltageFourth()->setRawValue(esc_status.voltage[3]);
        case 4:
            rpmFifth()->setRawValue(esc_status.rpm[0]);
            rpmSixth()->setRawValue(esc_status.rpm[1]);
            rpmSeventh()->setRawValue(esc_status.rpm[2]);
            rpmEighth()->setRawValue(esc_status.rpm[3]);
            currentFifth()->setRawValue(esc_status.current[0]);
            currentSixth()->setRawValue(esc_status.current[1]);
            currentSeventh()->setRawValue(esc_status.current[2]);
            currentEighth()->setRawValue(esc_status.current[3]);
            voltageFifth()->setRawValue(esc_status.voltage[0]);
            voltageSixth()->setRawValue(esc_status.voltage[1]);
            voltageSeventh()->setRawValue(esc_status.voltage[2]);
            voltageEighth()->setRawValue(esc_status.voltage[3]);
        }

    } else if (message.msgid == MAVLINK_MSG_ID_ESC_INFO) {
        mavlink_esc_info_t esc_info{};
        mavlink_msg_esc_info_decode(&message, &esc_info);
        
        count()->setRawValue(esc_info.count);

        switch (esc_info.index) {
        case 0:
        {
            // Info from first ESC block (4 motors)
            uint8_t value = (info()->rawValue().toUInt() & 0xF0) | uint8_t(esc_info.info << 0);
            info()->setRawValue(value);

            temperatureFirst()->setRawValue(esc_info.temperature[0] / 100.0);
            temperatureSecond()->setRawValue(esc_info.temperature[1] / 100.0);
            temperatureThird()->setRawValue(esc_info.temperature[2] / 100.0);
            temperatureFourth()->setRawValue(esc_info.temperature[3] / 100.0);
            
            errorCountFirst()->setRawValue(esc_info.error_count[0]);
            errorCountSecond()->setRawValue(esc_info.error_count[1]);
            errorCountThird()->setRawValue(esc_info.error_count[2]);
            errorCountFourth()->setRawValue(esc_info.error_count[3]);

            failureFlagsFirst()->setRawValue(esc_info.failure_flags[0]);
            failureFlagsSecond()->setRawValue(esc_info.failure_flags[1]);
            failureFlagsThird()->setRawValue(esc_info.failure_flags[2]);
            failureFlagsFourth()->setRawValue(esc_info.failure_flags[3]);
            break;
        }
        case 4:
        {
            // Info from second ESC block (4 motors)
            uint8_t value = (info()->rawValue().toUInt() & 0x0F) | uint8_t(esc_info.info << 0);
            info()->setRawValue(value);

            temperatureFifth()->setRawValue(esc_info.temperature[0] / 100.0);
            temperatureSixth()->setRawValue(esc_info.temperature[1] / 100.0);
            temperatureSeventh()->setRawValue(esc_info.temperature[2] / 100.0);
            temperatureEighth()->setRawValue(esc_info.temperature[3] / 100.0);
            
            errorCountFifth()->setRawValue(esc_info.error_count[0]);
            errorCountSixth()->setRawValue(esc_info.error_count[1]);
            errorCountSeventh()->setRawValue(esc_info.error_count[2]);
            errorCountEighth()->setRawValue(esc_info.error_count[3]);

            failureFlagsFifth()->setRawValue(esc_info.failure_flags[0]);
            failureFlagsSixth()->setRawValue(esc_info.failure_flags[1]);
            failureFlagsSeventh()->setRawValue(esc_info.failure_flags[2]);
            failureFlagsEighth()->setRawValue(esc_info.failure_flags[3]);
            break;
        }
        default:
            break;
        }

        _setTelemetryAvailable(true);
    }
}
