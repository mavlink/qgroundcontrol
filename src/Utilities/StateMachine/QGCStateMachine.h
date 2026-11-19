#pragma once

#include <concepts>
#include <functional>
#include <type_traits>

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtStateMachine/QStateMachine>

#include "AsyncFunctionState.h"
#include "CompletionWaitState.h"
#include "DelayState.h"
#include "FunctionState.h"
#include "GuardedTransition.h"
#include "MachineEventTransition.h"
#include "QGCAbstractState.h"
#include "QGCFinalState.h"
#include "QGCStateMachineEvent.h"
#include "RetryTransition.h"
#include "RetryableRequestMessageState.h"
#include "SendMavlinkCommandState.h"
#include "SendMavlinkMessageState.h"
#include "SignalSender.h"
#include "SkippableAsyncState.h"
#include "VehicleMessageWaitState.h"
#include "WaitForMavlinkMessageState.h"
#include "WaitForParamResponseState.h"
#include "WaitStateBase.h"

class Vehicle;

/// \brief QStateMachine that knows its vehicle, whether it is in progress, and how far it has progressed
///
/// A machine can be a state of another machine, as a QStateMachine can: the outer machine starts it when it
/// enters it and stops it when it leaves it, and its finished() is that of a state. Qt does not leave the states
/// of a machine that stops, so a nested machine that is active then goes on, and entering it again while it is
/// still in progress is refused as a second start() is.
///
class QGCStateMachine : public QStateMachine
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCStateMachine)

    /// Current progress from 0.0 to 1.0
    Q_PROPERTY(float progress READ progress NOTIFY progressUpdate)

    /// Whether the machine is in progress
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY inProgressChanged)

public:
    QGCStateMachine(const QString& machineName, Vehicle* vehicle, QObject* parent = nullptr);

    /// @return The vehicle, nullptr if the machine has none or the vehicle has been destroyed
    Vehicle *vehicle() const { return _vehicle; }

    /// @return true from start() until the machine emits stopped or finished
    /// Unlike QStateMachine::isRunning(), this is already true before Qt processes the queued start.
    bool inProgress() const { return _inProgress; }

    /// Use inProgress() instead: isRunning() is false until Qt processes the queued start
    bool isRunning() const = delete;  // NOLINT(bugprone-derived-method-shadowing-base-method)

    /// Use start() and stop() instead: setRunning() calls QStateMachine::start() and bypasses start()
    void setRunning(bool running) = delete;  // NOLINT(bugprone-derived-method-shadowing-base-method)

    // -------------------------------------------------------------------------
    // Event Handling
    // -------------------------------------------------------------------------

    /// Post a named event to be processed immediately
    /// The machine must be running, which it is from the entry of its initial state and not yet when start()
    /// returns: an event for a machine that is not running is not posted, and a warning says so.
    /// @param eventName Name of the event
    /// @param data Optional payload data
    /// @param priority Event priority (HighPriority events are processed before NormalPriority)
    void postEvent(const QString& eventName, const QVariant& data = QVariant(),
                   EventPriority priority = NormalPriority);

    /// Post a named event with a delay. The machine must be running, as for postEvent().
    /// @param eventName Name of the event
    /// @param delayMsecs Delay in milliseconds
    /// @param data Optional payload data
    /// @return Event ID that QStateMachine::cancelDelayedEvent() takes, -1 if the event was not posted
    int postDelayedEvent(const QString& eventName, int delayMsecs, const QVariant& data = QVariant());

    // -------------------------------------------------------------------------
    // Lifecycle Helpers
    // -------------------------------------------------------------------------

    /// Restart the machine (stop then start). Its progress starts over as at every start(): a progress above 0 is
    /// followed by progressUpdate(0) before the machine starts.
    /// A machine in progress starts again once it has stopped. Until then it restarts once, however often it is
    /// asked to.
    void restart();

    // -------------------------------------------------------------------------
    // Transition Helpers
    // -------------------------------------------------------------------------

    /// The class of a state that is the source of a transition and the sender of the signal @p Func that triggers
    /// it: the class that declares the signal, or QState for a signal that QState inherits. A factory declares its
    /// source state with it so that only the signal is deduced and the state converts to the signal's class (see
    /// SignalSender.h).
    template <typename Func>
    using SignalSourceState =
        std::conditional_t<std::is_base_of_v<SignalSender<Func>, QState>, QState, SignalSender<Func>>;

    /// Create a simple signal transition between states
    /// @param from Source state, which emits the signal
    template <typename Func>
        requires std::derived_from<SignalSourceState<Func>, QState>
    QSignalTransition* addTransition(SignalSourceState<Func>* from, Func signal, QAbstractState* to)
    {
        // Qualified, because a source state that is a QGCStateMachine has this function in place of QState's
        return from->QState::addTransition(from, signal, to);
    }

    /// @overload
    /// @param signal Signal that triggers the transition, by name as SIGNAL() gives it
    QSignalTransition* addTransition(QState* from, const char* signal, QAbstractState* to);

    /// Create a guarded transition between states
    /// @param from Source state, which emits the signal
    /// @param guard Predicate that must return true for transition
    template <typename Func>
        requires std::derived_from<SignalSourceState<Func>, QState>
    GuardedTransition* addGuardedTransition(SignalSourceState<Func>* from, Func signal, QAbstractState* to,
                                            GuardedTransition::Guard guard)
    {
        auto* transition = new GuardedTransition(from, signal, to, std::move(guard));
        from->QState::addTransition(transition);
        return transition;
    }

    /// Create a machine event transition
    /// @param eventName Event name to match
    MachineEventTransition* addEventTransition(QState* from, const QString& eventName, QAbstractState* to);

    /// Create a retry transition that retries an action before advancing
    /// @param from Source state, which emits the signal
    /// @param signal Signal that triggers retry evaluation
    /// @param to Target state (used after max retries)
    /// @param retryAction Action to perform on retry
    /// @param maxRetries Maximum number of retries before advancing
    template <typename Func>
        requires std::derived_from<SignalSourceState<Func>, QState>
    RetryTransition* addRetryTransition(SignalSourceState<Func>* from, Func signal, QAbstractState* to,
                                        std::function<void()> retryAction, int maxRetries = 1)
    {
        auto* transition = new RetryTransition(from, signal, to, std::move(retryAction), maxRetries);
        // Qualified, because a source state that is a QGCStateMachine has addTransition() in place of QState's
        from->QState::addTransition(transition);
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
    /// The progress starts over, and a progress above 0 is followed by progressUpdate(0). An entry without a state
    /// or with a weight of 0 or less is ignored with a warning.
    ///
    /// @param stateWeights List of state/weight pairs in order of execution
    void setProgressWeights(const QList<QPair<QAbstractState*, int>>& stateWeights);

    /// Update sub-progress within the current state (0.0 to 1.0).
    /// Call this from state callbacks to show progress within a long-running state.
    /// @param subProgress Progress within current state (0.0 = just started, 1.0 = complete)
    void setSubProgress(float subProgress);

    /// Get the current overall progress (0.0 to 1.0)
    float progress() const;

public slots:
    /// Start the machine. Takes the place of QStateMachine::start(), which is not virtual, so that inProgress()
    /// is true from the call on. The progress starts over: a machine that had progressed reports 0.
    void start();  // NOLINT(bugprone-derived-method-shadowing-base-method)

signals:
    /// Emitted when progress changes (if progress tracking is enabled)
    /// @param progress Overall progress from 0.0 to 1.0
    void progressUpdate(float progress);

    /// Emitted when inProgress() changes
    void inProgressChanged();

protected:
    void onEntry(QEvent* event) override;

private:
    void _onStateEntered();
    void _setInProgress(bool inProgress);
    bool _canPostEvent(const QString& eventName) const;
    float _calculateProgress() const;
    void _startProgressOver();

    Vehicle* _vehicle = nullptr;
    bool _inProgress = false;
    bool _restartPending = false;

    // Progress tracking
    QList<QAbstractState*> _progressStates;
    QList<int> _progressWeights;
    int _progressTotalWeight = 0;
    int _progressCurrentIndex = -1;
    float _progressSubProgress = 0.0f;
    float _progressLastEmitted = 0.0f;
};

/// @return The QGCStateMachine that @p state belongs to, nullptr if it belongs to another kind of machine or to none
QGCStateMachine* qgcStateMachineOf(const QAbstractState* state);

/// @return The name of @p state for a log line: "Machine:State" in a QGCStateMachine, the name of the state otherwise
QString qualifiedStateName(const QAbstractState* state);
