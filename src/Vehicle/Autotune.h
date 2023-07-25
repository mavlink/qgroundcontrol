/****************************************************************************
 *
 * (c) 2009-2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QTimer>
#include "Vehicle.h"
#include "MAVLinkProtocol.h"

class Autotune : public QObject
{
    Q_OBJECT


public:
    explicit Autotune(Vehicle *vehicle);

    Q_PROPERTY(bool      autotuneEnabled      READ autotuneEnabled        NOTIFY autotuneChanged)
    Q_PROPERTY(bool      autotuneInProgress   READ autotuneInProgress     NOTIFY autotuneChanged)
    Q_PROPERTY(float     autotuneProgress     READ autotuneProgress       NOTIFY autotuneChanged)
    Q_PROPERTY(QString   autotuneStatus       READ autotuneStatus         NOTIFY autotuneChanged)

    Q_INVOKABLE void autotuneRequest ();

    static void ackHandler(void* resultHandlerData, int compId, MAV_RESULT commandResult, uint8_t progress, Vehicle::MavCmdResultFailureCode_t failureCode);

    bool      autotuneEnabled    ();
    bool      autotuneInProgress () { return _autotuneInProgress; }
    float     autotuneProgress   () { return _autotuneProgress; }
    QString   autotuneStatus     () { return _autotuneStatus; }


public slots:
    void handleEnabled ();
    void sendMavlinkRequest();


private:
    void handleAckStatus(uint8_t ackProgress);
    void handleAckFailure();
    void handleAckError(uint8_t ackError);
    void startTimers();
    void stopTimers();


private slots:


private:
    Vehicle* _vehicle                {nullptr};
    bool     _autotuneInProgress     {false};
    float    _autotuneProgress       {0.0};
    QString  _autotuneStatus         {tr("Autotune: Not performed")};
    bool     _disarmMessageDisplayed {false};

    QTimer   _pollTimer;         // the frequency at which the polling should be performed


signals:
    void autotuneChanged ();
};
