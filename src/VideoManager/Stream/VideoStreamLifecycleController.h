#pragma once

#include <QtCore/QObject>
#include <memory>

#include "VideoReceiver.h"
#include "VideoStreamFsmState.h"
#include "VideoStreamStateMachine.h"

class VideoStreamLifecycleController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoStreamLifecycleController)

public:
    using SessionState = VideoReceiver::ReceiverState;

    VideoStreamLifecycleController(QString name,
                                   VideoStreamStateMachine::Policy policy,
                                   QObject* parent = nullptr);
    ~VideoStreamLifecycleController() override = default;

    [[nodiscard]] VideoStreamStateMachine* fsm() const { return _fsm.get(); }
    [[nodiscard]] SessionState sessionState() const;
    [[nodiscard]] VideoStreamFsm::State fsmState() const;
    [[nodiscard]] bool isRunning() const { return _fsm && _fsm->isRunning(); }

    bool bind(VideoReceiver* receiver);
    void destroy();
    void requestStart(uint32_t timeoutS);
    void requestStop();
    void handleReceiverError(VideoReceiver::ErrorCategory category, const QString& message);

signals:
    void sessionStateChanged(VideoReceiver::ReceiverState newState);
    void fsmStateChanged(VideoStreamFsm::State newState);
    void receiverFullyStarted();
    void receiverFullyStopped();
    void reconnectRequested();

private:
    QString _name;
    VideoStreamStateMachine::Policy _policy;
    std::unique_ptr<VideoStreamStateMachine> _fsm;
    SessionState _lastMappedState = SessionState::Stopped;
};
