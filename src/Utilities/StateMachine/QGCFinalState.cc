#include "QGCFinalState.h"
#include "QGCState.h"
#include "QGCStateMachine.h"

QGCFinalState::QGCFinalState(QState* parent)
    : QFinalState(parent)
{
    connect(this, &QFinalState::entered, this, [this] () {
        qCDebug(QGCStateMachineLog) << "Entered Final State" << qobject_cast<QGCStateMachine*>(this->machine())->machineName();
    });
    connect(this, &QState::exited, this, [this] () {
        qCDebug(QGCStateMachineLog) << "Exited Final State" << qobject_cast<QGCStateMachine*>(this->machine())->machineName();
    });
}
