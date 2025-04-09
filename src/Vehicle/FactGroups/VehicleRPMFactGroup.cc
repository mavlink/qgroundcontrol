/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleSetpointFactGroup.h"
#include "Vehicle.h"

VehicleRPMFactGroup::VehicleRPMFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/RPMFact.json"), parent)
{
    _addFact(&_rpm1Fact);
    _addFact(&_rpm2Fact);
    _addFact(&_rpm3Fact);
    _addFact(&_rpm4Fact);

    _rpm1Fact.setRawValue(qQNaN());
    _rpm2Fact.setRawValue(qQNaN());
    _rpm3Fact.setRawValue(qQNaN());
    _rpm4Fact.setRawValue(qQNaN());
}

void VehicleRPMFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid != MAVLINK_MSG_ID_RAW_RPM) {
        return;
    }

    mavlink_raw_rpm_t raw_rpm{};
    mavlink_msg_raw_rpm_decode(&message, &raw_rpm);
    switch (raw_rpm.index) {
        case 0:
            rpm1()->setRawValue(raw_rpm.frequency);
            break;
        case 1:
            rpm2()->setRawValue(raw_rpm.frequency);
            break;
        case 2:
            rpm3()->setRawValue(raw_rpm.frequency);
            break;
        case 3:
            rpm4()->setRawValue(raw_rpm.frequency);
            break;
        default:
            break;

    }

    _setTelemetryAvailable(true);
}
