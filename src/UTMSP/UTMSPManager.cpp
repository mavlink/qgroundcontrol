/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "UTMSPManager.h"
#include "UTMSPLogger.h"

#include "services/dispatcher.h"
#include "Vehicle.h"
#include "qqml.h"

UTMSPManager::UTMSPManager(QGCApplication* app, QGCToolbox* toolbox) :
    QGCTool(app, toolbox),
    _dispatcher(std::make_shared<Dispatcher>())
{
    _utmspAuthorization = new UTMSPAuthorization();
    qmlRegisterUncreatableType<UTMSPManager>              ("QGroundControl.UTMSP",      1, 0, "UTMSPManager",                "Reference only");
    qmlRegisterUncreatableType<UTMSPVehicle>              ("QGroundControl.UTMSP",      1, 0, "UTMSPVehicle",                "Reference only");
    qmlRegisterUncreatableType<UTMSPAuthorization>        ("QGroundControl.UTMSP",      1, 0, "UTMSPAuthorization",          "Reference only");
}

UTMSPManager::~UTMSPManager()
{
    UTMSP_LOG_DEBUG() << "UTMSPManager: Destructor called";
    delete _utmspAuthorization;
    _utmspAuthorization = nullptr;
}

void UTMSPManager::setToolbox(QGCToolbox* toolbox)
{
    UTMSP_LOG_INFO() << "UTMSPManager: ToolBox Set" ;
    QGCTool::setToolbox(toolbox);
}

UTMSPVehicle* UTMSPManager::instantiateVehicle(const Vehicle& vehicle)
{
    // TODO: Investigate safe deletion of pointer in this modification of having a member pointer
    _vehicle = new UTMSPVehicle(_dispatcher,vehicle);

    return _vehicle;
}

UTMSPAuthorization* UTMSPManager::instantiateUTMSPAuthorization(){

    return _utmspAuthorization;
}
