#include "QGCFinalState.h"
#include "QGCState.h"
#include "QGCStateMachine.h"

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
    return qualifiedStateName(this);
}

QGCStateMachine* QGCFinalState::machine() const
{
    return qgcStateMachineOf(this);
}

Vehicle* QGCFinalState::vehicle() const
{
    auto* m = machine();
    return m ? m->vehicle() : nullptr;
}
