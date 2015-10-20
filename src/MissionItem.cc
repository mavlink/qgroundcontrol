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

QGC_LOGGING_CATEGORY(MissionItemLog, "MissionItemLog")

const double MissionItem::defaultPitch =                15.0;
const double MissionItem::defaultHeading =              0.0;
const double MissionItem::defaultAltitude =             25.0;
const double MissionItem::defaultAcceptanceRadius =     3.0;
const double MissionItem::defaultLoiterOrbitRadius =    10.0;
const double MissionItem::defaultLoiterTurns =          1.0;

QDebug operator<<(QDebug dbg, const MissionItem& missionItem)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "MissionItem(" << missionItem.coordinate() << ")";
    
    return dbg;
}

QDebug operator<<(QDebug dbg, const MissionItem* missionItem)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "MissionItem(" << missionItem->coordinate() << ")";
    
    return dbg;
}

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
                         int            command,
                         double         param1,
                         double         param2,
                         double         param3,
                         double         param4,
                         bool           autocontinue,
                         bool           isCurrentItem,
                         int            frame)
    : QObject(parent)
    , _sequenceNumber(sequenceNumber)
    , _frame(-1)    // Forces set of _altitudeRelativeToHomeFact
    , _command((MavlinkQmlSingleton::Qml_MAV_CMD)command)
    , _autocontinue(autocontinue)
    , _isCurrentItem(isCurrentItem)
    , _reachedTime(0)
    , _headingDegreesFact(NULL)
    , _dirty(false)
    , _homePositionSpecialCase(false)
    , _homePositionValid(false)
{
    _latitudeFact                   = new Fact(0, "Latitude:",                      FactMetaData::valueTypeDouble, this);
    _longitudeFact                  = new Fact(0, "Longitude:",                     FactMetaData::valueTypeDouble, this);
    _altitudeFact                   = new Fact(0, "Altitude:",                      FactMetaData::valueTypeDouble, this);
    _headingDegreesFact             = new Fact(0, "Heading:",                       FactMetaData::valueTypeDouble, this);
    _loiterOrbitRadiusFact          = new Fact(0, "Radius:",                        FactMetaData::valueTypeDouble, this);
    _param1Fact                     = new Fact(0, QString(),                        FactMetaData::valueTypeDouble, this);
    _param2Fact                     = new Fact(0, QString(),                        FactMetaData::valueTypeDouble, this);
    _altitudeRelativeToHomeFact     = new Fact(0, "Altitude is relative to home",   FactMetaData::valueTypeDouble, this);

    setFrame(frame);
    
    setCoordinate(coordinate);
    setParam1(param1);
    setParam2(param2);
    _setYawRadians(param4);
    setLoiterOrbitRadius(param3);
    
    // FIXME: Need to fill out more meta data
    
    FactMetaData* latitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble, _latitudeFact);
    latitudeMetaData->setUnits("deg");
    
    FactMetaData* longitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble, _longitudeFact);
    longitudeMetaData->setUnits("deg");
    
    FactMetaData* altitudeMetaData = new FactMetaData(FactMetaData::valueTypeDouble, _altitudeFact);
    altitudeMetaData->setUnits("meters");
    
    FactMetaData* headingMetaData = new FactMetaData(FactMetaData::valueTypeDouble, _headingDegreesFact);
    headingMetaData->setUnits("deg");
    
    _pitchMetaData = new FactMetaData(FactMetaData::valueTypeDouble, this);
    _pitchMetaData->setUnits("deg");
    
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
    
    _latitudeFact->setMetaData(latitudeMetaData);
    _longitudeFact->setMetaData(longitudeMetaData);
    _altitudeFact->setMetaData(altitudeMetaData);
    _headingDegreesFact->setMetaData(headingMetaData);
    _loiterOrbitRadiusFact->setMetaData(loiterOrbitRadiusMetaData);

    _connectSignals();
}

MissionItem::MissionItem(const MissionItem& other, QObject* parent)
    : QObject(parent)
{
    _latitudeFact               = new Fact(this);
    _longitudeFact              = new Fact(this);
    _altitudeFact               = new Fact(this);
    _headingDegreesFact         = new Fact(this);
    _loiterOrbitRadiusFact      = new Fact(this);
    _param1Fact                 = new Fact(this);
    _param2Fact                 = new Fact(this);
    _altitudeRelativeToHomeFact = new Fact(this);
    
    _pitchMetaData = new FactMetaData(this);
    
    _acceptanceRadiusMetaData = new FactMetaData(this);
    _holdTimeMetaData = new FactMetaData(this);
    _loiterTurnsMetaData = new FactMetaData(this);
    _loiterSecondsMetaData = new FactMetaData(this);
    _delaySecondsMetaData = new FactMetaData(this);
    _jumpSequenceMetaData = new FactMetaData(this);
    _jumpRepeatMetaData = new FactMetaData(this);

    _connectSignals();

    *this = other;
}

const MissionItem& MissionItem::operator=(const MissionItem& other)
{
    _sequenceNumber             = other._sequenceNumber;
    _isCurrentItem              = other._isCurrentItem;
    _frame                      = other._frame;
    _command                    = other._command;
    _autocontinue               = other._autocontinue;
    _reachedTime                = other._reachedTime;
    _altitudeRelativeToHomeFact = other._altitudeRelativeToHomeFact;
    _dirty                      = other._dirty;
    _homePositionSpecialCase    = other._homePositionSpecialCase;
    _homePositionValid          = other._homePositionValid;

    *_latitudeFact              = *other._latitudeFact;
    *_longitudeFact             = *other._longitudeFact;
    *_altitudeFact              = *other._altitudeFact;
    *_headingDegreesFact        = *other._headingDegreesFact;
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

void MissionItem::_connectSignals(void)
{
    // Connect to valueChanged to track dirty state
    connect(_latitudeFact,                  &Fact::valueChanged, this, &MissionItem::_factValueChanged);
    connect(_longitudeFact,                 &Fact::valueChanged, this, &MissionItem::_factValueChanged);
    connect(_altitudeFact,                  &Fact::valueChanged, this, &MissionItem::_factValueChanged);
    connect(_headingDegreesFact,            &Fact::valueChanged, this, &MissionItem::_factValueChanged);
    connect(_loiterOrbitRadiusFact,         &Fact::valueChanged, this, &MissionItem::_factValueChanged);
    connect(_param1Fact,                    &Fact::valueChanged, this, &MissionItem::_factValueChanged);
    connect(_param2Fact,                    &Fact::valueChanged, this, &MissionItem::_factValueChanged);
    connect(_altitudeRelativeToHomeFact,    &Fact::valueChanged, this, &MissionItem::_factValueChanged);

    // Connect valueChanged signals so we can output coordinateChanged signal
    connect(_latitudeFact,  &Fact::valueChanged, this, &MissionItem::_coordinateFactChanged);
    connect(_longitudeFact, &Fact::valueChanged, this, &MissionItem::_coordinateFactChanged);
    connect(_altitudeFact,  &Fact::valueChanged, this, &MissionItem::_coordinateFactChanged);

    connect(_headingDegreesFact, &Fact::valueChanged, this, &MissionItem::_headingDegreesFactChanged);
}

MissionItem::~MissionItem()
{    
}

bool MissionItem::isNavigationType()
{
    return (_command < MavlinkQmlSingleton::MAV_CMD_NAV_LAST);
}

void MissionItem::save(QTextStream &saveStream)
{
    QString position("%1\t%2\t%3");
    position = position.arg(x(), 0, 'g', 18);
    position = position.arg(y(), 0, 'g', 18);
    position = position.arg(z(), 0, 'g', 18);
    QString parameters("%1\t%2\t%3\t%4");
    parameters = parameters.arg(param1(), 0, 'g', 18).arg(param2(), 0, 'g', 18).arg(loiterOrbitRadius(), 0, 'g', 18).arg(_yawRadians(), 0, 'g', 18);
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
        setFrame(wpParams[2].toInt());
        setAction(wpParams[3].toInt());
        setParam1(wpParams[4].toDouble());
        setParam2(wpParams[5].toDouble());
        setLoiterOrbitRadius(wpParams[6].toDouble());
        _setYawRadians(wpParams[7].toDouble());
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
    if (_latitudeFact->value().toDouble() != lat)
    {
        _latitudeFact->setValue(lat);
        emit changed(this);
        emit coordinateChanged(coordinate());
    }
}

void MissionItem::setLongitude(double lon)
{
    if (_longitudeFact->value().toDouble() != lon)
    {
        _longitudeFact->setValue(lon);
        emit changed(this);
        emit coordinateChanged(coordinate());
    }
}

void MissionItem::setAltitude(double altitude)
{
    if (_altitudeFact->value().toDouble() != altitude)
    {
        _altitudeFact->setValue(altitude);
        emit changed(this);
        emit valueStringsChanged(valueStrings());
        emit coordinateChanged(coordinate());
    }
}

void MissionItem::setAction(int /*MAV_CMD*/ action)
{
    if (_command != action) {
        _command = (MavlinkQmlSingleton::Qml_MAV_CMD)action;

        // Fix defaults according to WP type

        switch (_command) {
            case MavlinkQmlSingleton::MAV_CMD_NAV_TAKEOFF:
                setParam1(defaultPitch);
                break;
            case MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT:
                setAcceptanceRadius(defaultAcceptanceRadius);
                break;
            case MavlinkQmlSingleton::MAV_CMD_NAV_LOITER_UNLIM:
            case MavlinkQmlSingleton::MAV_CMD_NAV_LOITER_TIME:
                setLoiterOrbitRadius(defaultLoiterOrbitRadius);
                break;
            case MavlinkQmlSingleton::MAV_CMD_NAV_LOITER_TURNS:
                setLoiterOrbitRadius(defaultLoiterOrbitRadius);
                setParam1(defaultLoiterTurns);
                break;
            default:
                break;
        }
        setHeadingDegrees(defaultHeading);
        setAltitude(defaultAltitude);

        if (specifiesCoordinate()) {
            if (_frame != MAV_FRAME_GLOBAL && _frame != MAV_FRAME_GLOBAL_RELATIVE_ALT) {
                setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
            }
        } else {
            setFrame(MAV_FRAME_MISSION);
        }

        emit changed(this);
        emit commandNameChanged(commandName());
        emit commandChanged((MavlinkQmlSingleton::Qml_MAV_CMD)_command);
        emit valueLabelsChanged(valueLabels());
        emit valueStringsChanged(valueStrings());
    }
}

int MissionItem::frame(void) const
{
    if (_altitudeRelativeToHomeFact->value().toBool()) {
        return MAV_FRAME_GLOBAL_RELATIVE_ALT;
    } else {
        return _frame;
    }
}

void MissionItem::setFrame(int /*MAV_FRAME*/ frame)
{
    if (_frame != frame) {
        _altitudeRelativeToHomeFact->setValue(frame == MAV_FRAME_GLOBAL_RELATIVE_ALT);
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
    _setYawRadians(param4);
}

void MissionItem::setParam5(double param5)
{
    setLatitude(param5);
}

void MissionItem::setParam6(double param6)
{
    setLongitude(param6);
}

void MissionItem::setParam7(double param7)
{
    setAltitude(param7);
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

QString MissionItem::commandDescription(void)
{
    QString description;

    switch (_command) {
        case MAV_CMD_NAV_WAYPOINT:
            description = "Travel to a position in 3D space.";
            break;
        case MAV_CMD_NAV_LOITER_UNLIM:
            description = "Travel to a position and Loiter around the specified radius indefinitely.";
            break;
        case MAV_CMD_NAV_LOITER_TURNS:
            description = "Travel to a position and Loiter around the specified radius for a number of turns.";
            break;
        case MAV_CMD_NAV_LOITER_TIME:
            description = "Travel to a position and Loiter around the specified radius for an amount of time.";
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            description = "Send the vehicle back to the home position.";
            break;
        case MAV_CMD_NAV_LAND:
            description = "Land vehicle at the specified location.";
            break;
        case MAV_CMD_NAV_TAKEOFF:
            description = "Take off from the ground and travel towards the specified position.";
            break;
        case MAV_CMD_CONDITION_DELAY:
            description = "Delay";
            break;
        case MAV_CMD_DO_JUMP:
            description = "Jump To Command";
            break;
        default:
            description = QString("Unknown (%1)").arg(_command);
            break;
    }

    return description;
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
            list << _oneDecimalString(_altitudeFact->value().toDouble()) << _oneDecimalString(headingDegrees()) << _oneDecimalString(param2()) << _oneDecimalString(param1());
            break;
        case MAV_CMD_NAV_LOITER_UNLIM:
            list << _oneDecimalString(headingDegrees()) << _oneDecimalString(loiterOrbitRadius());
            break;
        case MAV_CMD_NAV_LOITER_TURNS:
            list << _oneDecimalString(headingDegrees()) << _oneDecimalString(loiterOrbitRadius()) << _oneDecimalString(param1());
            break;
        case MAV_CMD_NAV_LOITER_TIME:
            list << _oneDecimalString(headingDegrees()) << _oneDecimalString(loiterOrbitRadius()) << _oneDecimalString(param1());
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            break;
        case MAV_CMD_NAV_LAND:
            list << _oneDecimalString(_altitudeFact->value().toDouble()) << _oneDecimalString(headingDegrees());
            break;
        case MAV_CMD_NAV_TAKEOFF:
            list << _oneDecimalString(_altitudeFact->value().toDouble()) << _oneDecimalString(headingDegrees()) << _oneDecimalString(param1());
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
        if (_rgMavCmd2Name[i].command == (MAV_CMD)_command) {
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

QmlObjectListModel* MissionItem::textFieldFacts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);
    
    switch ((MAV_CMD)_command) {
        case MAV_CMD_NAV_WAYPOINT:
            _param1Fact->_setName("Hold:");
            _param1Fact->setMetaData(_holdTimeMetaData);
            model->append(_altitudeFact);
            if (!_homePositionSpecialCase) {
                model->append(_param1Fact);
            }
            break;
        case MAV_CMD_NAV_LOITER_UNLIM:
            model->append(_altitudeFact);
            model->append(_loiterOrbitRadiusFact);
            break;
        case MAV_CMD_NAV_LOITER_TURNS:
            _param1Fact->_setName("Turns:");
            _param1Fact->setMetaData(_loiterTurnsMetaData);
            model->append(_altitudeFact);
            model->append(_loiterOrbitRadiusFact);
            model->append(_param1Fact);
            break;
        case MAV_CMD_NAV_LOITER_TIME:
            _param1Fact->_setName("Seconds:");
            _param1Fact->setMetaData(_loiterSecondsMetaData);
            model->append(_altitudeFact);
            model->append(_loiterOrbitRadiusFact);
            model->append(_param1Fact);
            break;
        case MAV_CMD_NAV_LAND:
            model->append(_altitudeFact);
            break;
        case MAV_CMD_NAV_TAKEOFF:
            _param1Fact->_setName("Pitch:");
            _param1Fact->setMetaData(_pitchMetaData);
            model->append(_altitudeFact);
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

    if (specifiesHeading()) {
        model->append(_headingDegreesFact);
    }

    
    return model;
}

QmlObjectListModel* MissionItem::checkboxFacts(void)
{
    QmlObjectListModel* model = new QmlObjectListModel(this);
    
    switch ((MAV_CMD)_command) {
        case MAV_CMD_NAV_WAYPOINT:
            if (!_homePositionSpecialCase) {
                model->append(_altitudeRelativeToHomeFact);
            }
            break;
        case MAV_CMD_NAV_LOITER_UNLIM:
            model->append(_altitudeRelativeToHomeFact);
            break;
        case MAV_CMD_NAV_LOITER_TURNS:
            model->append(_altitudeRelativeToHomeFact);
            break;
        case MAV_CMD_NAV_LOITER_TIME:
            model->append(_altitudeRelativeToHomeFact);
            break;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            break;
        case MAV_CMD_NAV_LAND:
            model->append(_altitudeRelativeToHomeFact);
            break;
        case MAV_CMD_NAV_TAKEOFF:
            model->append(_altitudeRelativeToHomeFact);
            break;
        default:
            break;
    }
    
    return model;
}

double MissionItem::headingDegrees(void) const
{
    return _headingDegreesFact->value().toDouble();
}

void MissionItem::setHeadingDegrees(double headingDegrees)
{
    if (_headingDegreesFact->value().toDouble() != headingDegrees) {
        _headingDegreesFact->setValue(headingDegrees);
        emit changed(this);
        emit valueStringsChanged(valueStrings());
        emit headingDegreesChanged(headingDegrees);
    }
}


double MissionItem::_yawRadians(void) const
{
    return _headingDegreesFact->value().toDouble() * (M_PI / 180.0);
}

void MissionItem::_setYawRadians(double yawRadians)
{
    setHeadingDegrees(yawRadians * (180 / M_PI));
}

QGeoCoordinate MissionItem::coordinate(void) const
{
    return QGeoCoordinate(latitude(), longitude(), altitude());
}

void MissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    setLatitude(coordinate.latitude());
    setLongitude(coordinate.longitude());
    setAltitude(coordinate.altitude());
}

bool MissionItem::canEdit(void)
{
    bool found = false;
    
    for (int i=0; i<_cMavCmd2Name; i++) {
        if (_rgMavCmd2Name[i].command == (MAV_CMD)_command) {
            found = true;
            break;
        }
    }
    
    if (found) {
        if (!_autocontinue) {
            qCDebug(MissionItemLog) << "canEdit false due to _autocontinue != true";
            return false;
        }
        
        if (_frame != MAV_FRAME_GLOBAL && _frame != MAV_FRAME_GLOBAL_RELATIVE_ALT && _frame != MAV_FRAME_MISSION) {
            qCDebug(MissionItemLog) << "canEdit false due unsupported frame type:" << _frame;
            return false;
        }
        
        return true;
    } else {
        qCDebug(MissionItemLog) << "canEdit false due unsupported command:" << _command;
        return false;
    }
}

void MissionItem::setDirty(bool dirty)
{
    if (!_homePositionSpecialCase || !dirty) {
        // Home position never affects dirty bit

        _dirty = dirty;
        // We want to emit dirtyChanged even if _dirty didn't change. This can be handy signal for
        // any value within the item changing.
        emit dirtyChanged(_dirty);
    }
}

void MissionItem::_factValueChanged(QVariant value)
{
    Q_UNUSED(value);
    setDirty(true);
}

void MissionItem::_coordinateFactChanged(QVariant value)
{
    Q_UNUSED(value);
    emit coordinateChanged(coordinate());
}

bool MissionItem::specifiesHeading(void) const
{
    switch ((MAV_CMD)_command) {
        case MAV_CMD_NAV_LAND:
        case MAV_CMD_NAV_TAKEOFF:
            return true;
        default:
            return false;
    }
}

void MissionItem::_headingDegreesFactChanged(QVariant value)
{
    emit headingDegreesChanged(value.toDouble());
}

void MissionItem::setHomePositionValid(bool homePositionValid)
{
    _homePositionValid = homePositionValid;
    emit homePositionValidChanged(_homePositionValid);
}
