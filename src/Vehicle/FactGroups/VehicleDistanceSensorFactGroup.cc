/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleDistanceSensorFactGroup.h"
#include "Vehicle.h"

VehicleDistanceSensorFactGroup::VehicleDistanceSensorFactGroup(QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/DistanceSensorFact.json"), parent)
{
    _addFact(&_rotationNoneFact);
    _addFact(&_rotationYaw45Fact);
    _addFact(&_rotationYaw90Fact);
    _addFact(&_rotationYaw135Fact);
    _addFact(&_rotationYaw180Fact);
    _addFact(&_rotationYaw225Fact);
    _addFact(&_rotationYaw270Fact);
    _addFact(&_rotationYaw315Fact);
    _addFact(&_rotationPitch90Fact);
    _addFact(&_rotationPitch270Fact);
    _addFact(&_minDistanceFact);
    _addFact(&_maxDistanceFact);
}

void VehicleDistanceSensorFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    if (message.msgid != MAVLINK_MSG_ID_DISTANCE_SENSOR) {
        return;
    }

    mavlink_distance_sensor_t distanceSensor{};
    mavlink_msg_distance_sensor_decode(&message, &distanceSensor);

    struct orientation2Fact_s {
        const MAV_SENSOR_ORIENTATION orientation;
        Fact *fact;
    };

    const orientation2Fact_s rgOrientation2Fact[] = {
        { MAV_SENSOR_ROTATION_NONE, rotationNone() },
        { MAV_SENSOR_ROTATION_YAW_45, rotationYaw45() },
        { MAV_SENSOR_ROTATION_YAW_90, rotationYaw90() },
        { MAV_SENSOR_ROTATION_YAW_135, rotationYaw135() },
        { MAV_SENSOR_ROTATION_YAW_180, rotationYaw180() },
        { MAV_SENSOR_ROTATION_YAW_225, rotationYaw225() },
        { MAV_SENSOR_ROTATION_YAW_270, rotationYaw270() },
        { MAV_SENSOR_ROTATION_YAW_315, rotationYaw315() },
        { MAV_SENSOR_ROTATION_PITCH_90, rotationPitch90() },
        { MAV_SENSOR_ROTATION_PITCH_270, rotationPitch270() },
    };

    for (const orientation2Fact_s &orientation2Fact : rgOrientation2Fact) {
        if (orientation2Fact.orientation == distanceSensor.orientation) {
            orientation2Fact.fact->setRawValue(distanceSensor.current_distance / 100.0); // cm to meters
            break;
        }
    }

    maxDistance()->setRawValue(distanceSensor.max_distance / 100.0);

    _setTelemetryAvailable(true);
}
