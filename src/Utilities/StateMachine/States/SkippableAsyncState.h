#pragma once

#include <functional>

#include "CompletionWaitState.h"

/// \brief Combines skip condition checking with async operation setup in a single state.
///
/// Common pattern in initialization workflows where operations should be skipped under certain conditions.
///
/// On entry:
/// 1. Evaluate skipPredicate()
/// 2. If true: call skipAction() (if provided), emit skipped()
/// 3. If false: call setupFunc(this), wait for complete() or timeout
///
/// A retry that stays in the state after a timeout (RetryTransition) calls setupFunc(this) again, without asking
/// the predicate a second time.
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
///
class SkippableAsyncState : public CompletionWaitState
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

signals:
    /// Emitted when skip predicate returns true and the state is skipped
    void skipped();

protected:
    void onWaitEntered() override;
    void onWaitRestarted() override;

private:
    SkipPredicate _skipPredicate;
    SetupFunction _setupFunction;
    SkipAction _skipAction;
};
