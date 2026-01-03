#include "QGCStateMachine.h"
#include "QGCApplication.h"
#include "AudioOutput.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "AudioOutput.h"

#include <QFinalState>

QGCStateMachine::QGCStateMachine(const QString& machineName, Vehicle *vehicle, QObject* parent)
    : QStateMachine (parent)
    , _vehicle      (vehicle)
{
    setObjectName(machineName);

    connect(this, &QGCStateMachine::started, this, [this] () {
        qCDebug(QGCStateMachineLog) << "State machine started:" << objectName();
    });
    connect(this, &QGCStateMachine::stopped, this, [this] () {
        qCDebug(QGCStateMachineLog) << "State machine finished:" << objectName();
    });

    connect(this, &QGCStateMachine::stopped, this, [this] () { this->deleteLater(); });
}
