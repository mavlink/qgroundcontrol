#pragma once

#include <functional>
#include <memory>

#include <QtCore/QStringList>
#include <QtCore/QtTypes>
#include <QtMultimedia/QMediaFormat>

#include "VideoSourceResolver.h"

class QObject;
class VideoFrameDelivery;
class VideoRecorder;
class VideoReceiver;
class VideoStream;

/// Chooses and creates the recorder for a receiver session.
///
/// Receivers provide their recorder implementations, but this policy is the
/// central decision point for test overrides, fallback, and future
/// source/recording eligibility rules.
///
/// Sources that need GStreamer transport support can use a native recording
/// service, while Qt Multimedia remains the only display receiver. Other
/// sources fall back to the receiver's frame-recorder path.
class VideoRecordingPolicy
{
public:
    using RecorderFactory = std::function<VideoRecorder*(VideoStream*)>;

    enum class RecorderBackend : quint8
    {
        None,
        TestFactory,
        GStreamerNative,
        FrameDelivery,
    };

    [[nodiscard]] static bool isSupportedFileFormat(QMediaFormat::FileFormat format);
    [[nodiscard]] static QStringList videoNameFilters();
    [[nodiscard]] static uint64_t storageLimitBytes(uint32_t maxVideoSizeMiB);
    [[nodiscard]] static RecorderBackend selectBackend(const VideoSourceResolver::SourceDescriptor& source,
                                                       VideoReceiver* receiver,
                                                       VideoFrameDelivery* delivery,
                                                       const RecorderFactory& testFactory);

    [[nodiscard]] static std::unique_ptr<VideoRecorder> createRecorder(const VideoSourceResolver::SourceDescriptor& source,
                                                                       VideoReceiver* receiver,
                                                                       VideoFrameDelivery* delivery,
                                                                       QObject* parent,
                                                                       VideoStream* owner,
                                                                       const RecorderFactory& testFactory);
};
