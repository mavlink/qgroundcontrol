#include "ConditionalState.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMetaObject>

ConditionalState::ConditionalState(const QString& stateName, QState* parent, Predicate predicate, Action action)
    : QGCState(stateName, parent)
    , _predicate(std::move(predicate))
    , _action(std::move(action))
{
    connect(this, &QState::entered, this, &ConditionalState::_onEntered);
}

void ConditionalState::_onEntered()
{
    if (_predicate && _predicate()) {
        qCDebug(QGCStateMachineLog) << "Condition met, executing" << stateName();
        if (_action) {
            _action();
        }
        // Defer signal emission to avoid firing during state entry
        QMetaObject::invokeMethod(this, &ConditionalState::advance, Qt::QueuedConnection);
    } else {
        qCDebug(QGCStateMachineLog) << "Condition not met, skipping" << stateName();
        // Defer signal emission to avoid firing during state entry
        QMetaObject::invokeMethod(this, &ConditionalState::skipped, Qt::QueuedConnection);
    }
}
