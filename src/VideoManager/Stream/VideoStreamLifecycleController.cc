#include "VideoStreamLifecycleController.h"

#include "VideoStreamLifecyclePolicy.h"

#include <utility>

VideoStreamLifecycleController::VideoStreamLifecycleController(QString name,
                                                               VideoStreamStateMachine::Policy policy,
                                                               QObject* parent)
    : QObject(parent)
    , _name(std::move(name))
    , _policy(std::move(policy))
{
}

VideoStreamLifecycleController::SessionState VideoStreamLifecycleController::sessionState() const
{
    if (!_fsm)
        return SessionState::Stopped;
    return VideoStreamLifecyclePolicy::mapFsmState(_fsm->state());
}

VideoStreamFsm::State VideoStreamLifecycleController::fsmState() const
{
    return _fsm ? _fsm->state() : VideoStreamFsm::State::Idle;
}

bool VideoStreamLifecycleController::bind(VideoReceiver* receiver)
{
    if (_fsm || !receiver)
        return false;

    _fsm = std::make_unique<VideoStreamStateMachine>(_name + QStringLiteral("/fsm"), _policy, this);

    connect(_fsm.get(), &VideoStreamStateMachine::stateChanged, this,
            [this](VideoStreamStateMachine::State newFsmState) {
                emit fsmStateChanged(newFsmState);
                const SessionState mapped = VideoStreamLifecyclePolicy::mapFsmState(newFsmState);
                if (mapped != _lastMappedState) {
                    _lastMappedState = mapped;
                    emit sessionStateChanged(mapped);
                }
            });
    connect(_fsm.get(), &VideoStreamStateMachine::receiverFullyStarted,
            this, &VideoStreamLifecycleController::receiverFullyStarted);
    connect(_fsm.get(), &VideoStreamStateMachine::receiverFullyStopped,
            this, &VideoStreamLifecycleController::receiverFullyStopped);
    connect(_fsm.get(), &VideoStreamStateMachine::reconnectRequested,
            this, &VideoStreamLifecycleController::reconnectRequested);

    if (!_fsm->bind(receiver)) {
        _fsm.reset();
        return false;
    }

    _fsm->start();
    _lastMappedState = SessionState::Stopped;
    return true;
}

void VideoStreamLifecycleController::destroy()
{
    if (!_fsm)
        return;

    const SessionState preDestroy = VideoStreamLifecyclePolicy::mapFsmState(_fsm->state());
    if (_fsm->isRunning())
        _fsm->stop();
    _fsm.reset();

    emit fsmStateChanged(VideoStreamStateMachine::State::Idle);
    if (preDestroy != SessionState::Stopped) {
        _lastMappedState = SessionState::Stopped;
        emit sessionStateChanged(SessionState::Stopped);
    }
}

void VideoStreamLifecycleController::requestStart(uint32_t timeoutS)
{
    if (!_fsm)
        return;
    _fsm->setStartTimeoutS(static_cast<int>(timeoutS));
    _fsm->requestStart();
}

void VideoStreamLifecycleController::requestStop()
{
    if (_fsm)
        _fsm->requestStop();
}

void VideoStreamLifecycleController::handleReceiverError(VideoReceiver::ErrorCategory category,
                                                         const QString& message)
{
    if (_fsm)
        _fsm->handleReceiverError(category, message);
}
