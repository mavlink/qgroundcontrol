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

#include <QObject>

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

    AirspaceFlightPlanProvider              (QObject *parent = nullptr);
    virtual ~AirspaceFlightPlanProvider     () {}

    Q_PROPERTY(PermitStatus  flightPermitStatus     READ flightPermitStatus  NOTIFY flightPermitStatusChanged)   ///< state of flight permission

    virtual PermitStatus flightPermitStatus () const { return PermitUnknown; }
    virtual void         createFlight       (MissionController* missionController) = 0;


signals:
    void flightPermitStatusChanged          ();
};
