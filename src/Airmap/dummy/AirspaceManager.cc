/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "AirspaceManager.h"
#include "QGCApplication.h"

AirspaceManager::AirspaceManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    qmlRegisterUncreatableType<AirspaceManager>             ("QGroundControl.Airspace",      1, 0, "AirspaceManager",                "Reference only");
}

AirspaceManager::~AirspaceManager()
{
}

void AirspaceManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
}

void AirspaceManager::setROI(const QGeoCoordinate& pointNW, const QGeoCoordinate& pointSE, bool planView, bool reset)
{
    Q_UNUSED(pointNW);
    Q_UNUSED(pointSE);
    Q_UNUSED(planView);
    Q_UNUSED(reset)
}
