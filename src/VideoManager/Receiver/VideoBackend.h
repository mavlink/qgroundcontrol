#pragma once

#include <optional>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/qcontainerfwd.h>

#include "VideoDecoderOptions.h"

class QObject;
class QQuickWindow;
class VideoReceiver;

/// The network video backend this build has, behind which callers (VideoManager, VideoSettings, QGCCorePlugin, the
/// platform startup code) never name a concrete one: GStreamer, or, without it, Qt Multimedia's player
/// (QtMultimediaVideoReceiver), which a GStreamer build also offers through VideoSettings::videoBackend and falls back
/// to when GStreamer fails to initialize.
class VideoBackend
{
public:
    /// Outcome of prepareEnvironment(): whether the backend's runtime paths validated, with an error to log when they
    /// didn't. Threaded into initialize() so backend setup validity is passed as a value rather than stashed in
    /// process-global state.
    struct EnvPrepResult
    {
        bool ok = true;
        QString error;
    };

    virtual ~VideoBackend() = default;

    VideoBackend(const VideoBackend&) = delete;
    VideoBackend& operator=(const VideoBackend&) = delete;

    static VideoBackend& instance();

    /// True when the backend is skipped under unit tests.
    static bool disabledForUnitTests();

    /// The persisted gstDebugLevel setting, read before the settings system exists; nullopt when it was never set.
    static std::optional<int> persistedDebugLevel();

    /// What the backend offers.
    struct Capabilities
    {
        /// Plays network video streams; their settings are shown only then.
        bool streaming = false;
        /// GStreamer: the decoder, RTP jitter-buffer and display-sink settings apply, and Qt Multimedia's player is the
        /// alternative VideoSettings::videoBackend offers.
        bool pipelineSettings = false;
        /// Shows decoded frames without copying them through the CPU.
        bool gpuVideoPath = false;
        /// initialize() must run, off the GUI thread, before receivers are created.
        bool asyncInit = false;
    };

    struct InitParams
    {
        /// QCoreApplication::arguments(), snapshotted on the GUI thread.
        QStringList arguments;
        /// What prepareEnvironment() did.
        EnvPrepResult environment;
        /// The persisted debug threshold to start with (persistedDebugLevel()).
        std::optional<int> debugLevel;
    };

    virtual Capabilities capabilities() const = 0;

    /// A receiver that can play: once initialize() has failed, one that needs no initialization.
    virtual VideoReceiver* createReceiver(QObject* parent) = 0;

    // The rest is setup and tuning a backend that needs none leaves alone.

    /// Qt graphics setup the backend's GPU video paths need; call it before the QGuiApplication exists, as Qt reads it
    /// at startup. @p softwareRendering: the user asked for software rendering (--swrast), which it must keep.
    virtual void configureQtGraphics([[maybe_unused]] bool softwareRendering) {}

    /// GUI thread, before initialize().
    virtual EnvPrepResult prepareEnvironment() { return {}; }

    virtual bool initialize([[maybe_unused]] const InitParams& params) { return true; }

    /// What to tell the user once initialize() has failed: what failed, and what plays the streams instead.
    virtual QString initFailureMessage() const;

    /// The boot test's check that the initialized backend decodes and records.
    virtual bool selfTest() { return true; }

    virtual void applyDecoderPriorities([[maybe_unused]] VideoDecoderOption option) {}

    /// The hardware decoder families present, as forceVideoDecoder values (never Default or Software).
    virtual QList<VideoDecoderOption> availableDecoderFamilies() { return {}; }

    virtual void setDebugLevel([[maybe_unused]] int level) {}

    virtual void onMainWindowReady([[maybe_unused]] QQuickWindow* window) {}

    /// Android: hands the backend the JavaVM, application context and class loader (jobject, or null) its plugins
    /// look up through JNI.
    virtual void setAndroidJniState([[maybe_unused]] void* javaVm, [[maybe_unused]] void* appContext,
                                    [[maybe_unused]] void* classLoader)
    {}

protected:
    VideoBackend() = default;
};
