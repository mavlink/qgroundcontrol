/// @file Gimbal.h
/// @brief Class talking to gimbal managers based on the MAVLink gimbal v2 protocol.

#pragma once

#include "QGCApplication.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GimbalLog)

class Vehicle;

class Gimbal : public QObject
{
    Q_OBJECT
public:
    Gimbal(Vehicle* vehicle);

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
        GimbalItem(uint8_t compID_);
        uint8_t compID       = 0;
        bool    shouldRetry  = false;
    };

    void    _requestGimbalInformation(GimbalItem& item);
    void    _handleHeartbeat         (const mavlink_message_t& message);
    void    _handleGimbalInformation (const mavlink_message_t& message);

    static void _requestMessageHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);

    Vehicle*            _vehicle            = nullptr;
    QmlObjectListModel  _gimbals;
    QStringList         _gimbalLabels;

    QVector<GimbalItem> _potentialGimbals;
};
