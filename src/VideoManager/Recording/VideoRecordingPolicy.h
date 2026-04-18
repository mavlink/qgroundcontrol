#pragma once

#include <functional>
#include <memory>

#include "VideoSourceResolver.h"

class QObject;
class VideoFrameDelivery;
class VideoRecorder;
class VideoReceiver;
class VideoStream;

/// Chooses and creates the recorder for a receiver session.
///
/// Receiver backends still provide their native recorder implementations, but
/// this policy is the central decision point for test overrides, backend
/// fallback, and future source/recording eligibility rules.
class VideoRecordingPolicy
{
public:
    using RecorderFactory = std::function<VideoRecorder*(VideoStream*)>;

    [[nodiscard]] static std::unique_ptr<VideoRecorder> createRecorder(const VideoSourceResolver::SourceDescriptor& source,
                                                                       VideoReceiver* receiver,
                                                                       VideoFrameDelivery* delivery,
                                                                       QObject* parent,
                                                                       VideoStream* owner,
                                                                       const RecorderFactory& testFactory);
};
