/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include <QtCore/qapplicationstatic.h>

Q_APPLICATION_STATIC(UTMSPManager, _UTMSPManager);

UTMSPManager::UTMSPManager(QObject *parent)
    : QObject(parent)
    , _dispatcher(std::make_shared<Dispatcher>())
    , _utmspAuthorization(new UTMSPAuthorization(this))
{
    qmlRegisterUncreatableType<UTMSPManager>              ("QGroundControl.UTMSP",      1, 0, "UTMSPManager",                "Reference only");
    qmlRegisterUncreatableType<UTMSPVehicle>              ("QGroundControl.UTMSP",      1, 0, "UTMSPVehicle",                "Reference only");
    qmlRegisterUncreatableType<UTMSPAuthorization>        ("QGroundControl.UTMSP",      1, 0, "UTMSPAuthorization",          "Reference only");
}

UTMSPManager::~UTMSPManager()
{
    UTMSP_LOG_DEBUG() << "UTMSPManager: Destructor called";
}

UTMSPManager *UTMSPManager::instance()
{
    return _UTMSPManager();
}

void UTMSPManager::instantiateVehicle(Vehicle *vehicle)
{
    // TODO: Investigate safe deletion of pointer in this modification of having a member pointer
    _vehicle = new UTMSPVehicle(_dispatcher, vehicle, vehicle);
}

UTMSPAuthorization* UTMSPManager::instantiateUTMSPAuthorization()
{
    return _utmspAuthorization;
}
