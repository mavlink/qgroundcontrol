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

    Q_PROPERTY(qreal curRoll                 READ curRoll                 NOTIFY curRollChanged)
    Q_PROPERTY(qreal curPitch                READ curPitch                NOTIFY curPitchChanged)
    Q_PROPERTY(qreal curYaw                  READ curYaw                  NOTIFY curYawChanged)
    Q_PROPERTY(bool  gimbalHaveControl       READ gimbalHaveControl       NOTIFY gimbalHaveControlChanged)
    Q_PROPERTY(bool  gimbalOthersHaveControl READ gimbalOthersHaveControl NOTIFY gimbalOthersHaveControlChanged)
    Q_PROPERTY(uint  deviceId                READ deviceId                NOTIFY deviceIdChanged)

    // do this need to be const?
    qreal curRoll()                 { return _curRoll; } 
    qreal curPitch()                { return _curPitch; }  
    qreal curYaw()                  { return _curYaw; }
    bool  gimbalHaveControl()       { return _haveControl; }
    bool  gimbalOthersHaveControl() { return _othersHaveControl; }
    uint  deviceId()                { return _deviceId; }

    // This is called from c++, but must update QML emiting the signals
    void  setGimbalHaveControl(bool set)        { _haveControl = set;       emit gimbalHaveControlChanged(); }
    void  setGimbalOthersHaveControl(bool set)  { _othersHaveControl = set; emit gimbalOthersHaveControlChanged(); }
    void  setDeviceId(uint id)                  { _deviceId = id;           emit deviceIdChanged(); }

signals:
    void curRollChanged();
    void curPitchChanged();
    void curYawChanged();
    void gimbalHaveControlChanged();
    void gimbalOthersHaveControlChanged();
    void deviceIdChanged();

public:
    unsigned requestInformationRetries = 3;
    unsigned requestStatusRetries = 6;
    unsigned requestAttitudeRetries = 3;
    bool receivedInformation = false;
    bool receivedStatus = false;
    bool receivedAttitude = false;
    bool isComplete = false;
    bool retracted = false;
    bool neutral = false;
    bool yawLock = false;

private:
    float _curRoll = 0.0f;
    float _curPitch = 0.0f;
    float _curYaw = 0.0f;
    bool  _haveControl = false;
    bool  _othersHaveControl = false;
    uint8_t _deviceId = 0;                       // Component ID of gimbal device (or 1-6 for non-MAVLink gimbal)

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
 
    Q_PROPERTY(Gimbal*                       activeGimbal            READ activeGimbal     WRITE setActiveGimbal         NOTIFY activeGimbalChanged)
    Q_PROPERTY(QmlObjectListModel*           gimbals                 READ gimbals          CONSTANT)

    Gimbal*             activeGimbal()    { return _activeGimbal; }
    QmlObjectListModel* gimbals()         { return &_gimbals; }

    void setActiveGimbal(Gimbal* gimbal);

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
    void    activeGimbalChanged           ();
    void    gimbalLabelsChanged           ();
    void    showAcquireGimbalControlPopup (); // This triggers a popup in QML asking the user for aproval to take control

private slots:
    void    _mavlinkMessageReceived (const mavlink_message_t& message);

private:

    void    _requestGimbalInformation        (uint8_t compid);
    void    _handleHeartbeat                 (const mavlink_message_t& message);
    void    _handleGimbalManagerInformation  (const mavlink_message_t& message);
    void    _handleGimbalManagerStatus       (const mavlink_message_t& message);
    void    _handleGimbalDeviceAttitudeStatus(const mavlink_message_t& message);
    void    _checkComplete                   (Gimbal& gimbal, uint8_t compid);
    bool    _tryGetGimbalControl             ();        

    MAVLinkProtocol*    _mavlink            = nullptr;
    Vehicle*            _vehicle            = nullptr;
    Gimbal*             _activeGimbal       = nullptr;


    QMap<uint8_t, GimbalManager> _potentialGimbalManagers;
    QMap<uint8_t, Gimbal> _potentialGimbals;
    QmlObjectListModel _gimbals;
};
