/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "LinkInterface.h"
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(VehicleLinkManagerLog)

class Vehicle;
class VehicleLinkManagerTest;

class VehicleLinkManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(QString      primaryLinkName             READ primaryLinkName            WRITE setPrimaryLinkByName          NOTIFY primaryLinkChanged)
    Q_PROPERTY(QStringList  linkNames                   READ linkNames                                                      NOTIFY linkNamesChanged)
    Q_PROPERTY(QStringList  linkStatuses                READ linkStatuses                                                   NOTIFY linkStatusesChanged)
    Q_PROPERTY(bool         communicationLost           READ communicationLost                                              NOTIFY communicationLostChanged)
    Q_PROPERTY(bool         communicationLostEnabled    READ communicationLostEnabled   WRITE setCommunicationLostEnabled   NOTIFY communicationLostEnabledChanged)
    Q_PROPERTY(bool         autoDisconnect              MEMBER _autoDisconnect                                              NOTIFY autoDisconnectChanged)

    friend class Vehicle;
    friend class VehicleLinkManagerTest;

public:
    VehicleLinkManager(Vehicle *vehicle);
    ~VehicleLinkManager();

    void mavlinkMessageReceived(LinkInterface *link, const mavlink_message_t &message);
    bool containsLink(LinkInterface *link);
    WeakLinkInterfacePtr primaryLink() const { return _primaryLink; }
    QString primaryLinkName() const;
    QStringList linkNames() const;
    QStringList linkStatuses() const;
    bool communicationLost() const { return _communicationLost; }
    bool communicationLostEnabled() const { return _communicationLostEnabled; }
    void setPrimaryLinkByName(const QString &name);
    void setCommunicationLostEnabled(bool communicationLostEnabled);
    void closeVehicle();

signals:
    void primaryLinkChanged();
    void allLinksRemoved(Vehicle *vehicle);
    void communicationLostChanged(bool communicationLost);
    void communicationLostEnabledChanged(bool communicationLostEnabled);
    void linkNamesChanged();
    void linkStatusesChanged();
    void autoDisconnectChanged(bool autoDisconnect);

private slots:
    void _commLostCheck();

private:
    int _containsLinkIndex(const LinkInterface *link);
    void _addLink(LinkInterface *link);
    void _removeLink(LinkInterface *link);
    void _linkDisconnected();
    bool _updatePrimaryLink();
    SharedLinkInterfacePtr _bestActivePrimaryLink();
    void _commRegainedOnLink(LinkInterface *link);

    struct LinkInfo_t {
        SharedLinkInterfacePtr link;
        bool commLost = false;
        QElapsedTimer heartbeatElapsedTimer;
    };

    Vehicle *_vehicle = nullptr;
    QTimer *_commLostCheckTimer = nullptr;
    QList<LinkInfo_t> _rgLinkInfo;
    WeakLinkInterfacePtr _primaryLink;
    bool _communicationLost = false;
    bool _communicationLostEnabled = true;
    bool _autoDisconnect = false;                           ///< true: Automatically disconnect vehicle when last connection goes away or lost heartbeat

    static constexpr int _commLostCheckTimeoutMSecs = 1000; ///< Check for comm lost once a second
    static constexpr int _heartbeatMaxElpasedMSecs = 3500;  ///< No heartbeat for longer than this indicates comm loss
};
