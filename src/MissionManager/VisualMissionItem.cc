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

#include <QStringList>
#include <QDebug>

#include "VisualMissionItem.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "JsonHelper.h"

VisualMissionItem::VisualMissionItem(Vehicle* vehicle, QObject* parent)
    : QObject(parent)
    , _vehicle(vehicle)
    , _isCurrentItem(false)
    , _dirty(false)
    , _altDifference(0.0)
    , _altPercent(0.0)
    , _azimuth(0.0)
    , _distance(0.0)
{

}

VisualMissionItem::VisualMissionItem(const VisualMissionItem& other, QObject* parent)
    : QObject(parent)
    , _vehicle(NULL)
    , _isCurrentItem(false)
    , _dirty(false)
    , _altDifference(0.0)
    , _altPercent(0.0)
    , _azimuth(0.0)
    , _distance(0.0)
{
    *this = other;
}

const VisualMissionItem& VisualMissionItem::operator=(const VisualMissionItem& other)
{
    _vehicle = other._vehicle;

    setIsCurrentItem(other._isCurrentItem);
    setDirty(other._dirty);
    setAltDifference(other._altDifference);
    setAltPercent(other._altPercent);
    setAzimuth(other._azimuth);
    setDistance(other._distance);

    return *this;
}

VisualMissionItem::~VisualMissionItem()
{    
}

void VisualMissionItem::setIsCurrentItem(bool isCurrentItem)
{
    if (_isCurrentItem != isCurrentItem) {
        _isCurrentItem = isCurrentItem;
        emit isCurrentItemChanged(isCurrentItem);
    }
}

void VisualMissionItem::setDistance(double distance)
{
    _distance = distance;
    emit distanceChanged(_distance);
}

void VisualMissionItem::setAltDifference(double altDifference)
{
    _altDifference = altDifference;
    emit altDifferenceChanged(_altDifference);
}

void VisualMissionItem::setAltPercent(double altPercent)
{
    _altPercent = altPercent;
    emit altPercentChanged(_altPercent);
}

void VisualMissionItem::setAzimuth(double azimuth)
{
    _azimuth = azimuth;
    emit azimuthChanged(_azimuth);
}
