/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QDateTime>
#include <QGeoPositionInfo>

#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(RemoteIDManagerLog)

class RemoteIDSettings;
class QGCPositionManager;

// Supporting Opend Dron ID protocol
class RemoteIDManager : public QObject
{
    Q_OBJECT

public:
    RemoteIDManager(Vehicle* vehicle);

    Q_PROPERTY (bool    armStatusGood       READ armStatusGood      NOTIFY armStatusGoodChanged)
    Q_PROPERTY (QString armStatusError      READ armStatusError     NOTIFY armStatusErrorChanged)
    Q_PROPERTY (bool    commsGood           READ commsGood          NOTIFY commsGoodChanged)
    Q_PROPERTY (bool    gcsGPSGood          READ gcsGPSGood         NOTIFY gcsGPSGoodChanged)
    Q_PROPERTY (bool    basicIDGood         READ basicIDGood        NOTIFY basicIDGoodChanged)
    Q_PROPERTY (bool    emergencyDeclared   READ emergencyDeclared  NOTIFY emergencyDeclaredChanged)
    Q_PROPERTY (bool    operatorIDGood      READ operatorIDGood     NOTIFY operatorIDGoodChanged)


    // Check that the information filled by the pilot operatorID is good
    Q_INVOKABLE void checkOperatorID();

    // Declare emergency
    Q_INVOKABLE void setEmergency(bool declare);

    bool    armStatusGood       (void) const { return _armStatusGood; }
    QString armStatusError      (void) const { return _armStatusError; }
    bool    commsGood           (void) const { return _commsGood; }
    bool    gcsGPSGood          (void) const { return _gcsGPSGood; }
    bool    basicIDGood         (void) const { return _basicIDGood; }
    bool    emergencyDeclared   (void) const { return _emergencyDeclared;}
    bool    operatorIDGood      (void) const { return _operatorIDGood; }

    void mavlinkMessageReceived (mavlink_message_t& message);

    enum LocationTypes {
        TAKEOFF,
        LiveGNSS,
        FIXED
    };

    enum Region {
        FAA,
        EU
    };

signals:
    void armStatusGoodChanged();
    void armStatusErrorChanged();
    void commsGoodChanged();
    void gcsGPSGoodChanged();
    void basicIDGoodChanged();
    void emergencyDeclaredChanged();
    void operatorIDGoodChanged();

private slots:
    void _odidTimeout();
    void _sendMessages();
    void _updateLastGCSPositionInfo(QGeoPositionInfo update);
    void _checkGCSBasicID();

private:
    void _handleArmStatus(mavlink_message_t& message);

    // Self ID 
    void        _sendSelfIDMsg ();
    const char* _getSelfIDDescription();

    // Operator ID
    void        _sendOperatorID ();

    // System
    void        _sendSystem();
    uint32_t    _timestamp2019();

    // Basic ID
    void        _sendBasicID();
    
    MAVLinkProtocol*    _mavlink;
    Vehicle*            _vehicle;
    RemoteIDSettings*   _settings;
    QGCPositionManager* _positionManager;

    // Flags ODID
    bool    _armStatusGood;
    QString _armStatusError;
    bool    _commsGood;
    bool    _gcsGPSGood;
    bool    _basicIDGood;
    bool    _GCSBasicIDValid;
    bool    _operatorIDGood;

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