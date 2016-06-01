/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QStringList>
#include <QDebug>

#include "MissionItem.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "JsonHelper.h"

const char*  MissionItem::_itemType =               "missionItem";
const char*  MissionItem::_jsonTypeKey =            "type";
const char*  MissionItem::_jsonIdKey =              "id";
const char*  MissionItem::_jsonFrameKey =           "frame";
const char*  MissionItem::_jsonCommandKey =         "command";
const char*  MissionItem::_jsonParam1Key =          "param1";
const char*  MissionItem::_jsonParam2Key =          "param2";
const char*  MissionItem::_jsonParam3Key =          "param3";
const char*  MissionItem::_jsonParam4Key =          "param4";
const char*  MissionItem::_jsonAutoContinueKey =    "autoContinue";
const char*  MissionItem::_jsonCoordinateKey =      "coordinate";

MissionItem::MissionItem(QObject* parent)
    : QObject(parent)
    , _sequenceNumber(0)
    , _isCurrentItem(false)
    , _autoContinueFact             (0, "AutoContinue",                 FactMetaData::valueTypeUint32)
    , _commandFact                  (0, "",                             FactMetaData::valueTypeUint32)
    , _frameFact                    (0, "",                             FactMetaData::valueTypeUint32)
    , _param1Fact                   (0, "Param1:",                      FactMetaData::valueTypeDouble)
    , _param2Fact                   (0, "Param2:",                      FactMetaData::valueTypeDouble)
    , _param3Fact                   (0, "Param3:",                      FactMetaData::valueTypeDouble)
    , _param4Fact                   (0, "Param4:",                      FactMetaData::valueTypeDouble)
    , _param5Fact                   (0, "Latitude:",                    FactMetaData::valueTypeDouble)
    , _param6Fact                   (0, "Longitude:",                   FactMetaData::valueTypeDouble)
    , _param7Fact                   (0, "Altitude:",                    FactMetaData::valueTypeDouble)
{
    // Need a good command and frame before we start passing signals around
    _commandFact.setRawValue(MAV_CMD_NAV_WAYPOINT);
    _frameFact.setRawValue(MAV_FRAME_GLOBAL_RELATIVE_ALT);

    setAutoContinue(true);
}

MissionItem::MissionItem(int             sequenceNumber,
                         MAV_CMD         command,
                         MAV_FRAME       frame,
                         double          param1,
                         double          param2,
                         double          param3,
                         double          param4,
                         double          param5,
                         double          param6,
                         double          param7,
                         bool            autoContinue,
                         bool            isCurrentItem,
                         QObject*        parent)
    : QObject(parent)
    , _sequenceNumber(sequenceNumber)
    , _isCurrentItem(isCurrentItem)
    , _commandFact                  (0, "",                             FactMetaData::valueTypeUint32)
    , _frameFact                    (0, "",                             FactMetaData::valueTypeUint32)
    , _param1Fact                   (0, "Param1:",                      FactMetaData::valueTypeDouble)
    , _param2Fact                   (0, "Param2:",                      FactMetaData::valueTypeDouble)
    , _param3Fact                   (0, "Param3:",                      FactMetaData::valueTypeDouble)
    , _param4Fact                   (0, "Param4:",                      FactMetaData::valueTypeDouble)
    , _param5Fact                   (0, "Lat/X:",                       FactMetaData::valueTypeDouble)
    , _param6Fact                   (0, "Lon/Y:",                       FactMetaData::valueTypeDouble)
    , _param7Fact                   (0, "Alt/Z:",                       FactMetaData::valueTypeDouble)
{
    // Need a good command and frame before we start passing signals around
    _commandFact.setRawValue(MAV_CMD_NAV_WAYPOINT);
    _frameFact.setRawValue(MAV_FRAME_GLOBAL_RELATIVE_ALT);

    setCommand(command);
    setFrame(frame);
    setAutoContinue(autoContinue);

    _param1Fact.setRawValue(param1);
    _param2Fact.setRawValue(param2);
    _param3Fact.setRawValue(param3);
    _param4Fact.setRawValue(param4);
    _param5Fact.setRawValue(param5);
    _param6Fact.setRawValue(param6);
    _param7Fact.setRawValue(param7);
}

MissionItem::MissionItem(const MissionItem& other, QObject* parent)
    : QObject(parent)
    , _sequenceNumber(0)
    , _isCurrentItem(false)
    , _commandFact                  (0, "",                             FactMetaData::valueTypeUint32)
    , _frameFact                    (0, "",                             FactMetaData::valueTypeUint32)
    , _param1Fact                   (0, "Param1:",                      FactMetaData::valueTypeDouble)
    , _param2Fact                   (0, "Param2:",                      FactMetaData::valueTypeDouble)
    , _param3Fact                   (0, "Param3:",                      FactMetaData::valueTypeDouble)
    , _param4Fact                   (0, "Param4:",                      FactMetaData::valueTypeDouble)
    , _param5Fact                   (0, "Lat/X:",                       FactMetaData::valueTypeDouble)
    , _param6Fact                   (0, "Lon/Y:",                       FactMetaData::valueTypeDouble)
    , _param7Fact                   (0, "Alt/Z:",                       FactMetaData::valueTypeDouble)
{
    // Need a good command and frame before we start passing signals around
    _commandFact.setRawValue(MAV_CMD_NAV_WAYPOINT);
    _frameFact.setRawValue(MAV_FRAME_GLOBAL_RELATIVE_ALT);

    *this = other;
}

const MissionItem& MissionItem::operator=(const MissionItem& other)
{
    setCommand(other.command());
    setFrame(other.frame());
    setSequenceNumber(other._sequenceNumber);
    setAutoContinue(other.autoContinue());
    setIsCurrentItem(other._isCurrentItem);

    _param1Fact.setRawValue(other._param1Fact.rawValue());
    _param2Fact.setRawValue(other._param2Fact.rawValue());
    _param3Fact.setRawValue(other._param3Fact.rawValue());
    _param4Fact.setRawValue(other._param4Fact.rawValue());
    _param5Fact.setRawValue(other._param5Fact.rawValue());
    _param6Fact.setRawValue(other._param6Fact.rawValue());
    _param7Fact.setRawValue(other._param7Fact.rawValue());

    return *this;
}
MissionItem::~MissionItem()
{    
}

void MissionItem::save(QJsonObject& json) const
{
    json[_jsonTypeKey] = _itemType;
    json[_jsonIdKey] = sequenceNumber();
    json[_jsonFrameKey] = frame();
    json[_jsonCommandKey] = command();
    json[_jsonParam1Key] = param1();
    json[_jsonParam2Key] = param2();
    json[_jsonParam3Key] = param3();
    json[_jsonParam4Key] = param4();
    json[_jsonAutoContinueKey] = autoContinue();

    QJsonArray coordinateArray;
    coordinateArray << param5() << param6() << param7();
    json[_jsonCoordinateKey] = coordinateArray;
}

bool MissionItem::load(QTextStream &loadStream)
{
    const QStringList &wpParams = loadStream.readLine().split("\t");
    if (wpParams.size() == 12) {
        setSequenceNumber(wpParams[0].toInt());
        setIsCurrentItem(wpParams[1].toInt() == 1 ? true : false);
        setFrame((MAV_FRAME)wpParams[2].toInt());
        setCommand((MAV_CMD)wpParams[3].toInt());
        setParam1(wpParams[4].toDouble());
        setParam2(wpParams[5].toDouble());
        setParam3(wpParams[6].toDouble());
        setParam4(wpParams[7].toDouble());
        setParam5(wpParams[8].toDouble());
        setParam6(wpParams[9].toDouble());
        setParam7(wpParams[10].toDouble());
        setAutoContinue(wpParams[11].toInt() == 1 ? true : false);
        return true;
    }

    return false;
}

bool MissionItem::load(const QJsonObject& json, QString& errorString)
{
    QStringList requiredKeys;

    requiredKeys << _jsonTypeKey << _jsonIdKey << _jsonFrameKey << _jsonCommandKey <<
                    _jsonParam1Key << _jsonParam2Key << _jsonParam3Key << _jsonParam4Key <<
                    _jsonAutoContinueKey << _jsonCoordinateKey;
    if (!JsonHelper::validateRequiredKeys(json, requiredKeys, errorString)) {
        return false;
    }

    if (json[_jsonTypeKey] != _itemType) {
        errorString = QString("type found: %1 must be: %2").arg(json[_jsonTypeKey].toString()).arg(_itemType);
        return false;
    }

    // Make sure to set these first since they can signal other changes
    setFrame((MAV_FRAME)json[_jsonFrameKey].toInt());
    setCommand((MAV_CMD)json[_jsonCommandKey].toInt());

    QGeoCoordinate coordinate;
    if (!JsonHelper::toQGeoCoordinate(json[_jsonCoordinateKey], coordinate, true /* altitudeRequired */, errorString)) {
        return false;
    }
    setParam5(coordinate.latitude());
    setParam6(coordinate.longitude());
    setParam7(coordinate.altitude());

    setIsCurrentItem(false);
    setSequenceNumber(json[_jsonIdKey].toInt());
    setParam1(json[_jsonParam1Key].toDouble());
    setParam2(json[_jsonParam2Key].toDouble());
    setParam3(json[_jsonParam3Key].toDouble());
    setParam4(json[_jsonParam4Key].toDouble());
    setAutoContinue(json[_jsonAutoContinueKey].toBool());

    return true;
}


void MissionItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(_sequenceNumber);
    }
}

void MissionItem::setCommand(MAV_CMD command)
{
    if ((MAV_CMD)this->command() != command) {
        _commandFact.setRawValue(command);
    }
}

void MissionItem::setFrame(MAV_FRAME frame)
{
    if (this->frame() != frame) {
        _frameFact.setRawValue(frame);
    }
}

void MissionItem::setAutoContinue(bool autoContinue)
{
    if (this->autoContinue() != autoContinue) {
        _autoContinueFact.setRawValue(autoContinue);
    }
}

void MissionItem::setIsCurrentItem(bool isCurrentItem)
{
    if (_isCurrentItem != isCurrentItem) {
        _isCurrentItem = isCurrentItem;
        emit isCurrentItemChanged(isCurrentItem);
    }
}

void MissionItem::setParam1(double param)
{
    if (param1() != param) {
        _param1Fact.setRawValue(param);
    }
}

void MissionItem::setParam2(double param)
{
    if (param2() != param) {
        _param2Fact.setRawValue(param);
    }
}

void MissionItem::setParam3(double param)
{
    if (param3() != param) {
        _param3Fact.setRawValue(param);
    }
}

void MissionItem::setParam4(double param)
{
    if (param4() != param) {
        _param4Fact.setRawValue(param);
    }
}

void MissionItem::setParam5(double param)
{
    if (param5() != param) {
        _param5Fact.setRawValue(param);
    }
}

void MissionItem::setParam6(double param)
{
    if (param6() != param) {
        _param6Fact.setRawValue(param);
    }
}

void MissionItem::setParam7(double param)
{
    if (param7() != param) {
        _param7Fact.setRawValue(param);
    }
}

void MissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    setParam5(coordinate.latitude());
    setParam6(coordinate.longitude());
    setParam7(coordinate.altitude());
}

QGeoCoordinate MissionItem::coordinate(void) const
{
    return QGeoCoordinate(param5(), param6(), param7());
}
