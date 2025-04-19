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
#include "UTMSPVehicle.h"

#include "services/dispatcher.h"
#include "Vehicle.h"
#include <QtQml/qqml.h>
#include <QtCore/qapplicationstatic.h>

Q_APPLICATION_STATIC(UTMSPManager, _UTMSPManagerInstance);

UTMSPManager::UTMSPManager(QObject *parent)
    : QObject(parent)
    , _utmspAuthorization(new UTMSPAuthorization(this))
    , _dispatcher(std::make_shared<Dispatcher>())
{
    (void) qmlRegisterUncreatableType<UTMSPManager>("QGroundControl.UTMSP", 1, 0, "UTMSPManager", "Reference only");
    (void) qmlRegisterUncreatableType<UTMSPVehicle>("QGroundControl.UTMSP", 1, 0, "UTMSPVehicle", "Reference only");
    (void) qmlRegisterUncreatableType<UTMSPAuthorization>("QGroundControl.UTMSP", 1, 0, "UTMSPAuthorization", "Reference only");
}

UTMSPManager::~UTMSPManager()
{
    UTMSP_LOG_DEBUG() << "UTMSPManager: Destructor called";
}

UTMSPManager *UTMSPManager::instance()
{
    return _UTMSPManagerInstance();
}

UTMSPAuthorization *UTMSPManager::utmspAuthorization()
{
    return _utmspAuthorization;
}

UTMSPVehicle *UTMSPManager::utmspVehicle()
{
    return _vehicle;
}

UTMSPVehicle* UTMSPManager::instantiateVehicle(Vehicle *vehicle)
{
    _vehicle = new UTMSPVehicle(_dispatcher, vehicle, vehicle);
    return _vehicle;
}

UTMSPAuthorization* UTMSPManager::instantiateUTMSPAuthorization()
{
    return _utmspAuthorization;
}
