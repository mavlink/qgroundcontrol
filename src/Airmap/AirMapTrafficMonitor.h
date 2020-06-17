/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LifetimeChecker.h"
#include "AirMapSharedState.h"

#include <QObject>
#include <QGeoCoordinate>

#include "airmap/traffic.h"

#include <memory>

/**
 * @class AirMapTrafficMonitor
 *
 */

class AirMapTrafficMonitor : public QObject, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapTrafficMonitor            (AirMapSharedState& shared);
    virtual ~AirMapTrafficMonitor   ();

    void startConnection            (const QString& flightID);

    void stop();

signals:
    void error                      (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void trafficUpdate              (bool alert, QString traffic_id, QString vehicle_id, QGeoCoordinate location, float heading);

private:
    void _update                    (airmap::Traffic::Update::Type type, const std::vector<airmap::Traffic::Update>& update);

private:
    QString                                               _flightID;
    AirMapSharedState&                                    _shared;
    std::shared_ptr<airmap::Traffic::Monitor>             _monitor;
    std::shared_ptr<airmap::Traffic::Monitor::Subscriber> _subscriber;
};


