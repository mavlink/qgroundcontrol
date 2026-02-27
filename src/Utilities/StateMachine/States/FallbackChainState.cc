#include "FallbackChainState.h"
#include "QGCLoggingCategory.h"

FallbackChainState::FallbackChainState(const QString& stateName, QState* parent)
    : QGCState(stateName, parent)
{
}

void FallbackChainState::addStrategy(const QString& name, Strategy action)
{
    _strategies.append({name, std::move(action)});
}

void FallbackChainState::onEnter()
{
    _currentIndex = -1;
    _successfulStrategy.clear();

    // Start trying strategies
    QMetaObject::invokeMethod(this, &FallbackChainState::_tryNextStrategy, Qt::QueuedConnection);
}

void FallbackChainState::_tryNextStrategy()
{
    _currentIndex++;

    if (_currentIndex >= _strategies.size()) {
        // All strategies failed
        qCDebug(QGCStateMachineLog) << stateName() << "all strategies exhausted";
        emit error();
        return;
    }

    const StrategyEntry& entry = _strategies[_currentIndex];

    qCDebug(QGCStateMachineLog) << stateName() << "trying strategy" << entry.name
                                 << "(" << (_currentIndex + 1) << "of" << _strategies.size() << ")";
    emit tryingStrategy(entry.name, _currentIndex, _strategies.size());

    bool success = false;
    if (entry.action) {
        success = entry.action();
    }

    if (success) {
        qCDebug(QGCStateMachineLog) << stateName() << "strategy succeeded:" << entry.name;
        _successfulStrategy = entry.name;
        emit strategySucceeded(entry.name);
        emit advance();
    } else {
        qCDebug(QGCStateMachineLog) << stateName() << "strategy failed:" << entry.name;
        emit strategyFailed(entry.name);
        // Try next strategy (deferred to allow signal processing)
        QMetaObject::invokeMethod(this, &FallbackChainState::_tryNextStrategy, Qt::QueuedConnection);
    }
}
