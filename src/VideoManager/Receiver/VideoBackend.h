#pragma once

#include <optional>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/qcontainerfwd.h>

#include "VideoDecoderOptions.h"

class Fact;
class QObject;
class QQuickWindow;
class VideoReceiver;

/// The network video backend this build has, behind which callers (VideoManager, VideoSettings, QGCCorePlugin, the
/// platform startup code) never name a concrete one: GStreamer, or, without it, Qt Multimedia's player
/// (QtMultimediaVideoReceiver), which a GStreamer build also offers through VideoSettings::videoBackend.
class VideoBackend
{
public:
    /// Outcome of prepareEnvironment(), threaded into initialize() so backend setup validity is passed as a value
    /// rather than stashed in process-global state.
    struct EnvPrepResult
    {
        bool ok = true;
        QString error;
    };

    virtual ~VideoBackend() = default;

    static VideoBackend& instance();

    /// True when the backend is skipped under unit tests (opt back in with QGC_TEST_ENABLE_GSTREAMER).
    static bool disabledForUnitTests();

    /// The persisted gstDebugLevel setting, read before the settings system exists; nullopt when it was never set.
    static std::optional<int> persistedDebugLevel();

    /// Routes changes of @p fact (AppSettings::gstDebugLevel) to setDebugLevel() while @p context lives.
    static void bindDebugLevelFact(Fact* fact, QObject* context);

    /// Qt graphics setup the backend's GPU video paths need; call it before the QGuiApplication exists, as Qt reads it
    /// at startup. @p softwareRendering: the user asked for software rendering (--swrast), which it must keep.
    virtual void configureQtGraphics(bool softwareRendering) = 0;

    /// True when the build plays network video streams; their settings are shown only then.
    virtual bool hasStreaming() const = 0;

    /// True for GStreamer: the decoder, RTP jitter-buffer and display-sink settings apply, and Qt Multimedia's player
    /// is the alternative VideoSettings::videoBackend offers.
    virtual bool hasPipelineSettings() const = 0;

    /// True when the build can show decoded frames without copying them through the CPU.
    virtual bool hasGpuVideoPath() const = 0;

    /// True when initialize() must run, off the GUI thread, before receivers are created.
    virtual bool needsAsyncInit() const = 0;

    virtual VideoReceiver* createReceiver(QObject* parent) = 0;

    virtual EnvPrepResult prepareEnvironment() = 0;

    /// @p debugLevel: the persisted debug threshold to start with (persistedDebugLevel()).
    virtual bool initialize(const QStringList& arguments, const EnvPrepResult& envResult,
                            std::optional<int> debugLevel) = 0;

    /// The boot test's check that the initialized backend decodes and records; true without a backend.
    virtual bool selfTest() = 0;

    virtual void applyDecoderPriorities(int rawOption) = 0;

    /// The hardware decoder families present, as forceVideoDecoder values (never Default or Software).
    virtual QList<VideoDecoder::Options> availableDecoderFamilies() = 0;

    virtual void setDebugLevel(int level) = 0;

    virtual void onMainWindowReady(QQuickWindow* window) = 0;

    /// Android: hands the backend the JavaVM, application context and class loader (jobject, or null) its plugins
    /// look up through JNI.
    virtual void setAndroidJniState(void* javaVm, void* appContext, void* classLoader) = 0;
};
