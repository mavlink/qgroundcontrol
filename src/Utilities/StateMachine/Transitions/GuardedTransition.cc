#include "GuardedTransition.h"
#include "QGCState.h"

bool GuardedTransition::eventTest(QEvent* event)
{
    if (!QGCSignalTransition::eventTest(event)) {
        return false;
    }

    if (_guard && !_guard()) {
        qCDebug(QGCStateMachineLog) << "Guarded transition blocked by guard predicate";
        return false;
    }

    return true;
}
