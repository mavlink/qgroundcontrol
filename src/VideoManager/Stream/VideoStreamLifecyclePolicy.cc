#include "VideoStreamLifecyclePolicy.h"

#include <array>

namespace {

static constexpr int kDefaultCircuitFailureThreshold = 3;
static constexpr int kDefaultCircuitResetTimeoutMs = 8000;
static constexpr int kLocalCameraCircuitFailureThreshold = 1;
static constexpr int kLocalCameraCircuitResetTimeoutMs = 0;
static constexpr int kDynamicCircuitFailureThreshold = 1;
static constexpr int kDynamicCircuitResetTimeoutMs = 30000;

}  // namespace

VideoStreamStateMachine::Policy VideoStreamLifecyclePolicy::policyForRole(VideoStream::Role role)
{
    struct RolePolicy
    {
        bool allowSoftReconnect = true;
        int circuitFailureThreshold = 3;
        int circuitResetTimeoutMs = 8000;
    };

    static constexpr std::array<RolePolicy, VideoStream::RoleCount> kPolicies = {{
        {true, kDefaultCircuitFailureThreshold, kDefaultCircuitResetTimeoutMs},
        {true, kDefaultCircuitFailureThreshold, kDefaultCircuitResetTimeoutMs},
        // UVC: local capture reconnect is driven by QMediaDevices.
        {false, kLocalCameraCircuitFailureThreshold, kLocalCameraCircuitResetTimeoutMs},
        // Dynamic: fail bad MAVLink URIs quickly.
        {false, kDynamicCircuitFailureThreshold, kDynamicCircuitResetTimeoutMs},
    }};

    const RolePolicy rolePolicy = kPolicies.at(static_cast<size_t>(role));
    VideoStreamStateMachine::Policy policy;
    policy.allowSoftReconnect = rolePolicy.allowSoftReconnect;
    policy.circuitFailureThreshold = rolePolicy.circuitFailureThreshold;
    policy.circuitResetTimeoutMs = rolePolicy.circuitResetTimeoutMs;
    return policy;
}

VideoStream::SessionState VideoStreamLifecyclePolicy::mapFsmState(VideoStreamFsm::State state)
{
    using FsmState = VideoStreamFsm::State;
    switch (state) {
        case FsmState::Idle:
        case FsmState::Failed:
            return VideoStream::SessionState::Stopped;
        case FsmState::Starting:
            return VideoStream::SessionState::Starting;
        case FsmState::Connected:
        case FsmState::Streaming:
        case FsmState::Paused:
        case FsmState::Reconnecting:
            return VideoStream::SessionState::Running;
        case FsmState::Stopping:
            return VideoStream::SessionState::Stopping;
    }
    return VideoStream::SessionState::Stopped;
}
