#pragma once

#include "QGCState.h"

#include <functional>

/// A state that tries multiple strategies in order until one succeeds.
///
/// Strategies are executed in the order they were added. If a strategy
/// succeeds, the state emits advance(). If all strategies fail, error() is emitted.
///
/// Example usage:
/// @code
/// auto* state = new FallbackChainState("Connect", &machine);
/// state->addStrategy("Primary", []() { return connectPrimary(); });
/// state->addStrategy("Backup", []() { return connectBackup(); });
/// state->addStrategy("Offline", []() { return goOffline(); });
///
/// state->addTransition(state, &FallbackChainState::advance, connectedState);
/// state->addTransition(state, &FallbackChainState::error, failedState);
/// @endcode
class FallbackChainState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(FallbackChainState)

public:
    /// Strategy action - returns true on success
    using Strategy = std::function<bool()>;

    struct StrategyEntry {
        QString name;
        Strategy action;
    };

    FallbackChainState(const QString& stateName, QState* parent);

    /// Add a strategy to the chain
    /// @param name Strategy name for logging
    /// @param action Strategy action (returns true on success)
    void addStrategy(const QString& name, Strategy action);

    /// Get the name of the strategy that succeeded
    QString successfulStrategy() const { return _successfulStrategy; }

    /// Get the current strategy index being tried
    int currentStrategyIndex() const { return _currentIndex; }

    /// Get the total number of strategies
    int strategyCount() const { return _strategies.size(); }

signals:
    /// Emitted when trying a strategy
    void tryingStrategy(const QString& name, int index, int total);

    /// Emitted when a strategy fails and moving to next
    void strategyFailed(const QString& name);

    /// Emitted when a strategy succeeds
    void strategySucceeded(const QString& name);

protected:
    void onEnter() override;

private slots:
    void _tryNextStrategy();

private:
    QList<StrategyEntry> _strategies;
    int _currentIndex = -1;
    QString _successfulStrategy;
};
