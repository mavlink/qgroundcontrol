/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LifetimeChecker.h"
#include "AirMapSharedState.h"
#include "AirspaceFlightPlanProvider.h"

#include <QObject>
#include <QTimer>
#include <QList>
#include <QGeoCoordinate>

class PlanMasterController;

//-----------------------------------------------------------------------------
/// class to upload a flight
class AirMapFlightPlanManager : public AirspaceFlightPlanProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapFlightPlanManager                 (AirMapSharedState& shared, QObject *parent = nullptr);
    ~AirMapFlightPlanManager                ();

    PermitStatus        flightPermitStatus  () const override { return _flightPermitStatus; }
    QDateTime           flightStartTime     () const override { return _flightStartTime; }
    QDateTime           flightEndTime       () const override { return _flightEndTime; }
    bool                valid               () override { return _valid; }
    QmlObjectListModel* advisories          () override { return &_advisories; }
    QmlObjectListModel* ruleSets            () override { return &_rulesets; }
    QGCGeoBoundingCube* missionArea         () override { return &_flight.bc; }

    AirspaceAdvisoryProvider::AdvisoryColor
                        airspaceColor       () override { return _airspaceColor; }

    QmlObjectListModel* rulesViolation      () override { return &_rulesViolation; }
    QmlObjectListModel* rulesInfo           () override { return &_rulesInfo; }
    QmlObjectListModel* rulesReview         () override { return &_rulesReview; }
    QmlObjectListModel* rulesFollowing      () override { return &_rulesFollowing; }
    QmlObjectListModel* briefFeatures       () override { return &_briefFeatures; }

    void                updateFlightPlan    () override;
    void                startFlightPlanning (PlanMasterController* planController) override;
    void                setFlightStartTime  (QDateTime start) override;
    void                setFlightEndTime    (QDateTime end) override;
    void                submitFlightPlan    () override;

signals:
    void            error                   (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private slots:
    void _pollBriefing                      ();
    void _missionChanged                    ();

private:
    void _uploadFlightPlan                  ();
    void _updateFlightPlan                  ();
    void _createFlightPlan                  ();
    void _deleteFlightPlan                  ();
    bool _collectFlightDtata                ();

private:
    enum class State {
        Idle,
        GetPilotID,
        FlightUpload,
        FlightUpdate,
        FlightPlanPolling,
        FlightDelete,
        FlightSubmit,
        FlightPolling,
    };

    struct Flight {
        QGCGeoBoundingCube bc;
        QList<QGeoCoordinate> coords;
        QGeoCoordinate  takeoffCoord;
        float maxAltitude = 0;
        void reset() {
            bc.reset();
            coords.clear();
            maxAltitude = 0;
        }
    };

    Flight                  _flight;                ///< flight pending to be uploaded
    State                   _state = State::Idle;
    AirMapSharedState&      _shared;
    QTimer                  _pollTimer;             ///< timer to poll for approval check
    QString                 _flightPlan;            ///< Current flight plan
    QString                 _flightId;              ///< Current flight ID, not necessarily accepted yet
    QString                 _pilotID;               ///< Pilot ID in the form "auth0|abc123"
    PlanMasterController*   _planController = nullptr;
    bool                    _valid = false;
    QDateTime               _flightStartTime;
    QDateTime               _flightEndTime;
    QmlObjectListModel      _advisories;
    QmlObjectListModel      _rulesets;
    QmlObjectListModel      _rulesViolation;
    QmlObjectListModel      _rulesInfo;
    QmlObjectListModel      _rulesReview;
    QmlObjectListModel      _rulesFollowing;
    QmlObjectListModel      _briefFeatures;

    AirspaceAdvisoryProvider::AdvisoryColor  _airspaceColor;
    AirspaceFlightPlanProvider::PermitStatus _flightPermitStatus = AirspaceFlightPlanProvider::PermitNone;

};

