#pragma once

#include <cstdint>
#include <functional>
#include <utility>

#include "VideoReceiver.h"

/// Lifecycle state for any GStreamer tee branch (decoder or recorder).
/// Single canonical definition — inherited by both GstDecodingBranch and
/// GstRecordingBranch.
enum class BranchState : uint8_t
{
    Off,       ///< Branch not active
    Starting,  ///< Being set up (elements being linked)
    Active,    ///< Running and producing/consuming data
    Stopping,  ///< EOS sent, waiting for async teardown
};

/// Non-QObject base for GstDecodingBranch and GstRecordingBranch.
/// Owns the shared lifecycle state field and the pending-stop callback
/// that both branches use identically.
class GstTeeBranch
{
public:
    GstTeeBranch() = default;
    virtual ~GstTeeBranch() = default;

    GstTeeBranch(const GstTeeBranch&) = delete;
    GstTeeBranch& operator=(const GstTeeBranch&) = delete;

    [[nodiscard]] BranchState state() const { return _state; }
    void setState(BranchState s) { _state = s; }

    void setPendingStop(std::function<void(VideoReceiver::STATUS)> cb) { _pendingStop = std::move(cb); }
    std::function<void(VideoReceiver::STATUS)> takePendingStop() { return std::exchange(_pendingStop, nullptr); }

protected:
    BranchState _state = BranchState::Off;
    std::function<void(VideoReceiver::STATUS)> _pendingStop;
};
