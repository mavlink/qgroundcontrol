/// @file GimbalController.h
/// @brief Class talking to gimbal managers based on the MAVLink gimbal v2 protocol.

#pragma once

#include "QGCApplication.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GimbalLog)

class Vehicle;

class GimbalController : public QObject
{
    Q_OBJECT
public:
    GimbalController(Vehicle* vehicle);

    Q_PROPERTY(QmlObjectListModel*  gimbals                 READ gimbals                                        NOTIFY gimbalsChanged)
    Q_PROPERTY(QStringList          gimbalLabels            READ gimbalLabels                                   NOTIFY gimbalLabelsChanged)

    QmlObjectListModel* gimbals             () { return &_gimbals; }
    QStringList         gimbalLabels        () { return _gimbalLabels; }

signals:
    void    gimbalsChanged          ();
    void    gimbalLabelsChanged    ();

private slots:
    void    _mavlinkMessageReceived (const mavlink_message_t& message);

private:
    class GimbalItem {
    public:
        unsigned requestInformationRetries = 3;
        unsigned requestStatusRetries = 3;
        unsigned requestAttitudeRetries = 3;
        uint8_t responsibleCompid = 0;
        bool receivedInformation = false;
        bool receivedStatus = false;
        bool receivedAttitude = false;
    };

    void    _requestGimbalInformation        (uint8_t compid);
    void    _handleHeartbeat                 (const mavlink_message_t& message);
    void    _handleGimbalManagerInformation  (const mavlink_message_t& message);
    void    _handleGimbalManagerStatus       (const mavlink_message_t& message);
    void    _handleGimbalDeviceAttitudeStatus(const mavlink_message_t& message);

    static void _requestMessageHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);

    Vehicle*            _vehicle            = nullptr;
    QmlObjectListModel  _gimbals;
    QStringList         _gimbalLabels;

    QMap<uint8_t, GimbalItem> _potentialGimbals;
};
