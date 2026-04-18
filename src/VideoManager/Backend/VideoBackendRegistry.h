#pragma once

#include <QtCore/QLoggingCategory>

#include <array>
#include <functional>

#include "VideoReceiver.h"  // BackendKind
#include "VideoSourceResolver.h"

Q_DECLARE_LOGGING_CATEGORY(VideoBackendRegistryLog)

class QObject;

/// Single source of truth for backend availability and factory lookup for ALL
/// backends (GStreamer, QtMultimedia, UVC).
///
/// Ownership inversion: each backend registers itself here — either at startup
/// (QtMultimedia/UVC) or once async init completes (GStreamer). Consumers
/// (VideoReceiverFactory, QML bindings, unit tests) query availability and
/// retrieve factories without per-backend knowledge.
///
/// Thread safety: `registerBackend()` may be called from a worker thread
/// (GStreamer::initAsync). Reads happen from the UI thread. All slot access is
/// protected by a mutex; availability probes are invoked outside the lock to
/// avoid reentrancy.
class VideoBackendRegistry
{
public:
    using ReceiverFactoryFn = std::function<VideoReceiver*(QObject* parent)>;
    using AvailabilityFn    = std::function<bool()>;

    struct BackendDescriptor
    {
        VideoReceiver::BackendKind kind = VideoReceiver::BackendKind::QtMultimedia;
        const char* name = nullptr;
        VideoReceiver::Capabilities capabilities = {};
        bool supportsGStreamerSources = false;
        bool supportsQtSources = false;
        bool supportsLocalCamera = false;
        bool supportsLosslessRecording = false;
        bool supportsPlatformFrames = false;
        bool supportsLatency = false;
    };

    static VideoBackendRegistry& instance();

    /// Register (or replace) a backend. Passing a default-constructed factory
    /// clears the slot. `availability` defaults to `always true` — supply a
    /// dynamic probe for backends whose availability depends on runtime state
    /// (e.g. UVC queries `QMediaDevices::videoInputs()`).
    void registerBackend(VideoReceiver::BackendKind kind,
                         ReceiverFactoryFn factory,
                         AvailabilityFn availability = [] { return true; });
    void registerBackend(VideoReceiver::BackendKind kind,
                         ReceiverFactoryFn factory,
                         AvailabilityFn availability,
                         BackendDescriptor descriptor);

    /// Convenience: de-registers `kind`.
    void unregisterBackend(VideoReceiver::BackendKind kind);

    /// Returns the registered factory for `kind`, or an empty std::function if
    /// no backend has registered. Does NOT consult the availability probe.
    [[nodiscard]] ReceiverFactoryFn factory(VideoReceiver::BackendKind kind) const;

    /// Checks both registration and the availability probe for `kind`.
    [[nodiscard]] bool isAvailable(VideoReceiver::BackendKind kind) const;

    [[nodiscard]] BackendDescriptor descriptor(VideoReceiver::BackendKind kind) const;
    [[nodiscard]] std::array<BackendDescriptor, 3> descriptors() const;

    /// Returns the preferred available backend factory for a resolved source.
    [[nodiscard]] ReceiverFactoryFn factoryForSource(const VideoSourceResolver::SourceDescriptor& source) const;

    /// Test hook — resets all registered state. Production code does not call this.
    void clear();

    VideoBackendRegistry(const VideoBackendRegistry&) = delete;
    VideoBackendRegistry& operator=(const VideoBackendRegistry&) = delete;
    VideoBackendRegistry(VideoBackendRegistry&&) = delete;
    VideoBackendRegistry& operator=(VideoBackendRegistry&&) = delete;

private:
    VideoBackendRegistry() = default;
    ~VideoBackendRegistry() = default;
};
