#include "VideoBackendRegistry.h"

#include "QGCLoggingCategory.h"
#include "QtMultimediaReceiver.h"
#include "UVCReceiver.h"

#include <QtCore/QMetaEnum>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtMultimedia/QMediaDevices>
#include <array>

QGC_LOGGING_CATEGORY(VideoBackendRegistryLog, "Video.VideoBackendRegistry")

namespace {

struct Entry
{
    VideoBackendRegistry::ReceiverFactoryFn factory;
    VideoBackendRegistry::AvailabilityFn    availability;
    VideoBackendRegistry::BackendDescriptor descriptor;
};

// Must match the BackendKind enum: GStreamer=0, QtMultimedia=1, UVC=2.
constexpr size_t kBackendCount = 3;
static_assert(kBackendCount == 3, "Update kBackendCount when BackendKind gains a new enumerator");

QMutex& registryMutex()
{
    static QMutex m;
    return m;
}

std::array<Entry, kBackendCount>& entries()
{
    static std::array<Entry, kBackendCount> e;
    return e;
}

const char* kindName(VideoReceiver::BackendKind kind)
{
    const QMetaEnum me = QMetaEnum::fromType<VideoReceiver::BackendKind>();
    return me.valueToKey(static_cast<int>(kind));
}

VideoBackendRegistry::BackendDescriptor defaultDescriptor(VideoReceiver::BackendKind kind)
{
    using Kind = VideoReceiver::BackendKind;
    VideoBackendRegistry::BackendDescriptor descriptor;
    descriptor.kind = kind;
    descriptor.name = kindName(kind);

    switch (kind) {
        case Kind::GStreamer:
            descriptor.capabilities = VideoReceiver::CapStreaming | VideoReceiver::CapHWDecode |
                                      VideoReceiver::CapRecording | VideoReceiver::CapLowLatency |
                                      VideoReceiver::CapRecordingLossless;
            descriptor.supportsGStreamerSources = true;
            descriptor.supportsLosslessRecording = true;
            descriptor.supportsPlatformFrames = true;
            descriptor.supportsLatency = true;
            break;
        case Kind::QtMultimedia:
            descriptor.capabilities = VideoReceiver::CapStreaming | VideoReceiver::CapRecording;
            descriptor.supportsQtSources = true;
            break;
        case Kind::UVC:
            descriptor.capabilities = VideoReceiver::CapLocalCamera | VideoReceiver::CapRecording;
            descriptor.supportsLocalCamera = true;
            break;
    }
    return descriptor;
}

bool descriptorSupportsSource(const VideoBackendRegistry::BackendDescriptor& descriptor,
                              const VideoSourceResolver::SourceDescriptor& source)
{
    if (source.isLocalCamera)
        return descriptor.supportsLocalCamera;
    if (source.requiresGStreamer)
        return descriptor.supportsGStreamerSources;
    return descriptor.supportsQtSources;
}

}  // namespace

VideoBackendRegistry& VideoBackendRegistry::instance()
{
    static VideoBackendRegistry s;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        s.registerBackend(VideoReceiver::BackendKind::QtMultimedia,
                          &QtMultimediaReceiver::createVideoReceiver);
        s.registerBackend(VideoReceiver::BackendKind::UVC,
                          [](QObject* parent) -> VideoReceiver* { return new UVCReceiver(parent); },
                          [] { return !QMediaDevices::videoInputs().isEmpty(); });
    }
    return s;
}

void VideoBackendRegistry::registerBackend(VideoReceiver::BackendKind kind,
                                           ReceiverFactoryFn factory,
                                           AvailabilityFn availability)
{
    registerBackend(kind, std::move(factory), std::move(availability), defaultDescriptor(kind));
}

void VideoBackendRegistry::registerBackend(VideoReceiver::BackendKind kind,
                                           ReceiverFactoryFn factory,
                                           AvailabilityFn availability,
                                           BackendDescriptor descriptor)
{
    const size_t idx = static_cast<size_t>(kind);
    const bool available = factory.operator bool();
    if (!descriptor.name)
        descriptor = defaultDescriptor(kind);
    descriptor.kind = kind;
    {
        QMutexLocker lock(&registryMutex());
        entries()[idx] = { std::move(factory), std::move(availability), descriptor };
    }
    qCDebug(VideoBackendRegistryLog) << kindName(kind) << "backend" << (available ? "registered" : "cleared");
}

void VideoBackendRegistry::unregisterBackend(VideoReceiver::BackendKind kind)
{
    registerBackend(kind, {}, [] { return false; });
}

VideoBackendRegistry::ReceiverFactoryFn VideoBackendRegistry::factory(VideoReceiver::BackendKind kind) const
{
    QMutexLocker lock(&registryMutex());
    return entries()[static_cast<size_t>(kind)].factory;
}

bool VideoBackendRegistry::isAvailable(VideoReceiver::BackendKind kind) const
{
    // Snapshot under lock, probe outside to avoid reentrancy.
    AvailabilityFn probe;
    ReceiverFactoryFn fn;
    {
        QMutexLocker lock(&registryMutex());
        const auto& e = entries()[static_cast<size_t>(kind)];
        fn    = e.factory;
        probe = e.availability;
    }
    return fn && probe && probe();
}

VideoBackendRegistry::BackendDescriptor VideoBackendRegistry::descriptor(VideoReceiver::BackendKind kind) const
{
    QMutexLocker lock(&registryMutex());
    const auto& entry = entries()[static_cast<size_t>(kind)];
    return entry.descriptor.name ? entry.descriptor : defaultDescriptor(kind);
}

std::array<VideoBackendRegistry::BackendDescriptor, 3> VideoBackendRegistry::descriptors() const
{
    QMutexLocker lock(&registryMutex());
    std::array<BackendDescriptor, kBackendCount> out;
    for (size_t i = 0; i < kBackendCount; ++i) {
        const auto kind = static_cast<VideoReceiver::BackendKind>(i);
        out[i] = entries()[i].descriptor.name ? entries()[i].descriptor : defaultDescriptor(kind);
    }
    return out;
}

VideoBackendRegistry::ReceiverFactoryFn
VideoBackendRegistry::factoryForSource(const VideoSourceResolver::SourceDescriptor& source) const
{
    const auto preferred = source.preferredBackend;
    if (isAvailable(preferred) && descriptorSupportsSource(descriptor(preferred), source))
        return factory(preferred);

    if (!source.requiresGStreamer && !source.isLocalCamera) {
        const auto fallback = VideoReceiver::BackendKind::QtMultimedia;
        if (isAvailable(fallback) && descriptorSupportsSource(descriptor(fallback), source))
            return factory(fallback);
    }

    return {};
}

void VideoBackendRegistry::clear()
{
    QMutexLocker lock(&registryMutex());
    for (auto& e : entries())
        e = {};
}
