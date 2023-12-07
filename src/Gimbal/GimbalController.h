/// @file GimbalController.h
/// @brief Class talking to gimbal managers based on the MAVLink gimbal v2 protocol.

#pragma once

#include <QLoggingCategory>
#include "Vehicle.h"
#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(GimbalLog)

class MavlinkProtocol;

class Gimbal : public QObject
{
    Q_OBJECT
public:
    Gimbal();
    Gimbal(const Gimbal& other);
    const Gimbal& operator=(const Gimbal& other);

    Q_PROPERTY(qreal curRoll         READ curRoll           NOTIFY curRollChanged)
    Q_PROPERTY(qreal curPitch        READ curPitch          NOTIFY curPitchChanged)
    Q_PROPERTY(qreal curYaw          READ curYaw            NOTIFY curYawChanged)

    qreal curRoll()  { return _curRoll; } 
    qreal curPitch() { return _curPitch; }  
    qreal curYaw()   { return _curYaw; }

signals:
    void curRollChanged();
    void curPitchChanged();
    void curYawChanged();

public:
    unsigned requestInformationRetries = 3;
    unsigned requestStatusRetries = 6;
    unsigned requestAttitudeRetries = 3;
    uint8_t deviceId = 0;                       // Component ID of gimbal device (or 1-6 for non-MAVLink gimbal)
    bool receivedInformation = false;
    bool receivedStatus = false;
    bool receivedAttitude = false;
    bool isComplete = false;
    bool retracted = false;
    bool neutral = false;
    bool yawLock = false;
    bool haveControl = false;
    bool othersHaveControl = false;

private:
    float _curRoll = 0.0f;
    float _curPitch = 0.0f;
    float _curYaw = 0.0f;

    friend class GimbalController;
};

class GimbalController : public QObject
{
    Q_OBJECT
public:
    GimbalController(MAVLinkProtocol* mavlink, Vehicle* vehicle);

    class GimbalManager {
    public:
        unsigned requestGimbalManagerInformationRetries = 3;
        bool receivedInformation = false;
    };
 
    // TODO: Some sort of selection of gimbal to access the API.

    Q_PROPERTY(QVector<Gimbal*>              gimbals                 READ gimbals                                        NOTIFY gimbalsChanged)

    QVector<Gimbal*>& gimbals() { return _gimbals; }

    void sendGimbalManagerPitchYawFlags         (uint32_t flags);
    Q_INVOKABLE void gimbalControlValue         (double pitch, double yaw);
    Q_INVOKABLE void gimbalPitchStep            (int direction);
    Q_INVOKABLE void gimbalYawStep              (int direction);
    Q_INVOKABLE void centerGimbal               ();
    Q_INVOKABLE void gimbalOnScreenControl      (float panpct, float tiltpct, bool clickAndPoint, bool clickAndDrag, bool rateControl, bool retract = false, bool neutral = false, bool yawlock = false);
    Q_INVOKABLE void sendGimbalManagerPitchYaw  (float pan, float tilt);
    Q_INVOKABLE void toggleGimbalRetracted      (bool force = false, bool set = false);
    Q_INVOKABLE void toggleGimbalNeutral        (bool force = false, bool set = false);
    Q_INVOKABLE void toggleGimbalYawLock        (bool force = false, bool set = false);
    Q_INVOKABLE void acquireGimbalControl       ();
    Q_INVOKABLE void releaseGimbalControl       ();
    Q_INVOKABLE void setGimbalRcTargeting       ();
    Q_INVOKABLE void setGimbalHomeTargeting     ();

signals:
    void    gimbalsChanged          ();
    void    gimbalLabelsChanged    ();

private slots:
    void    _mavlinkMessageReceived (const mavlink_message_t& message);

private:

    void    _requestGimbalInformation        (uint8_t compid);
    void    _handleHeartbeat                 (const mavlink_message_t& message);
    void    _handleGimbalManagerInformation  (const mavlink_message_t& message);
    void    _handleGimbalManagerStatus       (const mavlink_message_t& message);
    void    _handleGimbalDeviceAttitudeStatus(const mavlink_message_t& message);
    void    _checkComplete                   (Gimbal& gimbal, uint8_t compid);

    MAVLinkProtocol*    _mavlink            = nullptr;
    Vehicle*            _vehicle            = nullptr;


    QMap<uint8_t, GimbalManager> _potentialGimbalManagers;
    QMap<uint8_t, Gimbal> _potentialGimbals;
    QVector<Gimbal*> _gimbals;
};
