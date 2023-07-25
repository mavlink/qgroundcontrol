/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QTimer>
#include <QDebug>
#include <QLoggingCategory>
#include <QElapsedTimer>

#include "QGCMAVLink.h"
#include "LinkInterface.h"

Q_DECLARE_LOGGING_CATEGORY(VehicleLinkManagerLog)

class Vehicle;
class LinkManager;
class VehicleLinkManagerTest;

class VehicleLinkManager : public QObject
{
    Q_OBJECT

    friend class Vehicle;
    friend class VehicleLinkManagerTest;

public:
    VehicleLinkManager(Vehicle* vehicle);

    Q_PROPERTY(bool             primaryLinkIsPX4Flow        READ primaryLinkIsPX4Flow                                           NOTIFY primaryLinkChanged)
    Q_PROPERTY(QString          primaryLinkName             READ primaryLinkName            WRITE setPrimaryLinkByName          NOTIFY primaryLinkChanged)
    Q_PROPERTY(QStringList      linkNames                   READ linkNames                                                      NOTIFY linkNamesChanged)
    Q_PROPERTY(QStringList      linkStatuses                READ linkStatuses                                                   NOTIFY linkStatusesChanged)
    Q_PROPERTY(bool             communicationLost           READ communicationLost                                              NOTIFY communicationLostChanged)
    Q_PROPERTY(bool             communicationLostEnabled    READ communicationLostEnabled   WRITE setCommunicationLostEnabled   NOTIFY communicationLostEnabledChanged)
    Q_PROPERTY(bool             autoDisconnect              MEMBER _autoDisconnect                                              NOTIFY autoDisconnectChanged)

    bool                    primaryLinkIsPX4Flow        (void) const;
    void                    mavlinkMessageReceived      (LinkInterface* link, mavlink_message_t message);
    bool                    containsLink                (LinkInterface* link);
    WeakLinkInterfacePtr    primaryLink                 (void) { return _primaryLink; }
    QString                 primaryLinkName             (void) const;
    QStringList             linkNames                   (void) const;
    QStringList             linkStatuses                (void) const;
    bool                    communicationLost           (void) const { return _communicationLost; }
    bool                    communicationLostEnabled    (void) const { return _communicationLostEnabled; }
    void                    setPrimaryLinkByName        (const QString& name);
    void                    setCommunicationLostEnabled (bool communicationLostEnabled);
    void                    closeVehicle                (void);

signals:
    void primaryLinkChanged             (void);
    void allLinksRemoved                (Vehicle* vehicle);
    void communicationLostChanged       (bool communicationLost);
    void communicationLostEnabledChanged(bool communicationLostEnabled);
    void linkNamesChanged               (void);
    void linkStatusesChanged            (void);
    void autoDisconnectChanged          (bool autoDisconnect);

private slots:
    void _commLostCheck(void);

private:
    int                     _containsLinkIndex      (LinkInterface* link);
    void                    _addLink                (LinkInterface* link);
    void                    _removeLink             (LinkInterface* link);
    void                    _linkDisconnected       (void);
    bool                    _updatePrimaryLink      (void);
    SharedLinkInterfacePtr  _bestActivePrimaryLink  (void);
    void                    _commRegainedOnLink     (LinkInterface*  link);

    typedef struct LinkInfo {
        SharedLinkInterfacePtr  link;
        bool                    commLost = false;
        QElapsedTimer           heartbeatElapsedTimer;
    } LinkInfo_t;

    Vehicle*                _vehicle                    = nullptr;
    LinkManager*            _linkMgr                    = nullptr;
    QTimer                  _commLostCheckTimer;
    QList<LinkInfo_t>       _rgLinkInfo;
    WeakLinkInterfacePtr    _primaryLink;
    bool                    _communicationLost          = false;
    bool                    _communicationLostEnabled   = true;
    bool                    _autoDisconnect             = false;    ///< true: Automatically disconnect vehicle when last connection goes away or lost heartbeat

    static const int _commLostCheckTimeoutMSecs     = 1000;  // Check for comm lost once a second
    static const int _heartbeatMaxElpasedMSecs      = 3500;  // No heartbeat for longer than this indicates comm loss
};
