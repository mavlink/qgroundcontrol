/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapController.h"
#include "AirMapManager.h"
#include "QGCApplication.h"
#include "QGCQGeoCoordinate.h"

QGC_LOGGING_CATEGORY(AirMapControllerLog, "AirMapControllerLog")

AirMapController::AirMapController(QObject* parent)
    : QObject(parent)
    , _manager(qgcApp()->toolbox()->airMapManager())
{
    connect(_manager, &AirMapManager::flightPermitStatusChanged, this, &AirMapController::flightPermitStatusChanged);
}

AirMapController::~AirMapController()
{

}
