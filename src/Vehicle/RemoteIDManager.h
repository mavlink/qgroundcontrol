#pragma once

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoPositionInfo>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkMessageType.h"

class RemoteIDSettings;
class Vehicle;

// Supporting Open Drone ID protocol
class RemoteIDManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    RemoteIDManager(Vehicle* vehicle);

    Q_PROPERTY(bool    available            READ available          NOTIFY availableChanged)             ///< true: the vehicle supports Mavlink Open Drone ID messages
    Q_PROPERTY(bool    armStatusGoodToArm   READ armStatusGoodToArm NOTIFY armStatusGoodToArmChanged)    ///< true: RID device reports MAV_ODID_ARM_STATUS_GOOD_TO_ARM
    Q_PROPERTY(QString armStatusError       READ armStatusError     NOTIFY armStatusErrorChanged)
    Q_PROPERTY(bool    ridDeviceCommsGood   READ ridDeviceCommsGood NOTIFY ridDeviceCommsGoodChanged)    ///< true: receiving ODID_ARM_STATUS from the RID device at the expected rate
    Q_PROPERTY(bool    gcsPositionUsable    READ gcsPositionUsable  NOTIFY gcsPositionUsableChanged)     ///< true: GCS position is valid/fresh enough to broadcast in OPEN_DRONE_ID_SYSTEM
    Q_PROPERTY(bool    vehicleReportsBasicIDMissing READ vehicleReportsBasicIDMissing NOTIFY vehicleReportsBasicIDMissingChanged) ///< true: RID device reported it has no basic ID from device parameters or GCS
    Q_PROPERTY(bool    emergencyDeclared    READ emergencyDeclared  NOTIFY emergencyDeclaredChanged)

    // Declare emergency
    Q_INVOKABLE void setEmergency(bool declare);

    bool    available           (void) const { return _available; }
    bool    armStatusGoodToArm  (void) const { return _armStatusGoodToArm; }
    QString armStatusError      (void) const { return _armStatusError; }
    bool    ridDeviceCommsGood  (void) const { return _ridDeviceCommsGood; }
    bool    gcsPositionUsable   (void) const { return _gcsPositionUsable; }
    bool    vehicleReportsBasicIDMissing(void) const { return _vehicleReportsBasicIDMissing; }
    bool    emergencyDeclared   (void) const { return _emergencyDeclared;}

    void mavlinkMessageReceived (mavlink_message_t& message);

    enum LocationTypes {
        TAKEOFF,
        LiveGNSS,
        FIXED
    };

signals:
    void availableChanged();
    void armStatusGoodToArmChanged();
    void armStatusErrorChanged();
    void ridDeviceCommsGoodChanged();
    void gcsPositionUsableChanged();
    void vehicleReportsBasicIDMissingChanged();
    void emergencyDeclaredChanged();

private slots:
    void _odidTimeout();
    void _sendMessages();
    void _updateLastGCSPositionInfo(QGeoPositionInfo update);

private:
    void _handleArmStatus(mavlink_message_t& message);

    // Self ID
    void        _sendSelfIDMsg ();
    QByteArray _getSelfIDDescription() const;

    // Operator ID
    void        _sendOperatorID ();

    // System
    void        _sendSystem();
    uint32_t    _timestamp2019();

    // Basic ID
    void        _sendBasicID();

    // GCS position status
    void        _updateGcsPositionStatus(bool usable, const QString& error = QString());

    Vehicle*            _vehicle;
    RemoteIDSettings*   _settings;

    // Flags ODID
    bool    _available = false;
    bool    _armStatusGoodToArm;
    QString _armStatusError;
    bool    _ridDeviceCommsGood;
    bool    _gcsPositionUsable;
    QString _gcsPositionError;
    bool    _vehicleReportsBasicIDMissing;

    bool        _emergencyDeclared;
    QDateTime   _lastGeoPositionTimeStamp;
    int         _targetSystem;
    int         _targetComponent;

    // After emergency cleared, this makes sure the non emergency selfID message makes it to the vehicle
    bool        _enforceSendingSelfID;

    static const uint8_t* _id_or_mac_unknown;

    // Timers
    QTimer _odidTimeoutTimer;
    QTimer _sendMessagesTimer;
};
