/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>

class UTMSPVehicle;
class Vehicle;
class Dispatcher;
class UTMSPAuthorization;

class UTMSPManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("UTMSPVehicle.h")
    Q_MOC_INCLUDE("UTMSPAuthorization.h")
    Q_PROPERTY(UTMSPVehicle *utmspVehicle READ utmspVehicle CONSTANT)
    Q_PROPERTY(UTMSPAuthorization *utmspAuthorization READ utmspAuthorization CONSTANT)

public:
    explicit UTMSPManager(QObject *parent = nullptr);
    ~UTMSPManager();

    static UTMSPManager *instance();

    UTMSPVehicle *instantiateVehicle(Vehicle *vehicle);
    UTMSPAuthorization *instantiateUTMSPAuthorization();
    UTMSPAuthorization *utmspAuthorization();
    UTMSPVehicle *utmspVehicle();

private:
    UTMSPVehicle *_vehicle = nullptr;
    UTMSPAuthorization *_utmspAuthorization = nullptr;
    std::shared_ptr<Dispatcher> _dispatcher;
};
