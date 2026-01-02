#pragma once

#include "DelayState.h"
#include "FunctionState.h"
#include "SendMavlinkCommandState.h"
#include "SendMavlinkMessageState.h"
#include "WaitForMavlinkMessageState.h"
#include "ShowAppMessageState.h"
#include "QGCFinalState.h"

#include <QStateMachine>
#include <QFinalState>
#include <QString>

#include <functional>

class Vehicle;

/// QGroundControl specific state machine
class QGCStateMachine : public QStateMachine
{
    Q_OBJECT
public:
    QGCStateMachine(const QString& machineName, Vehicle* vehicle, QObject* parent = nullptr);

    Vehicle *vehicle() const { return _vehicle; }
    QString machineName() const { return objectName(); }

signals:
    void error();

private:
    Vehicle *_vehicle = nullptr;
};
