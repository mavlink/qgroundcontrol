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

#include "ComplexMissionItem.h"

ComplexMissionItem::ComplexMissionItem(Vehicle* vehicle, QObject* parent)
    : VisualMissionItem(vehicle, parent)
    , _dirty(false)
{

}

ComplexMissionItem::ComplexMissionItem(const ComplexMissionItem& other, QObject* parent)
    : VisualMissionItem(other, parent)
    , _dirty(false)
{

}

QVariantList ComplexMissionItem::polygonPath(void)
{
    return _polygonPath;
#if 0
    QVariantList list;

    list << QVariant::fromValue(QGeoCoordinate(-35.362686830000001, 149.16410282999999))
         << QVariant::fromValue(QGeoCoordinate(-35.362660579999996, 149.16606619999999))
         << QVariant::fromValue(QGeoCoordinate(-35.363832989999999, 149.16505769));

    return list;
#endif
}

void ComplexMissionItem::clearPolygon(void)
{
    _polygonPath.clear();
    emit polygonPathChanged();
}

void ComplexMissionItem::addPolygonCoordinate(const QGeoCoordinate coordinate)
{
    _polygonPath << QVariant::fromValue(coordinate);
    emit polygonPathChanged();
}

int ComplexMissionItem::nextSequenceNumber(void) const
{
    return _sequenceNumber + _missionItems.count();
}

void ComplexMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_coordinate != coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(_coordinate);
    }
}

void ComplexMissionItem::setDirty(bool dirty)
{
   if (_dirty != dirty) {
       _dirty = dirty;
       emit dirtyChanged(_dirty);
   }
}

bool ComplexMissionItem::save(QJsonObject& missionObject, QJsonArray& missionItems, QString& errorString)
{
    Q_UNUSED(missionObject);
    Q_UNUSED(missionItems);

    errorString = "Complex save NYI";
    return false;
}
