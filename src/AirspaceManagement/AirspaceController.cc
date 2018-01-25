/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapManager.h"
#include "AirspaceController.h"
#include "AirspaceManagement.h"
#include "QGCApplication.h"
#include "QGCQGeoCoordinate.h"

#define WEATHER_UPDATE_DISTANCE 50000                   //-- 50km threshold for weather updates
#define WEATHER_UPDATE_TIME     30 * 60 * 60 * 1000     //-- 30 minutes threshold for weather updates

AirspaceController::AirspaceController(QObject* parent)
    : QObject(parent)
    , _manager(qgcApp()->toolbox()->airspaceManager())
{
    connect(_manager, &AirspaceManager::weatherUpdate, this, &AirspaceController::_weatherUpdate);
}

void AirspaceController::setROI(QGeoCoordinate center, double radius)
{
    _manager->setROI(center, radius);
}
