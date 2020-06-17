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
#include "AirspaceFlightPlanProvider.h"

#include <QTimer>
#include <QObject>
#include <QList>
#include <QGeoCoordinate>

//-----------------------------------------------------------------------------
/// class to upload a flight
class AirMapFlightManager : public QObject, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapFlightManager             (AirMapSharedState& shared);

    void    findFlight              (const QGCGeoBoundingCube& bc);
    void    endFlight               (const QString& id);
    QString flightID                () { return _flightID; }

signals:
    void    error                   (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void    flightIDChanged         ();

private:

    enum class State {
        Idle,
        GetPilotID,
        FetchFlights,
        FlightEnd,
    };

    State                               _state = State::Idle;
    AirMapSharedState&                  _shared;
    QString                             _flightID;
    QGCGeoBoundingCube                  _searchArea;
};

