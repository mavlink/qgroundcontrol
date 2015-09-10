/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief MissionItem class
 *
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include <QStringList>
#include <QDebug>

#include "MissionItem.h"

const MissionItem::MavCmd2Name_t  MissionItem::_rgMavCmd2Name[_cMavCmd2Name] = {
    { MAV_CMD_NAV_WAYPOINT,         "Waypoint" },
    { MAV_CMD_NAV_LOITER_UNLIM,     "Loiter" },
    { MAV_CMD_NAV_LOITER_TURNS,     "Loiter (turns)" },
    { MAV_CMD_NAV_LOITER_TIME,      "Loiter (seconds)" },
    { MAV_CMD_NAV_RETURN_TO_LAUNCH, "Return Home" },
    { MAV_CMD_NAV_LAND,             "Land" },
    { MAV_CMD_NAV_TAKEOFF,          "Takeoff" },
    { MAV_CMD_CONDITION_DELAY,      "Delay" },
    { MAV_CMD_DO_JUMP,              "Jump To Command" },
};

MissionItem::MissionItem(QObject*       parent,
                         int            sequenceNumber,
                         QGeoCoordinate coordinate,
                         double         param1,
                         double         param2,
                         double         param3,
                         double         param4,
                         bool           autocontinue,
                         bool           isCurrentItem,
                         int            frame,
                         int            command)
    : QObject(parent)
    , _sequenceNumber(sequenceNumber)
    , _coordinate(coordinate)
    , _frame(frame)
    , _command((MavlinkQmlSingleton::Qml_MAV_CMD)command)
    , _autocontinue(autocontinue)
    , _isCurrentItem(isCurrentItem)
    , _reachedTime(0)
    , _yawRadiansFact(NULL)
{

    _yawRadiansFact         = new Fact(0, "Heading:", FactMetaData::valueTypeDouble, this);
    _loiterOrbitRadiusFact  = new Fact(0, "Radius:",  FactMetaData::valueTypeDouble, this);
    _param1Fact             = new Fact(0, QString(),  FactMetaData::valueTypeDouble, this);
    _param2Fact             = new Fact(0, QString(),  FactMetaData::valueTypeDouble, this);
    
    setParam1(param1);
    setParam2(param2);
    setYawRadians(param4);
    setLoiterOrbitRadius(param3);
    
    // FIXME: Need to fill out more meta data
    
    FactMetaData* yawMetaData = new FactMetaData(FactMetaData::valueTypeDouble, this);
    yawMetaData->setUnits("degrees");
    
    _pitchMetaData = new FactMetaData(FactMetaData::valueTypeDouble, this);
    _pitchMetaData->setUnits("degrees");
    
    _acceptanceRadiusMetaData = new FactMetaData(FactMetaData::valueTypeDouble, this);
    _acceptanceRadiusMetaData->setUnits("meters");
    
    _holdTimeMetaData = new FactMetaData(FactMetaData::valueTypeDouble, this);
    _holdTimeMetaData->setUnits("seconds");
    
    FactMetaData* loiterOrbitRadiusMetaData = new FactMetaData(FactMetaData::valueTypeDouble, this);
    loiterOrbitRadiusMetaData->setUnits("meters");
    
    _loiterTurnsMetaData = new FactMetaData(FactMetaData::valueTypeInt32, this);
    _loiterTurnsMetaData->setUnits("count");
    
    _loiterSecondsMetaData = new FactMetaData(FactMetaData::valueTypeDouble, this);
    _loiterSecondsMetaData->setUnits("seconds");
    
    _delaySecondsMetaData = new FactMetaData(FactMetaData::valueTypeDouble, this);
    _delaySecondsMetaData->setUnits("seconds");
    
    _jumpSequenceMetaData = new FactMetaData(FactMetaData::valueTypeInt32, this);
    _jumpSequenceMetaData->setUnits("#");
    
    _jumpRepeatMetaData = new FactMetaData(FactMetaData::valueTypeInt32, this);
    _jumpRepeatMetaData->setUnits("count");
    
    _yawRadiansFact->setMetaData(yawMetaData);
    _loiterOrbitRadiusFact->setMetaData(loiterOrbitRadiusMetaData);
}

MissionItem::MissionItem(const MissionItem& other, QObject* parent)
    : QObject(parent)
{
    _yawRadiansFact         = new Fact(this);
    _loiterOrbitRadiusFact  = new Fact(this);
    _param1Fact             = new Fact(this);
    _param2Fact             = new Fact(this);
    
    _pitchMetaData = new FactMetaData(this);
    
    _acceptanceRadiusMetaData = new FactMetaData(this);
    _holdTimeMetaData = new FactMetaData(this);
    _loiterTurnsMetaData = new FactMetaData(this);
    _loiterSecondsMetaData = new FactMetaData(this);
    _delaySecondsMetaData = new FactMetaData(this);
    _jumpSequenceMetaData = new FactMetaData(this);
    _jumpRepeatMetaData = new FactMetaData(this);

    *this = other;
}

MissionItem::~MissionItem()
{    
}

const MissionItem& MissionItem::operator=(const MissionItem& other)
{
    _sequenceNumber = other._sequenceNumber;
    _isCurrentItem  = other._isCurrentItem;
    _coordinate     = other._coordinate;
    _frame          = other._frame;
    _command        = other._command;
    _autocontinue   = other._autocontinue;
    _reachedTime    = other._reachedTime;
    
    *_yawRadiansFact            = *other._yawRadiansFact;
    *_loiterOrbitRadiusFact     = *other._loiterOrbitRadiusFact;
    *_param1Fact                = *other._param1Fact;
    *_param2Fact                = *other._param2Fact;
    
    *_pitchMetaData             = *other._pitchMetaData;
    *_acceptanceRadiusMetaData  = *other._acceptanceRadiusMetaData;
    *_holdTimeMetaData          = *other._holdTimeMetaData;
    *_loiterTurnsMetaData       = *other._loiterTurnsMetaData;
    *_loiterSecondsMetaData     = *other._loiterSecondsMetaData;
    *_delaySecondsMetaData      = *other._delaySecondsMetaData;
    *_jumpSequenceMetaData      = *other._jumpSequenceMetaData;
    *_jumpRepeatMetaData        = *other._jumpRepeatMetaData;
    
    return *this;
}

bool MissionItem::isNavigationType()
{
    return (_command < MAV_CMD_NAV_LAST);
}

void MissionItem::save(QTextStream &saveStream)
{
    QString position("%1\t%2\t%3");
    position = position.arg(x(), 0, 'g', 18);
    position = position.arg(y(), 0, 'g', 18);
    position = position.arg(z(), 0, 'g', 18);
    QString parameters("%1\t%2\t%3\t%4");
    parameters = parameters.arg(param2(), 0, 'g', 18).arg(param2(), 0, 'g', 18).arg(loiterOrbitRadius(), 0, 'g', 18).arg(yawRadians(), 0, 'g', 18);
    // FORMAT: <INDEX> <CURRENT WP> <COORD FRAME> <COMMAND> <PARAM1> <PARAM2> <PARAM3> <PARAM4> <PARAM5/X/LONGITUDE> <PARAM6/Y/LATITUDE> <PARAM7/Z/ALTITUDE> <AUTOCONTINUE> <DESCRIPTION>
    // as documented here: http://qgroundcontrol.org/waypoint_protocol
    saveStream << this->sequenceNumber() << "\t" << this->isCurrentItem() << "\t" << this->frame() << "\t" << this->command() << "\t"  << parameters << "\t" << position  << "\t" << this->autoContinue() << "\r\n"; //"\t" << this->getDescription() << "\r\n";
}

bool MissionItem::load(QTextStream &loadStream)
{
    const QStringList &wpParams = loadStream.readLine().split("\t");
    if (wpParams.size() == 12) {
        setSequenceNumber(wpParams[0].toInt());
        setIsCurrentItem(wpParams[1].toInt() == 1 ? true : false);
        _frame = (MAV_FRAME) wpParams[2].toInt();
        setAction(wpParams[3].toInt());
        setParam1(wpParams[4].toDouble());
        setParam2(wpParams[5].toDouble());
        setLoiterOrbitRadius(wpParams[6].toDouble());
        setYawRadians(wpParams[7].toDouble());
        setLatitude(wpParams[8].toDouble());
        setLongitude(wpParams[9].toDouble());
        setAltitude(wpParams[10].toDouble());
        _autocontinue = (wpParams[11].toInt() == 1 ? true : false);
        return true;
    }
    return false;
}


void MissionItem::setSequenceNumber(int sequenceNumber)
{
    _sequenceNumber = sequenceNumber;
    emit sequenceNumberChanged(_sequenceNumber);
    emit changed(this);
}

void MissionItem::setX(double x)
{
    if (!isinf(x) && !isnan(x) && ((_frame == MAV_FRAME_LOCAL_NED) || (_frame == MAV_FRAME_LOCAL_ENU)))
    {
        setLatitude(x);
    }
}

void MissionItem::setY(double y)
{
    if (!isinf(y) && !isnan(y) && ((_frame == MAV_FRAME_LOCAL_NED) || (_frame == MAV_FRAME_LOCAL_ENU)))
    {
        setLongitude(y);
    }
}

void MissionItem::setZ(double z)
{
    if (!isinf(z) && !isnan(z) && ((_frame == MAV_FRAME_LOCAL_NED) || (_frame == MAV_FRAME_LOCAL_ENU)))
    {
        setAltitude(z);
    }
}

void MissionItem::setLatitude(double lat)
{
    if (_coordinate.latitude() != lat && ((_frame == MAV_FRAME_GLOBAL) || (_frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        _coordinate.setLatitude(lat);
        emit changed(this);
        emit coordinateChanged(coordinate());
    }
}

void MissionItem::setLongitude(double lon)
{
    if (_coordinate.longitude() != lon && ((_frame == MAV_FRAME_GLOBAL) || (_frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        _coordinate.setLongitude(lon);
        emit changed(this);
        emit coordinateChanged(coordinate());
    }
}

void MissionItem::setAltitude(double altitude)
{
    if (_coordinate.altitude() != altitude && ((_frame == MAV_FRAME_GLOBAL) || (_frame == MAV_FRAME_GLOBAL_RELATIVE_ALT)))
    {
        _coordinate.setAltitude(altitude);
        emit changed(this);
        emit valueStringsChanged(valueStrings());
        emit coordinateChanged(coordinate());
    }
}

void MissionItem::setAction(int /*MAV_CMD*/ action)
{
    if (_command != action) {
        _command = (MavlinkQmlSingleton::Qml_MAV_CMD)action;

        // Flick defaults according to WP type

        if (_command == MAV_CMD_NAV_TAKEOFF) {
            // We default to 15 degrees minimum takeoff pitch
            setParam1(15.0);
        }

        emit changed(this);
        emit commandNameChanged(commandName());
        emit commandChanged((MavlinkQmlSingleton::Qml_MAV_CMD)_command);
        emit specifiesCoordinateChanged(specifiesCoordinate());
        emit valueLabelsChanged(valueLabels());
        emit valueStringsChanged(valueStrings());
    }
}

void MissionItem::setFrame(int /*MAV_FRAME*/ frame)
{
    if (_frame != frame) {
        _frame = frame;
        emit changed(this);
    }
}

void MissionItem::setAutocontinue(bool autoContinue)
{
    if (_autocontinue != autoContinue) {
        _autocontinue = autoContinue;
        emit changed(this);
    }
}

void MissionItem::setIsCurrentItem(bool isCurrentItem)
{
    if (_isCurrentItem != isCurrentItem) {
        _isCurrentItem = isCurrentItem;
        emit isCurrentItemChanged(isCurrentItem);
    }
}

void MissionItem::setAcceptanceRadius(double radius)
{
    setParam2(radius);
}

void MissionItem::setParam1(double param)
{
    if (param1() != param)
    {
        _param1Fact->setValue(param);
        emit changed(this);
        emit valueStringsChanged(valueStrings());
    }
}

void MissionItem::setParam2(double param)
{
    if (param2() != param)
    {
        _param2Fact->setValue(param);
        emit valueStringsChanged(valueStrings());
        emit changed(this);
    }
}

void MissionItem::setParam3(double param3)
{
    setLoiterOrbitRadius(param3);
}

void MissionItem::setParam4(double param4)
{
    setYawRadians(param4);
}

void MissionItem::setParam5(double param5)
{
    if (_coordinate.latitude() != param5) {
        _coordinate.setLatitude(param5);
        emit changed(this);
        emit valueStringsChanged(valueStrings());
    }
}

void MissionItem::setParam6(double param6)
{
    if (_coordinate.longitude() != param6) {
        _coordinate.setLongitude(param6);
        emit changed(this);
        emit valueStringsChanged(valueStrings());
    }
}

void MissionItem::setParam7(double param7)
{
    if (_coordinate.altitude() != param7) {
        _coordinate.setAltitude(param7);
        emit valueStringsChanged(valueStrings());
        emit changed(this);
    }
}

void MissionItem::setLoiterOrbitRadius(double radius)
{
    if (loiterOrbitRadius() != radius) {
        _loiterOrbitRadiusFact->setValue(radius);
        emit valueStringsChanged(valueStrings());
        emit changed(this);
    }
}

void MissionItem::setHoldTime(int holdTime)
{
    setParam1(holdTime);
}

void MissionItem::setHoldTime(double holdTime)
{
    setParam1(holdTime);
}

bool MissionItem::specifiesCoordinate(void) const
{
    switch (_command) {
        case MAV_CMD_NAV_WAYPOINT:
        case MAV_CMD_NAV_LOITER_UNLIM:
        case MAV_CMD_NAV_LOITER_TURNS:
        case MAV_CMD_NAV_LOITER_TIME:
        case MAV_CMD_NAV_LAND:
        case MAV_CMD_NAV_TAKEOFF:
            return true;
        default:
            return false;
    }
}

QString MissionItem::commandName(void)
{
    QString type;
    
    switch (_command) {
        case MAV_CMD_NAV_WAYPOINT:
            type = "Waypoint";
            break;
        case MAV_CMD_NAV_LOITER_UNLIM:
        case MAV_CMD_NAV_LOITER_TURNS:
        case MAV_CMD_NAV_LOITER_TIME:
            type = "Loiter";
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            type = "Return Home";
            break;
        case MAV_CMD_NAV_LAND:
            type = "Land";
            break;
        case MAV_CMD_NAV_TAKEOFF:
            type = "Takeoff";
            break;
        case MAV_CMD_CONDITION_DELAY:
            type = "Delay";
            break;
        case MAV_CMD_DO_JUMP:
            type = "Jump To Command";
            break;
        default:
            type = QString("Unknown (%1)").arg(_command);
            break;
    }
    
    return type;
}

QStringList MissionItem::valueLabels(void)
{
    QStringList labels;
    
    switch (_command) {
        case MAV_CMD_NAV_WAYPOINT:
            if (frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT) {
                labels << "Alt (rel):";
            } else {
                labels << "Alt:";
            }
            labels << "Heading:" << "Radius:" << "Hold:";
            break;
        case MAV_CMD_NAV_LOITER_UNLIM:
            labels << "Heading:" << "Radius:";
            break;
        case MAV_CMD_NAV_LOITER_TURNS:
            labels << "Heading:"  << "Radius:"<< "Turns:";
            break;
        case MAV_CMD_NAV_LOITER_TIME:
            labels << "Heading:" << "Radius:" << "Seconds:";
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            break;
        case MAV_CMD_NAV_LAND:
            if (frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT) {
                labels << "Alt (rel):";
            } else {
                labels << "Alt:";
            }
            labels << "Heading:";
            break;
        case MAV_CMD_NAV_TAKEOFF:
            if (frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT) {
                labels << "Alt (rel):";
            } else {
                labels << "Alt:";
            }
            labels << "Heading:" << "Pitch:";
            break;
        case MAV_CMD_CONDITION_DELAY:
            labels << "Seconds:";
            break;
        case MAV_CMD_DO_JUMP:
            labels << "Jump to:" << "Repeat:";
            break;
        default:
            break;
    }
    
    return labels;
}

QString MissionItem::_oneDecimalString(double value)
{
    return QString("%1").arg(value, 0 /* min field width */, 'f' /* format */, 1 /* precision */);
}

QStringList MissionItem::valueStrings(void)
{
    QStringList list;
    
    switch (_command) {
        case MAV_CMD_NAV_WAYPOINT:
            list << _oneDecimalString(_coordinate.altitude()) << _oneDecimalString(yawDegrees()) << _oneDecimalString(param2()) << _oneDecimalString(param1());
            break;
        case MAV_CMD_NAV_LOITER_UNLIM:
            list << _oneDecimalString(yawRadians() * (180.0 / M_PI)) << _oneDecimalString(loiterOrbitRadius());
            break;
        case MAV_CMD_NAV_LOITER_TURNS:
            list << _oneDecimalString(yawRadians() * (180.0 / M_PI)) << _oneDecimalString(loiterOrbitRadius()) << _oneDecimalString(param1());
            break;
        case MAV_CMD_NAV_LOITER_TIME:
            list << _oneDecimalString(yawRadians() * (180.0 / M_PI)) << _oneDecimalString(loiterOrbitRadius()) << _oneDecimalString(param1());
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            break;
        case MAV_CMD_NAV_LAND:
            list << _oneDecimalString(_coordinate.altitude()) << _oneDecimalString(yawRadians() * (180.0 / M_PI));
            break;
        case MAV_CMD_NAV_TAKEOFF:
            list << _oneDecimalString(_coordinate.altitude()) << _oneDecimalString(yawRadians() * (180.0 / M_PI)) << _oneDecimalString(param1());
            break;
        case MAV_CMD_CONDITION_DELAY:
            list << _oneDecimalString(param1());
            break;
        case MAV_CMD_DO_JUMP:
            list << _oneDecimalString(param1()) << _oneDecimalString(param2());
            break;
        default:
            break;
    }
    
    return list;
}

void MissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    _coordinate = coordinate;
    emit coordinateChanged(coordinate);
    emit changed(this);
}

QStringList MissionItem::commandNames(void) {
    QStringList list;
    
    for (int i=0; i<_cMavCmd2Name; i++) {
        list += _rgMavCmd2Name[i].name;
    }
    
    return list;
}

int MissionItem::commandByIndex(void)
{
    for (int i=0; i<_cMavCmd2Name; i++) {
        if (_rgMavCmd2Name[i].command == _command) {
            return i;
        }
    }
    
    return -1;
}

void MissionItem::setCommandByIndex(int index)
{
    if (index < 0 || index >= _cMavCmd2Name) {
        qWarning() << "Invalid index" << index;
        return;
    }
    
    setCommand((MavlinkQmlSingleton::Qml_MAV_CMD)_rgMavCmd2Name[index].command);
}

QmlObjectListModel* MissionItem::facts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);
    
    switch ((MAV_CMD)_command) {
        case MAV_CMD_NAV_WAYPOINT:
            _param2Fact->_setName("Radius:");
            _param2Fact->setMetaData(_acceptanceRadiusMetaData);
            _param1Fact->_setName("Hold:");
            _param1Fact->setMetaData(_holdTimeMetaData);
            model->append(_yawRadiansFact);
            model->append(_param2Fact);
            model->append(_param1Fact);
            break;
        case MAV_CMD_NAV_LOITER_UNLIM:
            model->append(_yawRadiansFact);
            model->append(_loiterOrbitRadiusFact);
            break;
        case MAV_CMD_NAV_LOITER_TURNS:
            _param1Fact->_setName("Turns:");
            _param1Fact->setMetaData(_loiterTurnsMetaData);
            model->append(_yawRadiansFact);
            model->append(_loiterOrbitRadiusFact);
            model->append(_param1Fact);
            break;
        case MAV_CMD_NAV_LOITER_TIME:
            _param1Fact->_setName("Seconds:");
            _param1Fact->setMetaData(_loiterSecondsMetaData);
            model->append(_yawRadiansFact);
            model->append(_loiterOrbitRadiusFact);
            model->append(_param1Fact);
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            break;
        case MAV_CMD_NAV_LAND:
            model->append(_yawRadiansFact);
            break;
        case MAV_CMD_NAV_TAKEOFF:
            _param1Fact->_setName("Pitch:");
            _param1Fact->setMetaData(_pitchMetaData);
            model->append(_yawRadiansFact);
            model->append(_param1Fact);
            break;
        case MAV_CMD_CONDITION_DELAY:
            _param1Fact->_setName("Seconds:");
            _param1Fact->setMetaData(_delaySecondsMetaData);
            model->append(_param1Fact);
            break;
        case MAV_CMD_DO_JUMP:
            _param1Fact->_setName("Seq #:");
            _param1Fact->setMetaData(_jumpSequenceMetaData);
            _param2Fact->_setName("Repeat:");
            _param2Fact->setMetaData(_jumpRepeatMetaData);
            model->append(_param1Fact);
            model->append(_param2Fact);
            break;
        default:
            break;
    }
    
    return model;
}

int MissionItem::factCount(void)
{
    switch ((MAV_CMD)_command) {
        case MAV_CMD_NAV_WAYPOINT:
            return 3;
        case MAV_CMD_NAV_LOITER_UNLIM:
            return 2;
        case MAV_CMD_NAV_LOITER_TURNS:
            return 3;
        case MAV_CMD_NAV_LOITER_TIME:
            return 3;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            return 0;
        case MAV_CMD_NAV_LAND:
            return 1;
        case MAV_CMD_NAV_TAKEOFF:
            return 2;
        case MAV_CMD_CONDITION_DELAY:
            return 1;
        case MAV_CMD_DO_JUMP:
            return 2;
        default:
            return 0;
    }
}

double MissionItem::yawRadians(void) const
{
    return _yawRadiansFact->value().toDouble();
}

void MissionItem::setYawRadians(double yaw)
{
    if (yawRadians() != yaw)
    {
        _yawRadiansFact->setValue(yaw);
        emit yawChanged(yaw);
        emit changed(this);
        emit valueStringsChanged(valueStrings());
    }
}


double MissionItem::yawDegrees(void) const
{
    return yawRadians() * (180.0 / M_PI);
}

void MissionItem::setYawDegrees(double yaw)
{
    setYawRadians(yaw * (M_PI / 180.0));
}

