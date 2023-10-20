/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCFencePolygon.h"
#include "JsonHelper.h"

const char* QGCFencePolygon::_jsonInclusionKey = "inclusion";
const char* QGCFencePolygon::_jsonFenceActionKey = "fenceAction";
const char* QGCFencePolygon::_jsonMaxAltitudeKey = "maxAltitude";

QGCFencePolygon::QGCFencePolygon(bool inclusion, QObject* parent)
    : QGCMapPolygon (parent)
    , _inclusion    (inclusion)
{
    _init();
}

QGCFencePolygon::QGCFencePolygon(const QGCFencePolygon& other, QObject* parent)
    : QGCMapPolygon (other, parent)
    , _inclusion    (other._inclusion)
    , _fenceAction  (other._fenceAction)
    , _maxAltitude  (other._maxAltitude)
{
    _init();
}

void QGCFencePolygon::_init(void)
{
    connect(this, &QGCFencePolygon::inclusionChanged, this, &QGCFencePolygon::_setDirty);
}

const QGCFencePolygon& QGCFencePolygon::operator=(const QGCFencePolygon& other)
{
    QGCMapPolygon::operator=(other);

    setInclusion(other._inclusion);
    setFenceAction(other._fenceAction);
    setMaxAltitude(other._maxAltitude);

    return *this;
}

void QGCFencePolygon::_setDirty(void)
{
    setDirty(true);
}

void QGCFencePolygon::saveToJson(QJsonObject& json)
{
    json[JsonHelper::jsonVersionKey] = _jsonCurrentVersion;
    json[_jsonInclusionKey] = _inclusion;
    json[_jsonFenceActionKey] = _fenceAction;
    json[_jsonMaxAltitudeKey] = _maxAltitude;
    QGCMapPolygon::saveToJson(json);
}

bool QGCFencePolygon::loadFromJson(const QJsonObject& json, bool required, QString& errorString)
{
    errorString.clear();

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,   QJsonValue::Double, true },
        { _jsonInclusionKey,            QJsonValue::Bool,   true },
        { _jsonFenceActionKey,          QJsonValue::Double, true },
        { _jsonMaxAltitudeKey,          QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(json, keyInfoList, errorString)) {
        return false;
    }

    if (json[JsonHelper::jsonVersionKey].toInt() != _jsonCurrentVersion) {
        errorString = tr("GeoFence Polygon only supports version %1").arg(_jsonCurrentVersion);
        return false;
    }

    if (!QGCMapPolygon::loadFromJson(json, required, errorString)) {
        return false;
    }

    setMaxAltitude(json[_jsonMaxAltitudeKey].toInt());
    setFenceAction(json[_jsonFenceActionKey].toInt());
    setInclusion(json[_jsonInclusionKey].toBool());

    return true;
}

void QGCFencePolygon::setInclusion(bool inclusion)
{
    _inclusion = inclusion;
    emit inclusionChanged(inclusion);
}

void QGCFencePolygon::setFenceAction (int fenceAction)
{
    _fenceAction = fenceAction;
}

void QGCFencePolygon::setMaxAltitude (int maxAltitude)
{
    _maxAltitude = maxAltitude;
}
