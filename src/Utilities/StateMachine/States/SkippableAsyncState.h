#pragma once

#include "WaitStateBase.h"

#include <functional>

/// Combines skip condition checking with async operation setup in a single state.
/// Common pattern in initialization workflows where operations should be skipped under certain conditions.
///
/// On entry:
/// 1. Evaluate skipPredicate()
/// 2. If true: call skipAction() (if provided), emit skipped()
/// 3. If false: call setupFunc(this), wait for complete() or timeout
///
/// Example usage:
/// @code
/// auto* missionState = new SkippableAsyncState(
///     "LoadMission",
///     parentState,
///     [this]() { return _shouldSkipForLinkType() || !_hasPrimaryLink(); },
///     [this](SkippableAsyncState* state) {
///         state->connectToCompletion(_missionManager, &MissionManager::newMissionItemsAvailable);
///         _missionManager->loadFromVehicle();
///     },
///     [this]() { qCDebug(Log) << "Skipping mission load"; }
/// );
///
/// // Both advance and skipped transition to same next state
/// missionState->addTransition(missionState, &QGCState::advance, nextState);
/// missionState->addTransition(missionState, &SkippableAsyncState::skipped, nextState);
/// @endcode
class SkippableAsyncState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(SkippableAsyncState)

public:
    using SkipPredicate = std::function<bool()>;
    using SetupFunction = std::function<void(SkippableAsyncState* state)>;
    using SkipAction = std::function<void()>;

    /// @param stateName Name for this state (for logging)
    /// @param parent Parent state
    /// @param skipPredicate If true, skip the async operation and emit skipped()
    /// @param setupFunc Function called when state is entered and not skipped.
    ///                  Should start the async operation.
    /// @param skipAction Optional function called when skipping (for cleanup or logging)
    /// @param timeoutMsecs Timeout in milliseconds, 0 for no timeout
    SkippableAsyncState(const QString& stateName,
                        QState* parent,
                        SkipPredicate skipPredicate,
                        SetupFunction setupFunc,
                        SkipAction skipAction = nullptr,
                        int timeoutMsecs = 0);

    /// Call this to signal that the async operation has completed successfully
    void complete() { waitComplete(); }

    /// Call this to signal that the async operation has failed
    void fail() { waitFailed(); }

    /// Connect to an external signal that will trigger completion
    /// @param sender The QObject that will emit the completion signal
    /// @param signal The signal that indicates completion
    template<typename Func>
    void connectToCompletion(typename QtPrivate::FunctionPointer<Func>::Object* sender, Func signal)
    {
        _completionConnection = connect(sender, signal, this, [this]() {
            complete();
        });
    }

    /// Connect to an external signal that will trigger completion, with a slot
    /// Useful when you need to process the signal data before completing
    template<typename Func, typename Slot>
    void connectToCompletion(typename QtPrivate::FunctionPointer<Func>::Object* sender, Func signal, Slot slot)
    {
        _completionConnection = connect(sender, signal, this, slot);
    }

    /// Set the skip predicate
    void setSkipPredicate(SkipPredicate predicate) { _skipPredicate = std::move(predicate); }

    /// Set the setup function
    void setSetupFunction(SetupFunction setupFunc) { _setupFunction = std::move(setupFunc); }

    /// Set the skip action
    void setSkipAction(SkipAction skipAction) { _skipAction = std::move(skipAction); }

signals:
    /// Emitted when skip predicate returns true and the state is skipped
    void skipped();

protected:
    void connectWaitSignal() override;
    void disconnectWaitSignal() override;
    void onWaitEntered() override;

private:
    SkipPredicate _skipPredicate;
    SetupFunction _setupFunction;
    SkipAction _skipAction;
    QMetaObject::Connection _completionConnection;
    bool _wasSkipped = false;
};
