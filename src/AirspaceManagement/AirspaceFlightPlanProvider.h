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
#include <QAbstractListModel>

class PlanMasterController;
class AirspaceFlightInfo;

//-----------------------------------------------------------------------------
class AirspaceFlightAuthorization : public QObject
{
    Q_OBJECT
public:
    AirspaceFlightAuthorization                     (QObject *parent = nullptr);

    enum AuthorizationStatus {
        Accepted,
        Rejected,
        Pending,
        AcceptedOnSubmission,
        RejectedOnSubmission,
        Unknown
    };

    Q_ENUM(AuthorizationStatus)

    Q_PROPERTY(QString              name            READ name           CONSTANT)
    Q_PROPERTY(QString              id              READ id             CONSTANT)
    Q_PROPERTY(AuthorizationStatus  status          READ status         CONSTANT)
    Q_PROPERTY(QString              message         READ message        CONSTANT)

    virtual QString                 name            () = 0;
    virtual QString                 id              () = 0;
    virtual AuthorizationStatus     status          () = 0;
    virtual QString                 message         () = 0;

};


//-----------------------------------------------------------------------------
class AirspaceFlightInfo : public QObject
{
    Q_OBJECT
public:
    AirspaceFlightInfo                              (QObject *parent = nullptr);

    Q_PROPERTY(QString              flightID        READ flightID       CONSTANT)
    Q_PROPERTY(QString              flightPlanID    READ flightPlanID   CONSTANT)
    Q_PROPERTY(QString              createdTime     READ createdTime    CONSTANT)
    Q_PROPERTY(QString              startTime       READ startTime      CONSTANT)
    Q_PROPERTY(QString              endTime         READ endTime        CONSTANT)
    Q_PROPERTY(QGeoCoordinate       takeOff         READ takeOff        CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  boundingBox     READ boundingBox    CONSTANT)
    Q_PROPERTY(bool                 beingDeleted    READ beingDeleted   WRITE  setBeingDeleted  NOTIFY beingDeletedChanged)
    Q_PROPERTY(bool                 selected        READ selected       WRITE  setSelected      NOTIFY selectedChanged)

    virtual QString                 flightID        () = 0;
    virtual QString                 flightPlanID    () = 0;
    virtual QString                 createdTime     () = 0;
    virtual QString                 startTime       () = 0;
    virtual QString                 endTime         () = 0;
    virtual QGeoCoordinate          takeOff         () = 0;
    virtual QmlObjectListModel*     boundingBox     () = 0;

    virtual bool                    beingDeleted    () { return _beingDeleted; }
    virtual void                    setBeingDeleted (bool val) { _beingDeleted = val; emit beingDeletedChanged(); }
    virtual bool                    selected        () { return _selected; }
    virtual void                    setSelected     (bool sel) { _selected = sel; emit selectedChanged(); }

signals:
    void    selectedChanged                         ();
    void    beingDeletedChanged                     ();

protected:
    bool    _beingDeleted;
    bool    _selected;
};

//-----------------------------------------------------------------------------
class AirspaceFlightModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum QGCLogModelRoles {
        ObjectRole = Qt::UserRole + 1
    };

    AirspaceFlightModel         (QObject *parent = 0);

    Q_PROPERTY(int count READ count NOTIFY countChanged)

    Q_INVOKABLE AirspaceFlightInfo* get                 (int index);
    Q_INVOKABLE int                 findFlightPlanID    (QString flightPlanID);

    int         count           (void) const;
    void        append          (AirspaceFlightInfo *entry);
    void        remove          (const QString& flightPlanID);
    void        remove          (int index);
    void        clear           (void);

    AirspaceFlightInfo*
                operator[]      (int i);

    int         rowCount        (const QModelIndex & parent = QModelIndex()) const;
    QVariant    data            (const QModelIndex & index, int role = Qt::DisplayRole) const;

signals:
    void        countChanged    ();

protected:
    QHash<int, QByteArray> roleNames() const;
private:
    QList<AirspaceFlightInfo*> _flightEntries;
};

//-----------------------------------------------------------------------------
class AirspaceFlightPlanProvider : public QObject
{
    Q_OBJECT
public:

    enum PermitStatus {
        PermitNone = 0,     //-- No flght plan
        PermitPending,
        PermitAccepted,
        PermitRejected,
    };

    Q_ENUM(PermitStatus)

    AirspaceFlightPlanProvider                      (QObject *parent = nullptr);

    ///< Flight Planning and Filing
    Q_PROPERTY(QDateTime            flightStartTime         READ flightStartTime    WRITE  setFlightStartTime   NOTIFY flightStartTimeChanged)      ///< Start of flight
    Q_PROPERTY(QDateTime            flightEndTime           READ flightEndTime      WRITE  setFlightEndTime     NOTIFY flightEndTimeChanged)        ///< End of flight

    ///< Flight Briefing
    Q_PROPERTY(PermitStatus         flightPermitStatus      READ flightPermitStatus                             NOTIFY flightPermitStatusChanged)   ///< State of flight permission
    Q_PROPERTY(bool                 valid                   READ valid                                          NOTIFY advisoryChanged)
    Q_PROPERTY(QmlObjectListModel*  advisories              READ advisories                                     NOTIFY advisoryChanged)
    Q_PROPERTY(QmlObjectListModel*  ruleSets                READ ruleSets                                       NOTIFY advisoryChanged)
    Q_PROPERTY(QGCGeoBoundingCube*  missionArea             READ missionArea                                    NOTIFY missionAreaChanged)
    Q_PROPERTY(AirspaceAdvisoryProvider::AdvisoryColor airspaceColor READ airspaceColor                         NOTIFY advisoryChanged)
    Q_PROPERTY(QmlObjectListModel*  rulesViolation          READ rulesViolation                                 NOTIFY rulesChanged)
    Q_PROPERTY(QmlObjectListModel*  rulesInfo               READ rulesInfo                                      NOTIFY rulesChanged)
    Q_PROPERTY(QmlObjectListModel*  rulesReview             READ rulesReview                                    NOTIFY rulesChanged)
    Q_PROPERTY(QmlObjectListModel*  rulesFollowing          READ rulesFollowing                                 NOTIFY rulesChanged)
    Q_PROPERTY(QmlObjectListModel*  briefFeatures           READ briefFeatures                                  NOTIFY rulesChanged)
    Q_PROPERTY(QmlObjectListModel*  authorizations          READ authorizations                                 NOTIFY rulesChanged)

    ///< Flight Management
    Q_PROPERTY(AirspaceFlightModel* flightList              READ flightList                                     NOTIFY flightListChanged)
    Q_PROPERTY(bool                 loadingFlightList       READ loadingFlightList                              NOTIFY loadingFlightListChanged)

    //-- TODO: This will submit the current flight plan in memory.
    Q_INVOKABLE virtual void    submitFlightPlan            () = 0;
    Q_INVOKABLE virtual void    updateFlightPlan            () = 0;
    Q_INVOKABLE virtual void    loadFlightList              (QDateTime startTime, QDateTime endTime) = 0;
    Q_INVOKABLE virtual void    deleteFlight            (QString flighPlanID) = 0;
    Q_INVOKABLE virtual void    deleteSelectedFlights       () = 0;

    virtual PermitStatus        flightPermitStatus  () const { return PermitNone; }
    virtual QDateTime           flightStartTime     () const = 0;
    virtual QDateTime           flightEndTime       () const = 0;
    virtual QGCGeoBoundingCube* missionArea         () = 0;
    virtual bool                valid               () = 0;                     ///< Current advisory list is valid
    virtual QmlObjectListModel* advisories          () = 0;                     ///< List of AirspaceAdvisory
    virtual QmlObjectListModel* ruleSets            () = 0;                     ///< List of AirspaceRuleSet
    virtual AirspaceAdvisoryProvider::AdvisoryColor  airspaceColor () = 0;      ///< Aispace overall color

    virtual QmlObjectListModel* rulesViolation      () = 0;                     ///< List of AirspaceRule in violation
    virtual QmlObjectListModel* rulesInfo           () = 0;                     ///< List of AirspaceRule need more information
    virtual QmlObjectListModel* rulesReview         () = 0;                     ///< List of AirspaceRule should review
    virtual QmlObjectListModel* rulesFollowing      () = 0;                     ///< List of AirspaceRule following
    virtual QmlObjectListModel* briefFeatures       () = 0;                     ///< List of AirspaceRule in violation
    virtual QmlObjectListModel* authorizations      () = 0;                     ///< List of AirspaceFlightAuthorization
    virtual AirspaceFlightModel*flightList          () = 0;                     ///< List of AirspaceFlightInfo
    virtual bool                loadingFlightList   () = 0;

    virtual void                setFlightStartTime  (QDateTime start) = 0;
    virtual void                setFlightEndTime    (QDateTime end) = 0;
    virtual void                startFlightPlanning (PlanMasterController* planController) = 0;

signals:
    void flightPermitStatusChanged                  ();
    void flightStartTimeChanged                     ();
    void flightEndTimeChanged                       ();
    void advisoryChanged                            ();
    void missionAreaChanged                         ();
    void rulesChanged                               ();
    void flightListChanged                          ();
    void loadingFlightListChanged                   ();
};
