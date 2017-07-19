/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCFencePolygon.h"
#include "JsonHelper.h"

const char* QGCFencePolygon::_jsonInclusionKey = "inclusion";

QGCFencePolygon::QGCFencePolygon(bool inclusion, QObject* parent)
    : QGCMapPolygon (parent)
    , _inclusion    (inclusion)
{
    _init();
}

QGCFencePolygon::QGCFencePolygon(const QGCFencePolygon& other, QObject* parent)
    : QGCMapPolygon (other, parent)
    , _inclusion    (other._inclusion)
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

    return *this;
}

void QGCFencePolygon::_setDirty(void)
{
    setDirty(true);
}

void QGCFencePolygon::saveToJson(QJsonObject& json)
{
    QGCMapPolygon::saveToJson(json);

    json[_jsonInclusionKey] = _inclusion;
}

bool QGCFencePolygon::loadFromJson(const QJsonObject& json, bool required, QString& errorString)
{
    if (!QGCMapPolygon::loadFromJson(json, required, errorString)) {
        return false;
    }

    errorString.clear();

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { _jsonInclusionKey, QJsonValue::Bool, true },
    };
    if (!JsonHelper::validateKeys(json, keyInfoList, errorString)) {
        return false;
    }

    setInclusion(json[_jsonInclusionKey].toBool());

    return true;
}

void QGCFencePolygon::setInclusion(bool inclusion)
{
    if (inclusion != _inclusion) {
        _inclusion = inclusion;
        emit inclusionChanged(inclusion);
    }
}
