/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"

#include "UTMSPVehicle.h"

class UTMSPVehicle;
class Vehicle;
class Dispatcher;
class UTMSPAuthorization;

class UTMSPManager : public QGCTool
{
    Q_OBJECT
    Q_MOC_INCLUDE("UTMSPAuthorization.h")

public:
    UTMSPManager(QGCApplication* app, QGCToolbox* toolbox);
    virtual ~UTMSPManager();

    Q_PROPERTY(UTMSPVehicle*            utmspVehicle                    READ utmspVehicle                   CONSTANT)
    Q_PROPERTY(UTMSPAuthorization*      utmspAuthorization              READ utmspAuthorization             CONSTANT)

    void setToolbox (QGCToolbox* toolbox);

    UTMSPVehicle*               instantiateVehicle              (const Vehicle& vehicle);
    UTMSPAuthorization*         instantiateUTMSPAuthorization   (void);
    UTMSPAuthorization*         utmspAuthorization              (void)  { return _utmspAuthorization;}
    UTMSPVehicle*               utmspVehicle                    (void ) {return _vehicle;};

private:
    UTMSPVehicle*                        _vehicle                 = nullptr;
    UTMSPAuthorization*                  _utmspAuthorization      = nullptr;
    std::shared_ptr<Dispatcher>          _dispatcher;
};
