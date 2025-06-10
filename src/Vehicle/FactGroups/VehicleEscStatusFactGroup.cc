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
    _addFact(&_indexFact);
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
}

void VehicleEscStatusFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid == MAVLINK_MSG_ID_ESC_STATUS) {
        mavlink_esc_status_t content{};
        mavlink_msg_esc_status_decode(&message, &content);

        index()->setRawValue(content.index);

        // ESC_STATUS supports up to 8 motors in the arrays
        rpmFirst()->setRawValue(content.rpm[0]);
        rpmSecond()->setRawValue(content.rpm[1]);
        rpmThird()->setRawValue(content.rpm[2]);
        rpmFourth()->setRawValue(content.rpm[3]);
        rpmFifth()->setRawValue(content.rpm[4]);
        rpmSixth()->setRawValue(content.rpm[5]);
        rpmSeventh()->setRawValue(content.rpm[6]);
        rpmEighth()->setRawValue(content.rpm[7]);

        currentFirst()->setRawValue(content.current[0]);
        currentSecond()->setRawValue(content.current[1]);
        currentThird()->setRawValue(content.current[2]);
        currentFourth()->setRawValue(content.current[3]);
        currentFifth()->setRawValue(content.current[4]);
        currentSixth()->setRawValue(content.current[5]);
        currentSeventh()->setRawValue(content.current[6]);
        currentEighth()->setRawValue(content.current[7]);

        voltageFirst()->setRawValue(content.voltage[0]);
        voltageSecond()->setRawValue(content.voltage[1]);
        voltageThird()->setRawValue(content.voltage[2]);
        voltageFourth()->setRawValue(content.voltage[3]);
        voltageFifth()->setRawValue(content.voltage[4]);
        voltageSixth()->setRawValue(content.voltage[5]);
        voltageSeventh()->setRawValue(content.voltage[6]);
        voltageEighth()->setRawValue(content.voltage[7]);

        _setTelemetryAvailable(true);
    } else if (message.msgid == MAVLINK_MSG_ID_ESC_INFO) {
        mavlink_esc_info_t content{};
        mavlink_msg_esc_info_decode(&message, &content);

        // ESC_INFO contains count, info bitmask, temperature and error count data
        // The index field tells us which ESC this data is for
        
        // Store the count and info bitmask (these are global for this ESC index)
        count()->setRawValue(content.count);
        info()->setRawValue(content.info);
        
        switch (content.index) {
        case 0:
            // Temperature is in centi-celsius, convert to celsius
            temperatureFirst()->setRawValue(content.temperature[0] / 100.0);
            temperatureSecond()->setRawValue(content.temperature[1] / 100.0);
            temperatureThird()->setRawValue(content.temperature[2] / 100.0);
            temperatureFourth()->setRawValue(content.temperature[3] / 100.0);
            
            // Error counts
            errorCountFirst()->setRawValue(content.error_count[0]);
            errorCountSecond()->setRawValue(content.error_count[1]);
            errorCountThird()->setRawValue(content.error_count[2]);
            errorCountFourth()->setRawValue(content.error_count[3]);
            break;
        case 1:
            // ESC index 1 (motors 5-8)
            temperatureFifth()->setRawValue(content.temperature[0] / 100.0);
            temperatureSixth()->setRawValue(content.temperature[1] / 100.0);
            temperatureSeventh()->setRawValue(content.temperature[2] / 100.0);
            temperatureEighth()->setRawValue(content.temperature[3] / 100.0);
            
            errorCountFifth()->setRawValue(content.error_count[0]);
            errorCountSixth()->setRawValue(content.error_count[1]);
            errorCountSeventh()->setRawValue(content.error_count[2]);
            errorCountEighth()->setRawValue(content.error_count[3]);
            break;
        default:
            // Support for additional ESC indices can be added here
            break;
        }
    }
}
