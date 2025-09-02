#include "VehicleFlowSensorFactGroup.h"
#include "Vehicle.h"

Q_LOGGING_CATEGORY(FlowSensorLog, "qgc.vehicle.flowsensor")

VehicleFlowSensorFactGroup::VehicleFlowSensorFactGroup(QObject* parent)
    : FactGroup(0, ":/json/Vehicle/FlowSensorFact.json", parent)
  
{
    _addFact(&_flowRateFact);
    _addFact(&_pulseCountFact);


    _flowRateFact.setRawValue(qQNaN());
    _pulseCountFact.setRawValue(qQNaN());
}

// // ✅ These do the updates:
// void FlowSensorFactGroup::setFlowRate(double rate)
// {
//     _flowRateFact.setRawValue(rate);
// }

// void FlowSensorFactGroup::setPulseCount(int count)
// {
//     _pulseCountFact.setRawValue(count);
// }


void VehicleFlowSensorFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    qCDebug(FlowSensorLog) << "handleMessage received msgid in FlowsensorFactGroup:" << message.msgid;
    if (message.msgid != MAVLINK_MSG_ID_FLOW_SENSOR) {
        return; // Ignore messages that are not FLOW_SENSOR
        qCDebug(FlowSensorLog) << "handleMessage received msgid in FlowsensorFactGroup:" << message.msgid;
    }
    switch (message.msgid) {
        case MAVLINK_MSG_ID_FLOW_SENSOR:
        qCDebug(FlowSensorLog) << "FlowSensor Message received in FlowsensorFactGroup:" << message.msgid;
           _handleFlowSensor(message);
           break;
        default:
            break;
        }
    

}

void VehicleFlowSensorFactGroup::_handleFlowSensor(const mavlink_message_t &message)
{
    mavlink_flow_sensor_t flowSensor{};
    mavlink_msg_flow_sensor_decode(&message, &flowSensor);
   
   
    flowRate()->setRawValue(flowSensor.flow_rate_lpm);
    pulseCount()->setRawValue(flowSensor.pulse_count);
     // ✅ DEBUG: print raw MAVLink data
     qCDebug(FlowSensorLog) << "Decoded flow_rate_lpm:" << flowSensor.flow_rate_lpm
     << "pulse_count:" << flowSensor.pulse_count;

    _setTelemetryAvailable(true);
}

