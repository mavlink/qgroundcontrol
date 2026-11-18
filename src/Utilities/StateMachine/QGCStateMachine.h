#pragma once

#include "AsyncFunctionState.h"
#include "DelayState.h"
#include "FunctionState.h"
#include "QGCAbstractTransition.h"
#include "QGCSignalTransition.h"
#include "GuardedTransition.h"
#include "MachineEventTransition.h"
#include "RetryTransition.h"
#include "QGCAbstractState.h"
#include "QGCFinalState.h"
#include "QGCStateMachineEvent.h"
#include "SendMavlinkCommandState.h"
#include "SendMavlinkMessageState.h"
#include "SkippableAsyncState.h"
#include "RetryableRequestMessageState.h"
#include "WaitForMavlinkMessageState.h"
#include "WaitForParamResponseState.h"
#include "WaitStateBase.h"

#include <QtStateMachine/QStateMachine>
#include <QtCore/QAbstractAnimation>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <functional>

class Vehicle;

/// \brief QGroundControl specific state machine with enhanced error handling
///
class QGCStateMachine : public QStateMachine
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCStateMachine)

    /// Name of the currently active state (for QML binding)
    Q_PROPERTY(QString currentStateName READ currentStateName NOTIFY currentStateNameChanged)

    /// Current progress from 0.0 to 1.0 (for QML binding)
    Q_PROPERTY(float progress READ progress NOTIFY progressUpdate)

    /// Whether the machine is in progress (for QML binding)
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY inProgressChanged)

    /// List of recent state names for history display (for QML binding)
    Q_PROPERTY(QStringList stateHistory READ stateHistory NOTIFY stateHistoryChanged)

public:
    QGCStateMachine(const QString& machineName, Vehicle* vehicle, QObject* parent = nullptr);

    Vehicle *vehicle() const { return _vehicle; }
    QString machineName() const { return objectName(); }

    /// @return true from start() until the machine emits stopped or finished
    /// Unlike QStateMachine::isRunning(), this is already true before Qt processes the queued start.
    bool inProgress() const { return _inProgress; }

    /// Use inProgress() instead: isRunning() is false until Qt processes the queued start
    bool isRunning() const = delete;

    /// Use start() and stop() instead: setRunning() calls QStateMachine::start() and bypasses start()
    void setRunning(bool running) = delete;

    /// Get the name of the currently active state
    /// @return State name or empty string if not running
    QString currentStateName() const;

    /// Get a list of recent state names (limited history for QML display)
    /// @return List of state names, most recent last
    QStringList stateHistory() const { return _stateHistory; }

    /// Set the maximum number of state history entries to keep
    void setStateHistoryLimit(int limit) { _stateHistoryLimit = limit; }

    // -------------------------------------------------------------------------
    // Event Handling
    // -------------------------------------------------------------------------

    /// Set a global error state that all QGCState errors will transition to
    /// @param errorState The state to transition to on error
    void setGlobalErrorState(QAbstractState* errorState);

    QAbstractState* globalErrorState() const { return _globalErrorState; }

    /// Register a QGCState with the machine and automatically wire up error transitions
    /// If a global error state is set, the state's error() signal will transition to it
    /// @param state The state to register
    void registerState(QGCState* state);

    /// Register a QGCAbstractState with the machine and automatically wire up error transitions
    /// @param state The state to register
    void registerState(QGCAbstractState* state);

    /// Create and register a FunctionState
    /// @param function Function to execute
    FunctionState* addFunctionState(const QString& stateName, std::function<void()> function);

    /// Create and register an AsyncFunctionState
    /// @param setupFunction Function to execute on entry
    /// @param timeoutMsecs Optional timeout
    AsyncFunctionState* addAsyncFunctionState(const QString& stateName,
                                               AsyncFunctionState::SetupFunction setupFunction,
                                               int timeoutMsecs = 0);

    /// Create and register a DelayState
    /// @param delayMsecs Delay duration
    DelayState* addDelayState(int delayMsecs);

    /// Post a named event to be processed immediately
    /// @param eventName Name of the event
    /// @param data Optional payload data
    /// @param priority Event priority (HighPriority events are processed before NormalPriority)
    void postEvent(const QString& eventName, const QVariant& data = QVariant(),
                   EventPriority priority = NormalPriority);

    /// Post a named event with a delay
    /// @param eventName Name of the event
    /// @param delayMsecs Delay in milliseconds
    /// @param data Optional payload data
    /// @return Event ID that can be used to cancel the event
    int postDelayedEvent(const QString& eventName, int delayMsecs, const QVariant& data = QVariant());

    /// Cancel a previously posted delayed event
    /// @param eventId The ID returned by postDelayedEvent
    /// @return true if the event was cancelled, false if already delivered or not found
    bool cancelDelayedEvent(int eventId);

    // -------------------------------------------------------------------------
    // State Configuration
    // -------------------------------------------------------------------------

    /// Set the initial state and optionally start the machine
    /// @param state The initial state
    /// @param autoStart If true, starts the machine immediately
    void setInitialState(QAbstractState* state, bool autoStart = false);

    /// Add a final state that will stop the machine when entered
    /// @return The created final state
    QGCFinalState* addFinalState(const QString& stateName = QString());

    // -------------------------------------------------------------------------
    // Lifecycle Helpers
    // -------------------------------------------------------------------------

    /// Restart the machine (stop then start)
    void restart();

    // -------------------------------------------------------------------------
    // Transition Helpers
    // -------------------------------------------------------------------------

    /// Create a simple signal transition between states
    template<typename Func>
    QSignalTransition* addTransition(QState* from, Func signal, QAbstractState* to,
                                      QAbstractAnimation* animation = nullptr)
    {
        auto* transition = from->addTransition(from, signal, to);
        if (animation) {
            transition->addAnimation(animation);
        }
        return transition;
    }

    /// Create a guarded transition between states
    /// @param guard Predicate that must return true for transition
    template<typename Func>
    GuardedTransition* addGuardedTransition(QState* from, Func signal, QAbstractState* to,
                                            GuardedTransition::Guard guard,
                                            QAbstractAnimation* animation = nullptr)
    {
        auto* transition = new GuardedTransition(from, signal, to, std::move(guard));
        from->addTransition(transition);
        if (animation) {
            transition->addAnimation(animation);
        }
        return transition;
    }

    /// Create a machine event transition
    /// @param eventName Event name to match
    MachineEventTransition* addEventTransition(QState* from, const QString& eventName, QAbstractState* to,
                                               QAbstractAnimation* animation = nullptr);

    /// Create a retry transition that retries an action before advancing
    /// @param from Source state (must match signal's object type)
    /// @param signal Signal that triggers retry evaluation
    /// @param to Target state (used after max retries)
    /// @param retryAction Action to perform on retry
    /// @param maxRetries Maximum number of retries before advancing
    template<typename SenderType, typename Func>
    RetryTransition* addRetryTransition(SenderType* from, Func signal,
                                        QAbstractState* to,
                                        std::function<void()> retryAction,
                                        int maxRetries = 1)
    {
        auto* transition = new RetryTransition(from, signal, to, std::move(retryAction), maxRetries);
        from->addTransition(transition);
        return transition;
    }

    // -------------------------------------------------------------------------
    // Progress Tracking
    // -------------------------------------------------------------------------

    /// Set up weighted progress tracking for a sequence of states.
    /// Each state is assigned a weight representing its relative contribution to overall progress.
    /// States with higher weights contribute more to the progress bar.
    ///
    /// Example:
    /// @code
    /// machine.setProgressWeights({
    ///     {state1, 1},   // 1/10 of progress
    ///     {state2, 5},   // 5/10 of progress (e.g., slow parameter load)
    ///     {state3, 2},   // 2/10 of progress
    ///     {state4, 2}    // 2/10 of progress
    /// });
    /// @endcode
    ///
    /// @param stateWeights List of state/weight pairs in order of execution
    void setProgressWeights(const QList<QPair<QAbstractState*, int>>& stateWeights);

    /// Update sub-progress within the current state (0.0 to 1.0).
    /// Call this from state callbacks to show progress within a long-running state.
    /// @param subProgress Progress within current state (0.0 = just started, 1.0 = complete)
    void setSubProgress(float subProgress);

    /// Get the current overall progress (0.0 to 1.0)
    float progress() const;

    /// Reset progress tracking (call when restarting the machine)
    void resetProgress();

public slots:
    /// Start the state machine with debug logging
    void start();

signals:
    /// Emitted when progress changes (if progress tracking is enabled)
    /// @param progress Overall progress from 0.0 to 1.0
    void progressUpdate(float progress);

    /// Emitted when the current state changes (for QML binding)
    void currentStateNameChanged();

    /// Emitted when inProgress() changes (for QML binding)
    void inProgressChanged();

    /// Emitted when the state history changes (for QML binding)
    void stateHistoryChanged();

private:
    void _onStateEntered();
    void _setInProgress(bool inProgress);
    float _calculateProgress() const;

    Vehicle* _vehicle = nullptr;
    QAbstractState* _globalErrorState = nullptr;
    bool _inProgress = false;

    // Progress tracking
    QList<QAbstractState*> _progressStates;
    QList<int> _progressWeights;
    int _progressTotalWeight = 0;
    int _progressCurrentIndex = -1;
    float _progressSubProgress = 0.0f;
    float _progressLastEmitted = 0.0f;

    // QML state history
    QStringList _stateHistory;
    int _stateHistoryLimit = 20;
};
