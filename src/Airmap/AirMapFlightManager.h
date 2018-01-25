/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QTimer>

#include "AirMapManager.h"

//-----------------------------------------------------------------------------
/// class to upload a flight
class AirMapFlightManager : public QObject, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapFlightManager(AirMapSharedState& shared);

    /// Send flight path to AirMap
    void createFlight(const QList<MissionItem*>& missionItems);

    AirspaceAuthorization::PermitStatus flightPermitStatus() const { return _flightPermitStatus; }

    const QString& flightID() const { return _currentFlightId; }

public slots:
    void endFlight();

signals:
    void error(const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
    void flightPermitStatusChanged();

private slots:
    void _pollBriefing();

private:

    /**
     * upload flight stored in _flight
     */
    void _uploadFlight();

    /**
     * query the active flights and end the first one (because only a single flight can be active at a time).
     */
    void _endFirstFlight();

    /**
     * implementation of endFlight()
     */
    void _endFlight(const QString& flightID);

    /**
     * check if the briefing response is valid and call _submitPendingFlightPlan() if it is.
     */
    void _checkForValidBriefing();

    void _submitPendingFlightPlan();

    enum class State {
        Idle,
        GetPilotID,
        FlightUpload,
        FlightBrief,
        FlightSubmit,
        FlightPolling, // poll & check for approval
        FlightEnd,
        EndFirstFlight, // get a list of open flights & end the first one (because there can only be 1 active at a time)
    };
    struct Flight {
        QList<QGeoCoordinate> coords;
        QGeoCoordinate takeoffCoord;
        float maxAltitude = 0;

        void reset() {
            coords.clear();
            maxAltitude = 0;
        }
    };
    Flight                              _flight; ///< flight pending to be uploaded

    State                               _state = State::Idle;
    AirMapSharedState&                  _shared;
    QString                             _currentFlightId; ///< Flight ID, empty if there is none
    QString                             _pendingFlightId; ///< current flight ID, not necessarily accepted yet (once accepted, it's equal to _currentFlightId)
    QString                             _pendingFlightPlan; ///< current flight plan, waiting to be submitted
    AirspaceAuthorization::PermitStatus _flightPermitStatus = AirspaceAuthorization::PermitUnknown;
    QString                             _pilotID; ///< Pilot ID in the form "auth0|abc123"
    bool                                _noFlightCreatedYet = true;
    QTimer                              _pollTimer; ///< timer to poll for approval check
};

