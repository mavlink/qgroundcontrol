#pragma once

#include "VideoStream.h"
#include "VideoStreamStateMachine.h"

/// Lifecycle policy kept outside VideoStream's QML-facing model surface.
class VideoStreamLifecyclePolicy
{
public:
    [[nodiscard]] static VideoStreamStateMachine::Policy policyForRole(VideoStream::Role role);
    [[nodiscard]] static VideoStream::SessionState mapFsmState(VideoStreamFsm::State state);
};
