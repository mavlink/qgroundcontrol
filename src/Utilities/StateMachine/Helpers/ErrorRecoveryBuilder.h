#pragma once

#include "QGCState.h"

#include <QtCore/QTimer>

#include <functional>

class QGCStateMachine;

/// Fluent builder for creating error recovery states.
///
/// Combines multiple error recovery patterns into a single state
/// using a declarative fluent API.
///
/// Example usage:
/// @code
/// auto* state = ErrorRecoveryBuilder(&machine, "CriticalOp")
///     .withAction([]() { return doWork(); })
///     .retry(3, 1000)
///     .withFallback([]() { return fallbackWork(); })
///     .withRollback([]() { cleanup(); })
///     .onExhausted(ErrorRecoveryBuilder::LogAndError)
///     .build();
/// @endcode
class ErrorRecoveryBuilder
{
public:
    using Action = std::function<bool()>;
    using VoidAction = std::function<void()>;

    /// What to do when all recovery options are exhausted
    enum ExhaustedBehavior {
        EmitError,      ///< Emit error() signal (default)
        EmitAdvance,    ///< Continue anyway (skip)
        LogAndError,    ///< Log warning and emit error()
        LogAndAdvance   ///< Log warning and continue
    };

    ErrorRecoveryBuilder(QGCStateMachine* machine, const QString& stateName);

    /// Set the primary action to execute
    ErrorRecoveryBuilder& withAction(Action action);

    /// Configure retry behavior
    /// @param maxRetries Maximum number of retry attempts
    /// @param delayMsecs Delay between retries
    ErrorRecoveryBuilder& retry(int maxRetries, int delayMsecs = 1000);

    /// Add a fallback action to try if primary fails
    ErrorRecoveryBuilder& withFallback(Action fallback);

    /// Add a rollback action to execute on failure
    ErrorRecoveryBuilder& withRollback(VoidAction rollback);

    /// Configure what happens when all options are exhausted
    ErrorRecoveryBuilder& onExhausted(ExhaustedBehavior behavior);

    /// Set a timeout for the entire operation
    ErrorRecoveryBuilder& withTimeout(int timeoutMsecs);

    /// Build and return the configured state
    QGCState* build();

private:
    QGCStateMachine* _machine;
    QString _stateName;
    Action _action;
    Action _fallback;
    VoidAction _rollback;
    int _maxRetries = 0;
    int _retryDelayMsecs = 1000;
    int _timeoutMsecs = 0;
    ExhaustedBehavior _exhaustedBehavior = EmitError;
};

/// The state created by ErrorRecoveryBuilder
class ErrorRecoveryState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(ErrorRecoveryState)

public:
    using Action = std::function<bool()>;
    using VoidAction = std::function<void()>;

    ErrorRecoveryState(const QString& stateName, QState* parent);

    void setAction(Action action) { _action = std::move(action); }
    void setFallback(Action fallback) { _fallback = std::move(fallback); }
    void setRollback(VoidAction rollback) { _rollback = std::move(rollback); }
    void setRetry(int maxRetries, int delayMsecs);
    void setTimeout(int timeoutMsecs);
    void setExhaustedBehavior(ErrorRecoveryBuilder::ExhaustedBehavior behavior);

    /// Get the phase that succeeded (if any)
    QString successPhase() const { return _successPhase; }

signals:
    void retrying(int attempt, int maxAttempts);
    void tryingFallback();
    void rollingBack();
    void succeeded();
    void exhausted();

protected:
    void onEnter() override;

private slots:
    void _executeAction();
    void _onTimeout();

private:
    void _handleFailure();
    void _handleExhausted();

    Action _action;
    Action _fallback;
    VoidAction _rollback;
    int _maxRetries = 0;
    int _retryDelayMsecs = 1000;
    int _timeoutMsecs = 0;
    ErrorRecoveryBuilder::ExhaustedBehavior _exhaustedBehavior = ErrorRecoveryBuilder::EmitError;

    int _currentAttempt = 0;
    bool _triedFallback = false;
    QString _successPhase;
    QTimer _retryTimer;
    QTimer _timeoutTimer;
};
