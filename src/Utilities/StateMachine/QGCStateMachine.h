#pragma once

#include "AsyncFunctionState.h"
#include "CircuitBreakerState.h"
#include "ConditionalState.h"
#include "DelayState.h"
#include "EventQueuedState.h"
#include "FallbackChainState.h"
#include "FunctionState.h"
#include "LoopState.h"
#include "QGCAbstractTransition.h"
#include "QGCSignalTransition.h"
#include "GuardedTransition.h"
#include "InternalTransition.h"
#include "MachineEventTransition.h"
#include "NamedEventTransition.h"
#include "RetryTransition.h"
#include "TimeoutTransition.h"
#include "QGCAbstractState.h"
#include "QGCEventTransition.h"
#include "SignalDataTransition.h"
#include "QGCFinalState.h"
#include "QGCHistoryState.h"
#include "ParallelState.h"
#include "QGCStateMachineEvent.h"
#include "RequestMessageState.h"
#include "RetryThenFailState.h"
#include "RetryThenSkipState.h"
#include "RollbackState.h"
#include "SendMavlinkCommandState.h"
#include "SendMavlinkMessageState.h"
#include "SequenceState.h"
#include "ShowAppMessageState.h"
#include "SkippableAsyncState.h"
#include "SubMachineState.h"
#include "ProgressState.h"
#include "RetryableRequestMessageState.h"
#include "WaitForMavlinkMessageState.h"
#include "WaitForSignalState.h"
#include "WaitStateBase.h"
#include "StateContext.h"
#include "ErrorRecoveryBuilder.h"
#include "ErrorHandlers.h"

#include <QtStateMachine/QStateMachine>
#include <QtStateMachine/QFinalState>
#include <QtStateMachine/QHistoryState>
#include <QtCore/QAbstractAnimation>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <functional>

class Vehicle;

/// QGroundControl specific state machine with enhanced error handling
class QGCStateMachine : public QStateMachine
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCStateMachine)

    /// Name of the currently active state (for QML binding)
    Q_PROPERTY(QString currentStateName READ currentStateName NOTIFY currentStateNameChanged)

    /// Current progress from 0.0 to 1.0 (for QML binding)
    Q_PROPERTY(float progress READ progress NOTIFY progressUpdate)

    /// Whether the machine is currently running (for QML binding)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

    /// List of recent state names for history display (for QML binding)
    Q_PROPERTY(QStringList stateHistory READ stateHistory NOTIFY stateHistoryChanged)

public:
    using EntryCallback = std::function<void()>;
    using ExitCallback = std::function<void()>;
    using EventHandler = std::function<bool(QEvent*)>;

    QGCStateMachine(const QString& machineName, Vehicle* vehicle, QObject* parent = nullptr);

    Vehicle *vehicle() const { return _vehicle; }
    QString machineName() const { return objectName(); }

    /// Get the state machine's context for inter-state data passing
    /// @return Reference to the context object
    StateContext& context() { return _context; }
    const StateContext& context() const { return _context; }

    /// @return true if the state machine is currently running
    /// Compatibility method matching old StateMachine::active()
    bool active() const { return isRunning(); }

    /// Get the name of the currently active state
    /// @return State name or empty string if not running
    QString currentStateName() const;

    /// Get a list of recent state names (limited history for QML display)
    /// @return List of state names, most recent last
    QStringList stateHistory() const { return _stateHistory; }

    /// Set the maximum number of state history entries to keep
    void setStateHistoryLimit(int limit) { _stateHistoryLimit = limit; }

    // -------------------------------------------------------------------------
    // Entry/Exit Callbacks
    // -------------------------------------------------------------------------

    /// Set a callback to be invoked when the machine starts
    void setOnEntry(EntryCallback callback) { _entryCallback = std::move(callback); }

    /// Set a callback to be invoked when the machine stops
    void setOnExit(ExitCallback callback) { _exitCallback = std::move(callback); }

    /// Set both entry and exit callbacks
    void setCallbacks(EntryCallback onEntry, ExitCallback onExit = nullptr);

    // -------------------------------------------------------------------------
    // Event Handling
    // -------------------------------------------------------------------------

    /// Set a global event handler for the machine
    /// Handler is called before events are processed by states
    void setEventHandler(EventHandler handler) { _eventHandler = std::move(handler); }

    /// Enable automatic property restoration when states exit
    /// When enabled, properties set via QGCState::setProperty/setEnabled/setVisible
    /// are automatically restored to their previous values when the state exits.
    void enablePropertyRestore();

    /// Check if property restoration is enabled
    bool isPropertyRestoreEnabled() const;

    /// Set a global error state that all QGCState errors will transition to
    /// @param errorState The state to transition to on error
    void setGlobalErrorState(QAbstractState* errorState);

    /// Get the global error state
    QAbstractState* globalErrorState() const { return _globalErrorState; }

    /// Register a QGCState with the machine and automatically wire up error transitions
    /// If a global error state is set, the state's error() signal will transition to it
    /// @param state The state to register
    void registerState(QGCState* state);

    /// Register a QGCAbstractState with the machine and automatically wire up error transitions
    /// @param state The state to register
    void registerState(QGCAbstractState* state);

    /// Create and register a FunctionState
    /// @param stateName Name for the state
    /// @param function Function to execute
    /// @return The created state
    FunctionState* addFunctionState(const QString& stateName, std::function<void()> function);

    /// Create and register an AsyncFunctionState
    /// @param stateName Name for the state
    /// @param setupFunction Function to execute on entry
    /// @param timeoutMsecs Optional timeout
    /// @return The created state
    AsyncFunctionState* addAsyncFunctionState(const QString& stateName,
                                               AsyncFunctionState::SetupFunction setupFunction,
                                               int timeoutMsecs = 0);

    /// Create and register a DelayState
    /// @param delayMsecs Delay duration
    /// @return The created state
    DelayState* addDelayState(int delayMsecs);

    /// Create and register a ParallelState
    /// @param stateName Name for the state
    /// @return The created state
    ParallelState* addParallelState(const QString& stateName);

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
    /// @param stateName Name for the state
    /// @return The created final state
    QGCFinalState* addFinalState(const QString& stateName = QString());

    /// Create and register a ConditionalState
    /// @param stateName Name for the state
    /// @param predicate Condition to evaluate
    /// @param action Action to execute if predicate is true
    /// @return The created state
    ConditionalState* addConditionalState(const QString& stateName,
                                          ConditionalState::Predicate predicate,
                                          ConditionalState::Action action = nullptr);

    /// Create and register a WaitForSignalState
    /// @param stateName Name for the state
    /// @param sender Object that emits the signal
    /// @param signal Signal to wait for
    /// @param timeoutMsecs Optional timeout (0 = no timeout)
    /// @return The created state
    template<typename Func>
    WaitForSignalState* addWaitForSignalState(const QString& stateName,
                                               const QObject* sender,
                                               Func signal,
                                               int timeoutMsecs = 0)
    {
        auto* state = new WaitForSignalState(stateName, this, sender, signal, timeoutMsecs);
        registerState(state);
        return state;
    }

    /// Create and register a SubMachineState
    /// @param stateName Name for the state
    /// @param factory Function that creates the child state machine
    /// @return The created state
    SubMachineState* addSubMachineState(const QString& stateName, SubMachineState::MachineFactory factory);

    /// Create and register an EventQueuedState
    /// @param stateName Name for the state
    /// @param eventName Event name to wait for
    /// @param timeoutMsecs Optional timeout (0 = no timeout)
    /// @return The created state
    EventQueuedState* addEventQueuedState(const QString& stateName,
                                          const QString& eventName,
                                          int timeoutMsecs = 0);

    /// Create a composite state that performs an action, waits for a duration, then finishes.
    /// The state has internal structure: entry -> timing -> done (final).
    /// When done, emits QState::finished() which can be used for transitions.
    ///
    /// Inspired by Qt's trafficlight example pattern.
    ///
    /// Example usage:
    /// @code
    /// auto* showMessage = machine.createTimedActionState("ShowMessage", 3000,
    ///     []() { qDebug() << "Message shown"; },
    ///     []() { qDebug() << "Message hidden"; }
    /// );
    /// showMessage->addTransition(showMessage, &QState::finished, nextState);
    /// @endcode
    ///
    /// @param stateName Name for the state
    /// @param durationMsecs How long to stay in the state
    /// @param onEntry Optional action to perform when state is entered
    /// @param onExit Optional action to perform when state is exited
    /// @return The created composite state (use QState::finished for transitions)
    QState* createTimedActionState(const QString& stateName,
                                   int durationMsecs,
                                   std::function<void()> onEntry = nullptr,
                                   std::function<void()> onExit = nullptr);

    // -------------------------------------------------------------------------
    // State Queries
    // -------------------------------------------------------------------------

    /// Check if a specific state is currently active
    /// @param state The state to check
    /// @return true if the state is in the current configuration
    bool isStateActive(QAbstractState* state) const;

    /// Get all currently active states
    /// @return Set of active states
    QSet<QAbstractState*> activeStates() const { return configuration(); }

    /// Find a state by name
    /// @param stateName The object name of the state
    /// @return The state, or nullptr if not found
    QAbstractState* findState(const QString& stateName) const;

    /// Find all states of a specific type
    template<typename T>
    QList<T*> findStates() const
    {
        return findChildren<T*>();
    }

    // -------------------------------------------------------------------------
    // Error Handling & Recovery
    // -------------------------------------------------------------------------

    /// Check if the machine is in an error state
    bool isInErrorState() const;

    /// Get the last error message (from Qt's internal error handling)
    QString lastError() const { return errorString(); }

    /// Clear the error state and optionally restart
    /// @param restart If true, restarts the machine after clearing
    void clearError(bool restart = false);

    /// Reset the machine and restart from a specific state.
    /// Useful for retrying from a failed state without restarting the entire sequence.
    /// @param state The state to start from
    /// @return true if successful, false if state is invalid
    bool resetToState(QAbstractState* state);

    /// Recover from error state by restarting the machine.
    /// If not in error state, this has no effect.
    /// @return true if recovery was initiated
    bool recoverFromError();

    /// Set a recovery state to transition to after error handling.
    /// When an error occurs and this is set, the machine can recover
    /// to this state instead of stopping.
    /// @param state The state to recover to (nullptr to disable)
    void setRecoveryState(QAbstractState* state) { _recoveryState = state; }

    /// Get the recovery state
    QAbstractState* recoveryState() const { return _recoveryState; }

    /// Attempt recovery to the recovery state (if set).
    /// @return true if recovery was initiated
    bool attemptRecovery();

    // -------------------------------------------------------------------------
    // Lifecycle Helpers
    // -------------------------------------------------------------------------

    /// Start the machine if not already running
    void ensureRunning();

    /// Stop the machine and optionally delete it
    /// @param deleteOnStop If true, the machine will be deleted when stopped
    void stopMachine(bool deleteOnStop = true);

    /// Restart the machine (stop then start)
    void restart();

    // -------------------------------------------------------------------------
    // Transition Helpers
    // -------------------------------------------------------------------------

    /// Create a simple signal transition between states
    /// @param from Source state
    /// @param signal Signal that triggers the transition
    /// @param to Target state
    /// @param animation Optional animation to play during transition
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
    /// @param from Source state
    /// @param signal Signal that triggers the transition
    /// @param to Target state
    /// @param guard Predicate that must return true for transition
    /// @param animation Optional animation to play during transition
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
    /// @param from Source state
    /// @param eventName Event name to match
    /// @param to Target state
    /// @param animation Optional animation to run during transition
    MachineEventTransition* addEventTransition(QState* from, const QString& eventName, QAbstractState* to,
                                               QAbstractAnimation* animation = nullptr);

    /// Create a timeout transition
    /// @param from Source state
    /// @param timeoutMsecs Timeout in milliseconds
    /// @param to Target state
    /// @param animation Optional animation to run during transition
    TimeoutTransition* addTimeoutTransition(QState* from, int timeoutMsecs, QAbstractState* to,
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

    /// Create a conditional guarded transition
    /// Transition only taken if guard returns true
    /// @param from Source state (must match signal's object type)
    /// @param signal Signal that triggers evaluation
    /// @param to Target state
    /// @param guard Predicate that must return true
    template<typename SenderType, typename Func>
    GuardedTransition* addConditionalTransition(SenderType* from, Func signal,
                                                 QAbstractState* to,
                                                 std::function<bool()> guard)
    {
        auto* transition = new GuardedTransition(from, signal, to, std::move(guard));
        from->addTransition(transition);
        return transition;
    }

    /// Create a self-loop transition that performs an action but stays in the same state.
    /// Useful for handling events/signals without changing state.
    ///
    /// Inspired by Qt's rogue example where input events are processed
    /// but the state machine stays in the input state.
    ///
    /// Example usage:
    /// @code
    /// // Process button clicks without leaving the state
    /// machine.addSelfLoopTransition(inputState, button, &QPushButton::clicked,
    ///     [this]() { handleClick(); });
    ///
    /// // Handle incoming messages while waiting
    /// machine.addSelfLoopTransition(waitState, receiver, &Receiver::messageReceived,
    ///     [this](const Message& msg) { processMessage(msg); });
    /// @endcode
    ///
    /// @param state The state to loop back to
    /// @param sender Object that emits the signal
    /// @param signal Signal that triggers the action
    /// @param action Action to perform on each signal
    /// @return The created transition
    template<typename Sender, typename Signal>
    QSignalTransition* addSelfLoopTransition(QState* state,
                                              Sender* sender,
                                              Signal signal,
                                              std::function<void()> action)
    {
        auto* transition = state->addTransition(sender, signal, state);
        if (action) {
            connect(transition, &QAbstractTransition::triggered, this, std::move(action));
        }
        return transition;
    }

    /// Create a self-loop internal transition (no state exit/entry).
    /// Unlike addSelfLoopTransition, this does NOT trigger onExit/onEntry.
    /// Use when you want to handle signals without re-entering the state.
    ///
    /// @param state The state containing the transition
    /// @param sender Object that emits the signal
    /// @param signal Signal that triggers the action
    /// @param action Action to perform on each signal
    /// @return The created internal transition
    template<typename Sender, typename Signal>
    InternalTransition* addInternalTransition(QState* state,
                                               Sender* sender,
                                               Signal signal,
                                               std::function<void()> action)
    {
        auto* transition = new InternalTransition(sender, signal, std::move(action));
        state->addTransition(transition);
        return transition;
    }

    // -------------------------------------------------------------------------
    // State Introspection
    // -------------------------------------------------------------------------

    /// Get all transitions originating from a state
    /// @param state The source state
    /// @return List of transitions from this state
    QList<QAbstractTransition*> transitionsFrom(QAbstractState* state) const;

    /// Get all transitions targeting a state
    /// @param state The target state
    /// @return List of transitions to this state
    QList<QAbstractTransition*> transitionsTo(QAbstractState* state) const;

    /// Get the target states reachable from a state
    /// @param state The source state
    /// @return List of states that can be reached from this state
    QList<QAbstractState*> reachableFrom(QAbstractState* state) const;

    /// Get states that can reach this state
    /// @param state The target state
    /// @return List of states that have transitions to this state
    QList<QAbstractState*> predecessorsOf(QAbstractState* state) const;

    // -------------------------------------------------------------------------
    // Debugging & Visualization
    // -------------------------------------------------------------------------

    /// Get a human-readable string showing the current state.
    /// Useful for debugging.
    /// @return String like "MachineName: CurrentState (running)" or "MachineName: (not running)"
    QString dumpCurrentState() const;

    /// Get a human-readable string showing all states and transitions.
    /// Useful for debugging and documentation.
    /// @return Multi-line string describing the machine structure
    QString dumpConfiguration() const;

    /// Log the current state to debug output.
    void logCurrentState() const;

    /// Log the full configuration to debug output.
    void logConfiguration() const;

    /// Export state machine structure as Graphviz DOT format.
    /// The output can be rendered using Graphviz tools like `dot -Tpng output.dot -o output.png`.
    /// @return DOT format string representation of the state machine
    QString exportAsDot() const;

    /// Find states that are not reachable from the initial state.
    /// These are typically configuration errors or dead code.
    /// @return List of unreachable states
    QList<QAbstractState*> unreachableStates() const;

    /// Calculate the maximum path length from initial state to any final state.
    /// Useful for understanding worst-case execution depth.
    /// @return Maximum number of transitions in any path, or -1 if no initial state
    int maxPathLength() const;

    /// Find states that have no outgoing transitions and are not final states.
    /// These are potential dead-ends.
    /// @return List of dead-end states
    QList<QAbstractState*> deadEndStates() const;

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

    // -------------------------------------------------------------------------
    // Timeout Configuration
    // -------------------------------------------------------------------------

    /// Set a timeout override for a specific state.
    /// Use this to adjust timeouts at runtime without recompiling.
    /// @param stateName The object name of the state
    /// @param timeoutMsecs New timeout in milliseconds
    void setTimeoutOverride(const QString& stateName, int timeoutMsecs);

    /// Remove a timeout override
    /// @param stateName The state name to remove override for
    void removeTimeoutOverride(const QString& stateName);

    /// Get the timeout override for a state, or -1 if not set
    int timeoutOverride(const QString& stateName) const;

    /// Get all timeout overrides
    QHash<QString, int> allTimeoutOverrides() const { return _timeoutOverrides; }

    /// Clear all timeout overrides
    void clearTimeoutOverrides() { _timeoutOverrides.clear(); }

    /// Get timeout statistics (how often each state timed out)
    QHash<QString, int> timeoutStats() const { return _timeoutStats; }

    /// Record a timeout for statistics
    void recordTimeout(const QString& stateName);

    /// Clear timeout statistics
    void clearTimeoutStats() { _timeoutStats.clear(); }

public slots:
    /// Start the state machine with debug logging
    void start();

signals:
    void error();

    /// Emitted when progress changes (if progress tracking is enabled)
    /// @param progress Overall progress from 0.0 to 1.0
    void progressUpdate(float progress);

    /// Emitted when a named event is posted to the machine
    /// Used by EventQueuedState to detect events
    /// @param eventName Name of the posted event
    void machineEvent(const QString& eventName);

    /// Emitted when the current state changes (for QML binding)
    void currentStateNameChanged();

    /// Emitted when the running state changes (for QML binding)
    void runningChanged();

    /// Emitted when the state history changes (for QML binding)
    void stateHistoryChanged();

protected:
    /// Override to perform actions when machine starts
    virtual void onEnter() {}

    /// Override to perform actions when machine stops
    virtual void onLeave() {}

    // QStateMachine overrides
    void onEntry(QEvent* event) override;
    void onExit(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    void _onStateEntered();
    float _calculateProgress() const;

    Vehicle* _vehicle = nullptr;
    QAbstractState* _globalErrorState = nullptr;
    QAbstractState* _recoveryState = nullptr;
    bool _deleteOnStop = false;
    EntryCallback _entryCallback;
    ExitCallback _exitCallback;
    EventHandler _eventHandler;

    // Progress tracking
    QList<QAbstractState*> _progressStates;
    QList<int> _progressWeights;
    int _progressTotalWeight = 0;
    int _progressCurrentIndex = -1;
    float _progressSubProgress = 0.0f;
    float _progressLastEmitted = 0.0f;

    // Inter-state data context
    StateContext _context;

    // QML state history
    QStringList _stateHistory;
    int _stateHistoryLimit = 20;

    // Timeout configuration
    QHash<QString, int> _timeoutOverrides;
    QHash<QString, int> _timeoutStats;
};
