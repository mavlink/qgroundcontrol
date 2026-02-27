#include "QGCFinalState.h"
#include "QGCState.h"
#include "QGCStateMachine.h"
#include "Vehicle.h"

QGCFinalState::QGCFinalState(const QString& stateName, QState* parent)
    : QFinalState(parent)
{
    setObjectName(stateName);

    connect(this, &QFinalState::entered, this, [this]() {
        qCDebug(QGCStateMachineLog) << "Entered" << this->stateName();
    });
    connect(this, &QState::exited, this, [this]() {
        qCDebug(QGCStateMachineLog) << "Exited" << this->stateName();
    });
}

QGCFinalState::QGCFinalState(QState* parent)
    : QGCFinalState(QStringLiteral("FinalState"), parent)
{
}

QString QGCFinalState::stateName() const
{
    auto* m = machine();
    if (m) {
        return QStringLiteral("%1:%2").arg(objectName(), m->machineName());
    }
    return objectName();
}

QGCStateMachine* QGCFinalState::machine() const
{
    return qobject_cast<QGCStateMachine*>(QFinalState::machine());
}

Vehicle* QGCFinalState::vehicle() const
{
    auto* m = machine();
    return m ? m->vehicle() : nullptr;
}
