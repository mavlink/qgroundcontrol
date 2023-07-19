/// @file GimbalController.h
/// @brief Class talking to gimbal managers based on the MAVLink gimbal v2 protocol.

#pragma once

#include <QLoggingCategory>
#include "Vehicle.h"
#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(GimbalLog)

class MavlinkProtocol;

class GimbalController : public QObject
{
    Q_OBJECT
public:
    GimbalController(MAVLinkProtocol* mavlink, Vehicle* vehicle);

    class Gimbal {
    public:
        unsigned requestInformationRetries = 3;
        unsigned requestStatusRetries = 6;
        unsigned requestAttitudeRetries = 3;
        uint8_t responsibleCompid = 0;
        bool receivedInformation = false;
        bool receivedStatus = false;
        bool receivedAttitude = false;
        bool isComplete = false;

        float curRoll = 0.0f;
        float curPitch = 0.0f;
        float curYaw = 0.0f;
        bool retracted = false;
        bool neutral = false;
        bool yawLock = false;

        bool haveControl = false;
        bool othersHaveControl = false;
    };
 
    // TODO: Some sort of selection of gimbal to access the API.

    Q_PROPERTY(QVector<Gimbal*>              gimbals                 READ gimbals                                        NOTIFY gimbalsChanged)

    QVector<Gimbal*>& gimbals() { return _gimbals; }

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
    void    _checkComplete                   (Gimbal& gimbal);

    MAVLinkProtocol*    _mavlink            = nullptr;
    Vehicle*            _vehicle            = nullptr;

    QMap<uint8_t, Gimbal> _potentialGimbals;
    QVector<Gimbal*> _gimbals;
};
