#pragma once

#include "QGCState.h"

#include <QtCore/QTimer>

#include <functional>

/// A state that retries an action and chooses behavior when retries are exhausted.
///
/// The action is executed on entry. If it fails, the state retries after a delay.
/// Once retries are exhausted, behavior is controlled by ExhaustedBehavior.
class RetryState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(RetryState)

public:
    using Action = std::function<bool()>;

    enum ExhaustedBehavior {
        EmitError,      ///< Emit error() after retries exhausted.
        EmitAdvance     ///< Emit advance() after retries exhausted (skip/continue).
    };
    Q_ENUM(ExhaustedBehavior)

    RetryState(const QString& stateName, QState* parent,
               Action action,
               int maxRetries = 0,
               int retryDelayMsecs = 1000,
               ExhaustedBehavior exhaustedBehavior = EmitError);

    int currentAttempt() const { return _currentAttempt; }
    int maxRetries() const { return _maxRetries; }
    int totalAttempts() const { return _maxRetries + 1; }
    ExhaustedBehavior exhaustedBehavior() const { return _exhaustedBehavior; }
    bool wasSkipped() const { return _wasSkipped; }

signals:
    /// Emitted before each retry attempt.
    void retrying(int attempt, int maxAttempts);

    /// Emitted when the action eventually succeeds.
    void succeeded();

    /// Emitted when all retries are exhausted (both behaviors).
    void exhausted();

    /// Emitted when retries are exhausted and behavior is EmitAdvance.
    void skipped();

protected:
    void onEnter() override;

private slots:
    void _executeAction();

private:
    Action _action;
    int _maxRetries = 0;
    int _retryDelayMsecs = 1000;
    ExhaustedBehavior _exhaustedBehavior = EmitError;

    int _currentAttempt = 0;
    bool _wasSkipped = false;
    QTimer _retryTimer;
};
