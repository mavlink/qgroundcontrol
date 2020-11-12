/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleDistanceSensorFactGroup.h"
#include "Vehicle.h"

const char* VehicleDistanceSensorFactGroup::_rotationNoneFactName =     "rotationNone";
const char* VehicleDistanceSensorFactGroup::_rotationYaw45FactName =    "rotationYaw45";
const char* VehicleDistanceSensorFactGroup::_rotationYaw90FactName =    "rotationYaw90";
const char* VehicleDistanceSensorFactGroup::_rotationYaw135FactName =   "rotationYaw135";
const char* VehicleDistanceSensorFactGroup::_rotationYaw180FactName =   "rotationYaw180";
const char* VehicleDistanceSensorFactGroup::_rotationYaw225FactName =   "rotationYaw225";
const char* VehicleDistanceSensorFactGroup::_rotationYaw270FactName =   "rotationYaw270";
const char* VehicleDistanceSensorFactGroup::_rotationYaw315FactName =   "rotationYaw315";
const char* VehicleDistanceSensorFactGroup::_rotationPitch90FactName =  "rotationPitch90";
const char* VehicleDistanceSensorFactGroup::_rotationPitch270FactName = "rotationPitch270";
const char* VehicleDistanceSensorFactGroup::_minDistanceFactName =      "minDistance";
const char* VehicleDistanceSensorFactGroup::_maxDistanceFactName =      "maxDistance";

VehicleDistanceSensorFactGroup::VehicleDistanceSensorFactGroup(QObject* parent)
    : FactGroup             (1000, ":/json/Vehicle/DistanceSensorFact.json", parent)
    , _rotationNoneFact     (0, _rotationNoneFactName,      FactMetaData::valueTypeDouble)
    , _rotationYaw45Fact    (0, _rotationYaw45FactName,     FactMetaData::valueTypeDouble)
    , _rotationYaw90Fact    (0, _rotationYaw90FactName,     FactMetaData::valueTypeDouble)
    , _rotationYaw135Fact   (0, _rotationYaw135FactName,    FactMetaData::valueTypeDouble)
    , _rotationYaw180Fact   (0, _rotationYaw180FactName,    FactMetaData::valueTypeDouble)
    , _rotationYaw225Fact   (0, _rotationYaw225FactName,    FactMetaData::valueTypeDouble)
    , _rotationYaw270Fact   (0, _rotationYaw270FactName,    FactMetaData::valueTypeDouble)
    , _rotationYaw315Fact   (0, _rotationYaw315FactName,    FactMetaData::valueTypeDouble)
    , _rotationPitch90Fact  (0, _rotationPitch90FactName,   FactMetaData::valueTypeDouble)
    , _rotationPitch270Fact (0, _rotationPitch270FactName,  FactMetaData::valueTypeDouble)
    , _minDistanceFact      (0, _minDistanceFactName,       FactMetaData::valueTypeDouble)
    , _maxDistanceFact      (0, _maxDistanceFactName,       FactMetaData::valueTypeDouble)
{
    _addFact(&_rotationNoneFact,        _rotationNoneFactName);
    _addFact(&_rotationYaw45Fact,       _rotationYaw45FactName);
    _addFact(&_rotationYaw90Fact,       _rotationYaw90FactName);
    _addFact(&_rotationYaw135Fact,      _rotationYaw135FactName);
    _addFact(&_rotationYaw180Fact,      _rotationYaw180FactName);
    _addFact(&_rotationYaw225Fact,      _rotationYaw225FactName);
    _addFact(&_rotationYaw270Fact,      _rotationYaw270FactName);
    _addFact(&_rotationYaw315Fact,      _rotationYaw315FactName);
    _addFact(&_rotationPitch90Fact,     _rotationPitch90FactName);
    _addFact(&_rotationPitch270Fact,    _rotationPitch270FactName);
    _addFact(&_minDistanceFact,         _minDistanceFactName);
    _addFact(&_maxDistanceFact,         _maxDistanceFactName);
}

void VehicleDistanceSensorFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_DISTANCE_SENSOR) {
        return;
    }

    mavlink_distance_sensor_t distanceSensor;

    mavlink_msg_distance_sensor_decode(&message, &distanceSensor);

    struct orientation2Fact_s {
        MAV_SENSOR_ORIENTATION  orientation;
        Fact*                   fact;
    };

    orientation2Fact_s rgOrientation2Fact[] =
    {
        { MAV_SENSOR_ROTATION_NONE,         rotationNone() },
        { MAV_SENSOR_ROTATION_YAW_45,       rotationYaw45() },
        { MAV_SENSOR_ROTATION_YAW_90,       rotationYaw90() },
        { MAV_SENSOR_ROTATION_YAW_135,      rotationYaw135() },
        { MAV_SENSOR_ROTATION_YAW_180,      rotationYaw180() },
        { MAV_SENSOR_ROTATION_YAW_225,      rotationYaw225() },
        { MAV_SENSOR_ROTATION_YAW_270,      rotationYaw270() },
        { MAV_SENSOR_ROTATION_YAW_315,      rotationYaw315() },
        { MAV_SENSOR_ROTATION_PITCH_90,     rotationPitch90() },
        { MAV_SENSOR_ROTATION_PITCH_270,    rotationPitch270() },
    };

    for (size_t i=0; i<sizeof(rgOrientation2Fact)/sizeof(rgOrientation2Fact[0]); i++) {
        const orientation2Fact_s& orientation2Fact = rgOrientation2Fact[i];
        if (orientation2Fact.orientation == distanceSensor.orientation) {
            orientation2Fact.fact->setRawValue(distanceSensor.current_distance / 100.0); // cm to meters
        }
    }

    maxDistance()->setRawValue(distanceSensor.max_distance / 100.0);
    _setTelemetryAvailable(true);
}
