/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirspaceController.h"
#include "AirspaceManagement.h"
#include "QGCApplication.h"
#include "QGCQGeoCoordinate.h"

AirspaceController::AirspaceController(QObject* parent)
    : QObject(parent)
    , _manager(qgcApp()->toolbox()->airspaceManager())
{
}

