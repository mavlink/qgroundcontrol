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
#include "VisualMissionItem.h"

const char*  MissionItem::_jsonFrameKey =           "frame";
const char*  MissionItem::_jsonCommandKey =         "command";
const char*  MissionItem::_jsonAutoContinueKey =    "autoContinue";
const char*  MissionItem::_jsonCoordinateKey =      "coordinate";
const char*  MissionItem::_jsonParamsKey =          "params";
const char*  MissionItem::_jsonDoJumpIdKey =        "doJumpId";

// Deprecated V1 format keys
const char*  MissionItem::_jsonParam1Key =          "param1";
const char*  MissionItem::_jsonParam2Key =          "param2";
const char*  MissionItem::_jsonParam3Key =          "param3";
const char*  MissionItem::_jsonParam4Key =          "param4";

MissionItem::MissionItem(QObject* parent)
    : QObject(parent)
    , _sequenceNumber(0)
    , _doJumpId(-1)
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

    connect(&_param2Fact, &Fact::rawValueChanged, this, &MissionItem::_param2Changed);
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
    , _doJumpId(-1)
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

    connect(&_param2Fact, &Fact::rawValueChanged, this, &MissionItem::_param2Changed);
}

MissionItem::MissionItem(const MissionItem& other, QObject* parent)
    : QObject(parent)
    , _sequenceNumber(0)
    , _doJumpId(-1)
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

    connect(&_param2Fact, &Fact::rawValueChanged, this, &MissionItem::_param2Changed);
}

const MissionItem& MissionItem::operator=(const MissionItem& other)
{
    _doJumpId = other._doJumpId;

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
    json[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeSimpleItemValue;
    json[_jsonFrameKey] = frame();
    json[_jsonCommandKey] = command();
    json[_jsonAutoContinueKey] = autoContinue();
    json[_jsonDoJumpIdKey] = _sequenceNumber;

    QJsonArray rgParams =  { param1(), param2(), param3(), param4() };
    json[_jsonParamsKey] = rgParams;

    QJsonValue coordinateValue;
    JsonHelper::saveGeoCoordinate(QGeoCoordinate(param5(), param6(), param7()), true /* writeAltitude */, coordinateValue);
    json[_jsonCoordinateKey] = coordinateValue;
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

bool MissionItem::_convertJsonV1ToV2(const QJsonObject& json, QJsonObject& v2Json, QString& errorString)
{
    // V1 format type = "missionItem", V2 format type = "MissionItem"
    // V1 format has params in separate param[1-n] keys
    // V2 format has params in params array
    v2Json = json;

    if (json.contains(_jsonParamsKey)) {
        // Already V2 format
        return true;
    }        

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { VisualMissionItem::jsonTypeKey,   QJsonValue::String, true },
        { _jsonParam1Key,                   QJsonValue::Double, true },
        { _jsonParam2Key,                   QJsonValue::Double, true },
        { _jsonParam3Key,                   QJsonValue::Double, true },
        { _jsonParam4Key,                   QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(json, keyInfoList, errorString)) {
        return false;
    }

    if (v2Json[VisualMissionItem::jsonTypeKey].toString() == QStringLiteral("missionItem")) {
        v2Json[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeSimpleItemValue;
    }

    QJsonArray rgParams =  { json[_jsonParam1Key].toDouble(),  json[_jsonParam2Key].toDouble(), json[_jsonParam3Key].toDouble(), json[_jsonParam4Key].toDouble() };
    v2Json[_jsonParamsKey] = rgParams;
    v2Json.remove(_jsonParam1Key);
    v2Json.remove(_jsonParam2Key);
    v2Json.remove(_jsonParam3Key);
    v2Json.remove(_jsonParam4Key);

    return true;
}

bool MissionItem::load(const QJsonObject& json, int sequenceNumber, QString& errorString)
{
    QJsonObject v2Json;
    if (!_convertJsonV1ToV2(json, v2Json, errorString)) {
        return false;
    }

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { VisualMissionItem::jsonTypeKey,   QJsonValue::String, true },
        { _jsonFrameKey,                    QJsonValue::Double, true },
        { _jsonCommandKey,                  QJsonValue::Double, true },
        { _jsonParamsKey,                   QJsonValue::Array,  true },
        { _jsonAutoContinueKey,             QJsonValue::Bool,   true },
        { _jsonCoordinateKey,               QJsonValue::Array,  true },
        { _jsonDoJumpIdKey,                 QJsonValue::Double, false },
    };
    if (!JsonHelper::validateKeys(v2Json, keyInfoList, errorString)) {
        return false;
    }

    if (v2Json[VisualMissionItem::jsonTypeKey] != VisualMissionItem::jsonTypeSimpleItemValue) {
        errorString = tr("Type found: %1 must be: %2").arg(v2Json[VisualMissionItem::jsonTypeKey].toString()).arg(VisualMissionItem::jsonTypeSimpleItemValue);
        return false;
    }

    QJsonArray rgParams = v2Json[_jsonParamsKey].toArray();
    if (rgParams.count() != 4) {
        errorString = tr("%1 key must contains 4 values").arg(_jsonParamsKey);
        return false;
    }

    // Make sure to set these first since they can signal other changes
    setFrame((MAV_FRAME)v2Json[_jsonFrameKey].toInt());
    setCommand((MAV_CMD)v2Json[_jsonCommandKey].toInt());

    QGeoCoordinate coordinate;
    if (!JsonHelper::loadGeoCoordinate(v2Json[_jsonCoordinateKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    setParam5(coordinate.latitude());
    setParam6(coordinate.longitude());
    setParam7(coordinate.altitude());

    _doJumpId = -1;
    if (v2Json.contains(_jsonDoJumpIdKey)) {
        _doJumpId = v2Json[_jsonDoJumpIdKey].toInt();
    }
    setIsCurrentItem(false);
    setSequenceNumber(sequenceNumber);
    setAutoContinue(v2Json[_jsonAutoContinueKey].toBool());

    setParam1(rgParams[0].toDouble());
    setParam2(rgParams[1].toDouble());
    setParam3(rgParams[2].toDouble());
    setParam4(rgParams[3].toDouble());

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

double MissionItem::flightSpeed(void) const
{
    double flightSpeed = std::numeric_limits<double>::quiet_NaN();

    if (_commandFact.rawValue().toInt() == MAV_CMD_DO_CHANGE_SPEED && _param2Fact.rawValue().toDouble() > 0) {
        flightSpeed = _param2Fact.rawValue().toDouble();
    }

    return flightSpeed;
}

void MissionItem::_param2Changed(QVariant value)
{
    if (_commandFact.rawValue().toInt() == MAV_CMD_DO_CHANGE_SPEED && _param2Fact.rawValue().toDouble() > 0) {
        emit flightSpeedChanged(value.toDouble());
    }
}
