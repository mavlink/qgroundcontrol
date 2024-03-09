/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QLoggingCategory>
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "MAVLinkProtocol.h"

Q_DECLARE_LOGGING_CATEGORY(GimbalLog)

class MavlinkProtocol;

class Gimbal : public FactGroup
{
    Q_OBJECT

    friend class GimbalController; // so it can set private members of gimbal, it is the only class that will need to modify them

public:
    Gimbal();
    Gimbal(const Gimbal& other);
    const Gimbal& operator=(const Gimbal& other);

    Q_PROPERTY(Fact* absoluteRoll               READ absoluteRoll               CONSTANT)
    Q_PROPERTY(Fact* absolutePitch              READ absolutePitch              CONSTANT)
    Q_PROPERTY(Fact* bodyYaw                    READ bodyYaw                    CONSTANT)
    Q_PROPERTY(Fact* absoluteYaw                READ absoluteYaw                CONSTANT)
    Q_PROPERTY(Fact* deviceId                   READ deviceId                   CONSTANT)
    Q_PROPERTY(bool  yawLock                    READ yawLock                    NOTIFY yawLockChanged)
    Q_PROPERTY(bool  retracted                  READ retracted                  NOTIFY retractedChanged)
    Q_PROPERTY(bool  gimbalHaveControl          READ gimbalHaveControl          NOTIFY gimbalHaveControlChanged)
    Q_PROPERTY(bool  gimbalOthersHaveControl    READ gimbalOthersHaveControl    NOTIFY gimbalOthersHaveControlChanged)

    Fact* absoluteRoll()                  { return &_absoluteRollFact;  }
    Fact* absolutePitch()                 { return &_absolutePitchFact; }
    Fact* bodyYaw()                       { return &_bodyYawFact;       }
    Fact* absoluteYaw()                   { return &_absoluteYawFact;   }
    Fact* deviceId()                      { return &_deviceIdFact;      }
    bool  yawLock() const                 { return _yawLock;            }
    bool  retracted() const               { return _retracted;          }
    bool  gimbalHaveControl() const       { return _haveControl;        }
    bool  gimbalOthersHaveControl() const { return _othersHaveControl;  }

    void  setAbsoluteRoll(float absoluteRoll)   { _absoluteRollFact.setRawValue(absoluteRoll);                     }
    void  setAbsolutePitch(float absolutePitch) { _absolutePitchFact.setRawValue(absolutePitch);                   }
    void  setBodyYaw(float bodyYaw)             { _bodyYawFact.setRawValue(bodyYaw);                               }
    void  setAbsoluteYaw(float absoluteYaw)     { _absoluteYawFact.setRawValue(absoluteYaw);                       }
    void  setDeviceId(uint id)                  { _deviceIdFact.setRawValue(id);                                   }
    void  setYawLock(bool yawLock)              { _yawLock = yawLock;       emit yawLockChanged();                 }
    void  setRetracted(bool retracted)          { _retracted = retracted;   emit retractedChanged();                 }
    void  setGimbalHaveControl(bool set)        { _haveControl = set;       emit gimbalHaveControlChanged();       }
    void  setGimbalOthersHaveControl(bool set)  { _othersHaveControl = set; emit gimbalOthersHaveControlChanged(); }


signals:
    void yawLockChanged();
    void retractedChanged();
    void gimbalHaveControlChanged();
    void gimbalOthersHaveControlChanged();

private:
    void _initFacts(); // To be called EXCLUSIVELY in Gimbal constructors

    // Private members only accesed by friend class GimbalController
    unsigned _requestInformationRetries = 3;
    unsigned _requestStatusRetries = 6;
    unsigned _requestAttitudeRetries = 3;
    bool _receivedInformation = false;
    bool _receivedStatus = false;
    bool _receivedAttitude = false;
    bool _isComplete = false;
    bool _neutral = false;

    // Q_PROPERTIES
    Fact _absoluteRollFact;
    Fact _absolutePitchFact;
    Fact _bodyYawFact;
    Fact _absoluteYawFact;
    Fact _deviceIdFact; // Component ID of gimbal device (or 1-6 for non-MAVLink gimbal)
    bool _yawLock = false;
    bool _retracted = false;
    bool _haveControl = false;
    bool _othersHaveControl = false;

    // Fact names
    static const char* _absoluteRollFactName;
    static const char* _absolutePitchFactName;
    static const char* _bodyYawFactName;
    static const char* _absoluteYawFactName;
    static const char* _deviceIdFactName;
};

class GimbalController : public QObject
{
    Q_OBJECT
public:
    GimbalController(MAVLinkProtocol* mavlink, Vehicle* vehicle);
    ~GimbalController();

    class GimbalManager {
    public:
        unsigned requestGimbalManagerInformationRetries = 6;
        bool receivedInformation = false;
    };

    Q_PROPERTY(Gimbal*              activeGimbal    READ activeGimbal   WRITE setActiveGimbal   NOTIFY activeGimbalChanged)
    Q_PROPERTY(QmlObjectListModel*  gimbals         READ gimbals        CONSTANT)

    Gimbal*             activeGimbal()  { return _activeGimbal; }
    QmlObjectListModel* gimbals()       { return &_gimbals; }

    void setActiveGimbal(Gimbal* gimbal);

    void sendPitchYawFlags                  (uint32_t flags);
    Q_INVOKABLE void gimbalOnScreenControl  (float panpct, float tiltpct, bool clickAndPoint, bool clickAndDrag, bool rateControl, bool retract = false, bool neutral = false, bool yawlock = false);
    Q_INVOKABLE void sendPitchBodyYaw       (float pitch, float yaw, bool showError = true);
    Q_INVOKABLE void sendPitchAbsoluteYaw   (float pitch, float yaw, bool showError = true);
    Q_INVOKABLE void toggleGimbalRetracted  (bool set = false);
    Q_INVOKABLE void toggleGimbalYawLock    (bool set = false);
    Q_INVOKABLE void acquireGimbalControl   ();
    Q_INVOKABLE void releaseGimbalControl   ();

public slots:
    // These slots are conected with joysticks for button control
    void gimbalYawLock              (bool yawLock) { toggleGimbalYawLock(yawLock); }
    Q_INVOKABLE void centerGimbal   (); // Also used by qml
    void gimbalPitchStep            (int direction);
    void gimbalYawStep              (int direction);

signals:
    void    activeGimbalChanged           ();
    void    showAcquireGimbalControlPopup (); // This triggers a popup in QML asking the user for aproval to take control

private slots:
    void    _mavlinkMessageReceived(const mavlink_message_t& message);

private:
    void    _requestGimbalInformation           (uint8_t compid);
    void    _handleHeartbeat                    (const mavlink_message_t& message);
    void    _handleGimbalManagerInformation     (const mavlink_message_t& message);
    void    _handleGimbalManagerStatus          (const mavlink_message_t& message);
    void    _handleGimbalDeviceAttitudeStatus   (const mavlink_message_t& message);
    void    _checkComplete                      (Gimbal& gimbal, uint8_t compid);
    bool    _tryGetGimbalControl                ();
    bool    _yawInVehicleFrame                  (uint32_t flags);
    
    MAVLinkProtocol*    _mavlink            = nullptr;
    Vehicle*            _vehicle            = nullptr;
    Gimbal*             _activeGimbal       = nullptr;

    QMap<uint8_t, GimbalManager> _potentialGimbalManagers;
    QMap<uint8_t, Gimbal> _potentialGimbals;
    QmlObjectListModel _gimbals;
    
    static const char* _gimbalFactGroupNamePrefix;
};
