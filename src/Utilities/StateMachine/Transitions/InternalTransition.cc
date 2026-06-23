#include "InternalTransition.h"
#include "QGCState.h"

void InternalTransition::onTransition(QEvent* event)
{
    Q_UNUSED(event);

    if (_action) {
        _action();
    }

    qCDebug(QGCStateMachineLog) << "Internal transition executed";
}
