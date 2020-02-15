/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Vehicle.h"
#include "VehicleObjectAvoidance.h"
#include "ParameterManager.h"
#include <cmath>

static const char* kColPrevParam = "CP_DIST";

//-----------------------------------------------------------------------------
VehicleObjectAvoidance::VehicleObjectAvoidance(Vehicle *vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
{
}

//-----------------------------------------------------------------------------
void
VehicleObjectAvoidance::update(mavlink_obstacle_distance_t* message)
{
    //-- Collect raw data
    if(std::isfinite(message->increment_f) && message->increment_f > 0) {
        _increment = static_cast<qreal>(message->increment_f);
    } else {
        _increment = static_cast<qreal>(message->increment);
    }
    _minDistance = message->min_distance;
    _maxDistance = message->max_distance;
    _angleOffset = static_cast<qreal>(message->angle_offset);
    if(_distances.count() == 0) {
        for(int i = 0; i < MAVLINK_MSG_OBSTACLE_DISTANCE_FIELD_DISTANCES_LEN; i++) {
            _distances.append(static_cast<int>(message->distances[i]));
        }
    } else {
        for(int i = 0; i < MAVLINK_MSG_OBSTACLE_DISTANCE_FIELD_DISTANCES_LEN; i++) {
            _distances[i] = static_cast<int>(message->distances[i]);
        }
    }
    //-- Create a plottable grid with found objects
    _objGrid.clear();
    _objDistance.clear();
    auto* sp = qobject_cast<VehicleSetpointFactGroup*>(_vehicle->setpointFactGroup());
    qreal startAngle = sp->yaw()->rawValue().toDouble() + _angleOffset;
    for(int i = 0; i < MAVLINK_MSG_OBSTACLE_DISTANCE_FIELD_DISTANCES_LEN; i++) {
        if(_distances[i] < _maxDistance && message->distances[i] != UINT16_MAX) {
            qreal d = static_cast<qreal>(_distances[i]);
            d = d / static_cast<qreal>(_maxDistance);
            qreal a = (_increment * i) - startAngle;
            if(a < 0) a = a + 360;
            qreal rd = (M_PI / 180.0) * a;
            QPointF p = QPointF(d * cos(rd), d * sin(rd));
            _objGrid.append(p);
            _objDistance.append(d);
        }
    }
    emit objectAvoidanceChanged();
}

//-----------------------------------------------------------------------------
bool
VehicleObjectAvoidance::enabled()
{
    uint8_t id = static_cast<uint8_t>(_vehicle->id());
    if(_vehicle->parameterManager()->parameterExists(id, kColPrevParam)) {
        return _vehicle->parameterManager()->getParameter(id, kColPrevParam)->rawValue().toInt() >= 0;
    }
    return false;
}

//-----------------------------------------------------------------------------
void
VehicleObjectAvoidance::start(int minDistance)
{
    uint8_t id = static_cast<uint8_t>(_vehicle->id());
    if(_vehicle->parameterManager()->parameterExists(id, kColPrevParam)) {
        _vehicle->parameterManager()->getParameter(id, kColPrevParam)->setRawValue(minDistance);
        emit objectAvoidanceChanged();
    }
}

//-----------------------------------------------------------------------------
void
VehicleObjectAvoidance::stop()
{
    uint8_t id = static_cast<uint8_t>(_vehicle->id());
    if(_vehicle->parameterManager()->parameterExists(id, kColPrevParam)) {
        _vehicle->parameterManager()->getParameter(id, kColPrevParam)->setRawValue(-1);
        emit objectAvoidanceChanged();
    }
}

//-----------------------------------------------------------------------------
QPointF
VehicleObjectAvoidance::grid(int i)
{
    if(i < _objGrid.count() && i >= 0) {
        return _objGrid[i];
    }
    return QPointF(0,0);
}

//-----------------------------------------------------------------------------
qreal
VehicleObjectAvoidance::distance(int i)
{
    if(i < _objDistance.count() && i >= 0) {
        return _objDistance[i];
    }
    return 0;
}
