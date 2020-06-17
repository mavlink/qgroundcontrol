/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include <QDebug>

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
    Q_PROPERTY(QVariantList         boundingBox     READ boundingBox    CONSTANT)
    Q_PROPERTY(bool                 active          READ active         NOTIFY activeChanged)

    virtual QString                 flightID        () = 0;
    virtual QString                 flightPlanID    () = 0;
    virtual QString                 createdTime     () = 0;
    virtual QString                 startTime       () = 0;
    virtual QDateTime               qStartTime      () = 0;
    virtual QString                 endTime         () = 0;
    virtual QGeoCoordinate          takeOff         () = 0;
    virtual QVariantList            boundingBox     () = 0;
    virtual bool                    active          () = 0;

signals:
    void    activeChanged                           ();
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
    Q_INVOKABLE int                 findFlightID        (QString flightID);

    int         count           () const;
    void        append          (AirspaceFlightInfo *entry);
    void        remove          (const QString& flightID);
    void        remove          (int index);
    void        clear           ();
    void        sortStartFlight ();

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
        PermitNotRequired,
    };

    Q_ENUM(PermitStatus)

    AirspaceFlightPlanProvider                      (QObject *parent = nullptr);

    ///< Flight Planning and Filing
    Q_PROPERTY(QDateTime            flightStartTime         READ flightStartTime    WRITE  setFlightStartTime   NOTIFY flightStartTimeChanged)      ///< Start of flight
    Q_PROPERTY(int                  flightDuration          READ flightDuration     WRITE  setFlightDuration    NOTIFY flightDurationChanged)       ///< Flight Duration
    Q_PROPERTY(bool                 flightStartsNow         READ flightStartsNow    WRITE  setFlightStartsNow   NOTIFY flightStartsNowChanged)

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
    Q_PROPERTY(bool                 dirty                   READ dirty                  WRITE setDirty          NOTIFY dirtyChanged)

    //-- TODO: This will submit the current flight plan in memory.
    Q_INVOKABLE virtual void    submitFlightPlan            () = 0;
    Q_INVOKABLE virtual void    updateFlightPlan            () = 0;
    Q_INVOKABLE virtual void    loadFlightList              (QDateTime startTime, QDateTime endTime) = 0;
    Q_INVOKABLE virtual void    endFlight                   (QString flighID) = 0;

    virtual PermitStatus        flightPermitStatus  () const { return PermitNone; }
    virtual QDateTime           flightStartTime     () const = 0;
    virtual int                 flightDuration      () const = 0;
    virtual bool                flightStartsNow     () const = 0;
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
    virtual bool                dirty               () { return _dirty; }

    virtual void                setFlightStartTime  (QDateTime start) = 0;
    virtual void                setFlightDuration   (int seconds) = 0;
    virtual void                setFlightStartsNow  (bool now) = 0;
    virtual void                startFlightPlanning (PlanMasterController* planController) = 0;
    virtual void                setDirty            (bool dirt) { if(_dirty != dirt) { _dirty = dirt; emit dirtyChanged(); }}

signals:
    void flightPermitStatusChanged                  ();
    void flightStartTimeChanged                     ();
    void flightStartsNowChanged                     ();
    void flightDurationChanged                      ();
    void advisoryChanged                            ();
    void missionAreaChanged                         ();
    void rulesChanged                               ();
    void flightListChanged                          ();
    void loadingFlightListChanged                   ();
    void dirtyChanged                               ();

protected:
    bool _dirty = false;
};
