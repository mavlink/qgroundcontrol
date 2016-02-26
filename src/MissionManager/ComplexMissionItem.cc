/*===================================================================
QGroundControl Open Source Ground Control Station

(c) 2009, 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "ComplexMissionItem.h"
#include "JsonHelper.h"
#include "MissionController.h"

const char* ComplexMissionItem::_jsonVersionKey =   "version";
const char* ComplexMissionItem::_jsonTypeKey =      "type";
const char* ComplexMissionItem::_jsonPolygonKey =   "polygon";
const char* ComplexMissionItem::_jsonIdKey =        "id";

const char* ComplexMissionItem::_complexType = "survey";

ComplexMissionItem::ComplexMissionItem(Vehicle* vehicle, QObject* parent)
    : VisualMissionItem(vehicle, parent)
    , _sequenceNumber(0)
    , _dirty(false)
{
    MissionItem missionItem;

    // FIXME: Bogus entries for testing
    _missionItems += new MissionItem(this);
    _missionItems += new MissionItem(this);
}

ComplexMissionItem::ComplexMissionItem(const ComplexMissionItem& other, QObject* parent)
    : VisualMissionItem(other, parent)
    , _sequenceNumber(other.sequenceNumber())
    , _dirty(false)
{

}

void ComplexMissionItem::clearPolygon(void)
{
    // Bug workaround, see below
    while (_polygonPath.count() > 1) {
        _polygonPath.takeLast();
    }
    emit polygonPathChanged();

    // Although this code should remove the polygon from the map it doesn't. There appears
    // to be a bug in MapPolygon which causes it to not be redrawn if the list is empty. So
    // we work around it by using the code above to remove all bu the last point which in turn
    // will cause the polygon to go away.
    _polygonPath.clear();
    emit specifiesCoordinateChanged();
}

void ComplexMissionItem::addPolygonCoordinate(const QGeoCoordinate coordinate)
{
    _polygonPath << QVariant::fromValue(coordinate);
    emit polygonPathChanged();

    int pointCount = _polygonPath.count();
    if (pointCount == 1) {
        setCoordinate(coordinate);
    } else if (pointCount == 3) {
        emit specifiesCoordinateChanged();
    }
    _setExitCoordinate(coordinate);
}

int ComplexMissionItem::lastSequenceNumber(void) const
{
    return _sequenceNumber + _missionItems.count() - 1;
}

void ComplexMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_coordinate != coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(_coordinate);
        _missionItems[0]->setCoordinate(coordinate);

        // FIXME: Hack
        _setExitCoordinate(coordinate);
    }
}

void ComplexMissionItem::setDirty(bool dirty)
{
   if (_dirty != dirty) {
       _dirty = dirty;
       emit dirtyChanged(_dirty);
   }
}

void ComplexMissionItem::save(QJsonObject& saveObject) const
{
    saveObject[_jsonVersionKey] =    1;
    saveObject[_jsonTypeKey] =       _complexType;
    saveObject[_jsonIdKey] =         sequenceNumber();

    // Polygon shape

    QJsonArray polygonArray;

    for (int i=0; i<_polygonPath.count(); i++) {
        const QVariant& polyVar = _polygonPath[i];

        QJsonValue jsonValue;
        JsonHelper::writeQGeoCoordinate(jsonValue, polyVar.value<QGeoCoordinate>(), false /* writeAltitude */);
        polygonArray += jsonValue;
    }

    saveObject[_jsonPolygonKey] = polygonArray;

    // Base mission items

    QJsonArray simpleItems;
    for (int i=0; i<_missionItems.count(); i++) {
        MissionItem* missionItem = _missionItems[i];

        QJsonObject jsonObject;
        missionItem->save(jsonObject);
        simpleItems.append(jsonObject);
    }
    saveObject[MissionController::jsonSimpleItemsKey] = simpleItems;
}

void ComplexMissionItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;

        // Update internal mission items to new numbering
        for (int i=0; i<_missionItems.count(); i++) {
            _missionItems[i]->setSequenceNumber(sequenceNumber++);
        }

        emit sequenceNumberChanged(sequenceNumber);
    }
}

void ComplexMissionItem::_clear(void)
{
    // Clear old settings
    for (int i=0; i<_missionItems.count(); i++) {
        _missionItems[i]->deleteLater();
    }
    _missionItems.clear();
    _polygonPath.clear();
}


bool ComplexMissionItem::load(const QJsonObject& complexObject, QString& errorString)
{
    _clear();

    // Validate requires keys
    QStringList requiredKeys;
    requiredKeys << _jsonVersionKey << _jsonTypeKey << _jsonIdKey << _jsonPolygonKey << MissionController::jsonSimpleItemsKey;
    if (!JsonHelper::validateRequiredKeys(complexObject, requiredKeys, errorString)) {
        _clear();
        return false;
    }

    // Validate types
    QStringList keyList;
    QList<QJsonValue::Type> typeList;
    keyList << _jsonVersionKey << _jsonTypeKey << _jsonIdKey << _jsonPolygonKey << MissionController::jsonSimpleItemsKey;
    typeList << QJsonValue::Double << QJsonValue::String << QJsonValue::Double << QJsonValue::Array << QJsonValue::Array;
    if (!JsonHelper::validateKeyTypes(complexObject, keyList, typeList, errorString)) {
        _clear();
        return false;
    }

    // Version check
    if (complexObject[_jsonVersionKey].toInt() != 1) {
        errorString = tr("QGroundControl does not support this version of survey items");
        _clear();
        return false;
    }
    QString complexType = complexObject[_jsonTypeKey].toString();
    if (complexType != _complexType) {
        errorString = tr("QGroundControl does not support loading this complex mission item type: %1").arg(complexType);
        _clear();
        return false;
    }

    setSequenceNumber(complexObject[_jsonIdKey].toInt());

    // Polygon shape
    QJsonArray polygonArray(complexObject[_jsonPolygonKey].toArray());
    for (int i=0; i<polygonArray.count(); i++) {
        const QJsonValue& pointValue = polygonArray[i];

        QGeoCoordinate pointCoord;
        if (!JsonHelper::toQGeoCoordinate(pointValue, pointCoord, false /* altitudeRequired */, errorString)) {
            _clear();
            return false;
        }
        _polygonPath << QVariant::fromValue(pointCoord);
    }

    // Internal mission items
    QJsonArray itemArray(complexObject[MissionController::jsonSimpleItemsKey].toArray());
    for (int i=0; i<itemArray.count(); i++) {
        const QJsonValue& itemValue = itemArray[i];

        if (!itemValue.isObject()) {
            errorString = QStringLiteral("Mission item is not an object");
            _clear();
            return false;
        }

        MissionItem* item = new MissionItem(_vehicle, this);
        if (item->load(itemValue.toObject(), errorString)) {
            _missionItems.append(item);
        } else {
            _clear();
            return false;
        }
    }

    int itemCount = _missionItems.count();
    if (itemCount > 0) {
        _coordinate = _missionItems[0]->coordinate();
        _exitCoordinate = _missionItems[itemCount - 1]->coordinate();
    }

    qDebug() << coordinate() << exitCoordinate() << _missionItems[0]->coordinate() << _missionItems[1]->coordinate();

    return true;
}

void ComplexMissionItem::_setExitCoordinate(const QGeoCoordinate& coordinate)
{
    if (_exitCoordinate != coordinate) {
        _exitCoordinate = coordinate;
        emit exitCoordinateChanged(coordinate);

        int itemCount = _missionItems.count();
        if (itemCount > 0) {
            _missionItems[itemCount - 1]->setCoordinate(coordinate);
        }
    }
}

bool ComplexMissionItem::specifiesCoordinate(void) const
{
    return _polygonPath.count() > 2;
}
