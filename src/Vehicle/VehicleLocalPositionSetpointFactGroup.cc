/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleLocalPositionSetpointFactGroup.h"
#include "Vehicle.h"

#include <QtMath>

const char* VehicleLocalPositionSetpointFactGroup::_xFactName =     "x";
const char* VehicleLocalPositionSetpointFactGroup::_yFactName =     "y";
const char* VehicleLocalPositionSetpointFactGroup::_zFactName =     "z";
const char* VehicleLocalPositionSetpointFactGroup::_vxFactName =    "vx";
const char* VehicleLocalPositionSetpointFactGroup::_vyFactName =    "vy";
const char* VehicleLocalPositionSetpointFactGroup::_vzFactName =    "vz";

VehicleLocalPositionSetpointFactGroup::VehicleLocalPositionSetpointFactGroup(QObject* parent)
    : FactGroup     (1000, ":/json/Vehicle/LocalPositionSetpointFact.json", parent)
    , _xFact    (0, _xFactName,     FactMetaData::valueTypeDouble)
    , _yFact    (0, _yFactName,     FactMetaData::valueTypeDouble)
    , _zFact    (0, _zFactName,     FactMetaData::valueTypeDouble)
    , _vxFact   (0, _vxFactName,    FactMetaData::valueTypeDouble)
    , _vyFact   (0, _vyFactName,    FactMetaData::valueTypeDouble)
    , _vzFact   (0, _vzFactName,    FactMetaData::valueTypeDouble)
{
    _addFact(&_xFact,      _xFactName);
    _addFact(&_yFact,      _yFactName);
    _addFact(&_zFact,      _zFactName);
    _addFact(&_vxFact,     _vxFactName);
    _addFact(&_vyFact,     _vyFactName);
    _addFact(&_vzFact,     _vzFactName);

    // Start out as not available "--.--"
    _xFact.setRawValue(qQNaN());
    _yFact.setRawValue(qQNaN());
    _zFact.setRawValue(qQNaN());
    _vxFact.setRawValue(qQNaN());
    _vyFact.setRawValue(qQNaN());
    _vzFact.setRawValue(qQNaN());
}

void VehicleLocalPositionSetpointFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED) {
        return;
    }

    mavlink_position_target_local_ned_t localPosition;
    mavlink_msg_position_target_local_ned_decode(&message, &localPosition);

    x()->setRawValue(localPosition.x);
    y()->setRawValue(localPosition.y);
    z()->setRawValue(localPosition.z);

    vx()->setRawValue(localPosition.vx);
    vy()->setRawValue(localPosition.vy);
    vz()->setRawValue(localPosition.vz);

    _setTelemetryAvailable(true);
}
