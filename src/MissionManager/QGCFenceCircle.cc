/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCFenceCircle.h"
#include "JsonHelper.h"

const char* QGCFenceCircle::_jsonInclusionKey = "inclusion";

QGCFenceCircle::QGCFenceCircle(QObject* parent)
    : QGCMapCircle  (parent)
    , _inclusion    (true)
{
    _init();
}

QGCFenceCircle::QGCFenceCircle(const QGeoCoordinate& center, double radius, bool inclusion, QObject* parent)
    : QGCMapCircle  (center, radius, false /* showRotation */, true /* clockwiseRotation */, parent)
    , _inclusion    (inclusion)
{
    _init();
}

QGCFenceCircle::QGCFenceCircle(const QGCFenceCircle& other, QObject* parent)
    : QGCMapCircle  (other, parent)
    , _inclusion    (other._inclusion)
{
    _init();
}

void QGCFenceCircle::_init(void)
{
    connect(this, &QGCFenceCircle::inclusionChanged, this, &QGCFenceCircle::_setDirty);
}

const QGCFenceCircle& QGCFenceCircle::operator=(const QGCFenceCircle& other)
{
    QGCMapCircle::operator=(other);

    setInclusion(other._inclusion);

    return *this;
}

void QGCFenceCircle::_setDirty(void)
{
    setDirty(true);
}

void QGCFenceCircle::saveToJson(QJsonObject& json)
{
    json[JsonHelper::jsonVersionKey] = _jsonCurrentVersion;
    json[_jsonInclusionKey] = _inclusion;
    QGCMapCircle::saveToJson(json);
}

bool QGCFenceCircle::loadFromJson(const QJsonObject& json, QString& errorString)
{
    errorString.clear();

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,   QJsonValue::Double, true },
        { _jsonInclusionKey,            QJsonValue::Bool,   true },
    };
    if (!JsonHelper::validateKeys(json, keyInfoList, errorString)) {
        return false;
    }

    if (json[JsonHelper::jsonVersionKey].toInt() != _jsonCurrentVersion) {
        errorString = tr("GeoFence Circle only supports version %1").arg(_jsonCurrentVersion);
        return false;
    }

    if (!QGCMapCircle::loadFromJson(json, errorString)) {
        return false;
    }

    setInclusion(json[_jsonInclusionKey].toBool());

    return true;
}

void QGCFenceCircle::setInclusion(bool inclusion)
{
    if (inclusion != _inclusion) {
        _inclusion = inclusion;
        emit inclusionChanged(inclusion);
    }
}
