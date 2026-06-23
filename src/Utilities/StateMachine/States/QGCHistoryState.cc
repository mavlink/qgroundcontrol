#include "QGCHistoryState.h"
#include "QGCStateMachine.h"

QGCHistoryState::QGCHistoryState(const QString& stateName, QState* parent, HistoryType historyType)
    : QHistoryState(historyType, parent)
{
    setObjectName(stateName);

    connect(this, &QHistoryState::entered, this, [this]() {
        qCDebug(QGCStateMachineLog) << "Entered history state" << this->stateName();
    });
    connect(this, &QHistoryState::exited, this, [this]() {
        qCDebug(QGCStateMachineLog) << "Exited history state" << this->stateName();
    });
}

QGCStateMachine* QGCHistoryState::machine() const
{
    return qobject_cast<QGCStateMachine*>(QHistoryState::machine());
}

Vehicle* QGCHistoryState::vehicle() const
{
    auto* m = machine();
    return m ? m->vehicle() : nullptr;
}

QString QGCHistoryState::stateName() const
{
    if (machine()) {
        return QStringLiteral("%1:%2").arg(objectName(), machine()->machineName());
    }
    return objectName();
}
