/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

/**
 * @file AirspaceFlightPlanProvider.h
 * Create and maintain a flight plan
 */

#include "AirspaceAdvisoryProvider.h"
#include "QmlObjectListModel.h"

#include <QObject>
#include <QDateTime>

class MissionController;

//-----------------------------------------------------------------------------
class AirspaceFlightPlanProvider : public QObject
{
    Q_OBJECT
public:

    enum PermitStatus {
        PermitUnknown = 0,
        PermitPending,
        PermitAccepted,
        PermitRejected,
    };

    Q_ENUM(PermitStatus)

    AirspaceFlightPlanProvider                      (QObject *parent = nullptr);
    virtual ~AirspaceFlightPlanProvider             () {}

    Q_PROPERTY(PermitStatus         flightPermitStatus      READ flightPermitStatus                             NOTIFY flightPermitStatusChanged)   ///< State of flight permission
    Q_PROPERTY(QDateTime            flightStartTime         READ flightStartTime    WRITE  setFlightStartTime   NOTIFY flightStartTimeChanged)      ///< Start of flight
    Q_PROPERTY(QDateTime            flightEndTime           READ flightEndTime      WRITE  setFlightEndTime     NOTIFY flightEndTimeChanged)        ///< End of flight
    Q_PROPERTY(bool                 valid                   READ valid                                          NOTIFY advisoryChanged)
    Q_PROPERTY(QmlObjectListModel*  advisories              READ advisories                                     NOTIFY advisoryChanged)
    Q_PROPERTY(QmlObjectListModel*  ruleSets                READ ruleSets                                       NOTIFY advisoryChanged)
    Q_PROPERTY(AirspaceAdvisoryProvider::AdvisoryColor airspaceColor READ airspaceColor                         NOTIFY advisoryChanged)

    virtual PermitStatus        flightPermitStatus  () const { return PermitUnknown; }
    virtual QDateTime           flightStartTime     () const = 0;
    virtual QDateTime           flightEndTime       () const = 0;
    virtual bool                valid               () = 0;                     ///< Current advisory list is valid
    virtual QmlObjectListModel* advisories          () = 0;                     ///< List of AirspaceAdvisory
    virtual QmlObjectListModel* ruleSets            () = 0;                     ///< List of AirspaceRuleSet
    virtual AirspaceAdvisoryProvider::AdvisoryColor  airspaceColor () = 0;      ///< Aispace overall color

    virtual void                setFlightStartTime  (QDateTime start) = 0;
    virtual void                setFlightEndTime    (QDateTime end) = 0;
    virtual void                createFlightPlan    (MissionController* missionController) = 0;

signals:
    void flightPermitStatusChanged                  ();
    void flightStartTimeChanged                     ();
    void flightEndTimeChanged                       ();
    void advisoryChanged                            ();
};
