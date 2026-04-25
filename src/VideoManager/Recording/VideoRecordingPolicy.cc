#include "VideoRecordingPolicy.h"

#include "FrameDeliveryRecorder.h"
#include "VideoFrameDelivery.h"
#include "VideoReceiver.h"
#include "VideoRecorder.h"

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#include "GstNativeRecorder.h"
#endif

namespace {

static constexpr uint64_t kBytesPerMiB = 1024ULL * 1024ULL;
static const QStringList kVideoNameFilters{
    QStringLiteral("*.mkv"),
    QStringLiteral("*.mov"),
    QStringLiteral("*.mp4"),
};

}  // namespace

bool VideoRecordingPolicy::isSupportedFileFormat(QMediaFormat::FileFormat format)
{
    return format == QMediaFormat::Matroska || format == QMediaFormat::QuickTime || format == QMediaFormat::MPEG4;
}

QStringList VideoRecordingPolicy::videoNameFilters()
{
    // Must match the containers listed in VideoRecorder::Capabilities.
    return kVideoNameFilters;
}

uint64_t VideoRecordingPolicy::storageLimitBytes(uint32_t maxVideoSizeMiB)
{
    return static_cast<uint64_t>(maxVideoSizeMiB) * kBytesPerMiB;
}

VideoRecordingPolicy::RecorderBackend
VideoRecordingPolicy::selectBackend(const VideoSourceResolver::SourceDescriptor& source,
                                    VideoReceiver* receiver,
                                    VideoFrameDelivery* delivery,
                                    const RecorderFactory& testFactory)
{
    if (!receiver)
        return RecorderBackend::None;

    if (testFactory)
        return RecorderBackend::TestFactory;

    if ((source.uri.isEmpty() && !source.isLocalCamera) ||
        !(receiver->capabilities() & VideoReceiver::CapRecording))
        return RecorderBackend::None;

#ifdef QGC_GST_STREAMING
    if (GStreamer::isAvailable() && GstNativeRecorder::supportsSource(source))
        return RecorderBackend::GStreamerNative;
#endif

    return delivery ? RecorderBackend::FrameDelivery : RecorderBackend::None;
}

std::unique_ptr<VideoRecorder> VideoRecordingPolicy::createRecorder(const VideoSourceResolver::SourceDescriptor& source,
                                                                    VideoReceiver* receiver,
                                                                    VideoFrameDelivery* delivery,
                                                                    QObject* parent,
                                                                    VideoStream* owner,
                                                                    const RecorderFactory& testFactory)
{
    switch (selectBackend(source, receiver, delivery, testFactory)) {
    case RecorderBackend::TestFactory:
        return std::unique_ptr<VideoRecorder>(testFactory(owner));
    case RecorderBackend::GStreamerNative:
#ifdef QGC_GST_STREAMING
        return std::make_unique<GstNativeRecorder>(source, parent);
#else
        return {};
#endif
    case RecorderBackend::FrameDelivery:
        return std::make_unique<FrameDeliveryRecorder>(delivery, parent);
    case RecorderBackend::None:
    default:
        return {};
    }
}
