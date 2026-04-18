#include "VideoRecordingPolicy.h"

#include "VideoFrameDelivery.h"
#include "VideoReceiver.h"
#include "VideoRecorder.h"

std::unique_ptr<VideoRecorder> VideoRecordingPolicy::createRecorder(const VideoSourceResolver::SourceDescriptor& source,
                                                                    VideoReceiver* receiver,
                                                                    VideoFrameDelivery* delivery,
                                                                    QObject* parent,
                                                                    VideoStream* owner,
                                                                    const RecorderFactory& testFactory)
{
    if (!receiver)
        return {};

    if (testFactory)
        return std::unique_ptr<VideoRecorder>(testFactory(owner));

    if ((source.uri.isEmpty() && !source.isLocalCamera) || !delivery ||
        !(receiver->capabilities() & VideoReceiver::CapRecording))
        return {};

    return std::unique_ptr<VideoRecorder>(receiver->createRecorder(delivery, parent));
}
