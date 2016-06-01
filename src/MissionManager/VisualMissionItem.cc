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
