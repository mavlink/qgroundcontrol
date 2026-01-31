#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "Vehicle.h"
#include "MAVLinkLib.h"

class AutotuneStateMachine;

class Autotune : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    friend class AutotuneStateMachine;

public:
    explicit Autotune(Vehicle *vehicle);

    Q_PROPERTY(bool      autotuneInProgress   READ autotuneInProgress     NOTIFY autotuneChanged)
    Q_PROPERTY(float     autotuneProgress     READ autotuneProgress       NOTIFY autotuneChanged)
    Q_PROPERTY(QString   autotuneStatus       READ autotuneStatus         NOTIFY autotuneChanged)

    Q_INVOKABLE void autotuneRequest();

    static void ackHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);
    static void progressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack);

    bool autotuneInProgress();
    float autotuneProgress();
    QString autotuneStatus();

signals:
    void autotuneChanged();

private:
    Vehicle* _vehicle = nullptr;
    AutotuneStateMachine* _stateMachine = nullptr;
};
