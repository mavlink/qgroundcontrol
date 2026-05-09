#pragma once

class Fact;
class QObject;
class QQuickItem;
class QQuickWindow;
class QVideoSink;
class VideoReceiver;

namespace GStreamer
{

enum VideoDecoderOptions {
    ForceVideoDecoderDefault = 0,
    ForceVideoDecoderSoftware,
    ForceVideoDecoderNVIDIA,
    ForceVideoDecoderVAAPI,
    ForceVideoDecoderDirectX3D,
    ForceVideoDecoderVideoToolbox,
    ForceVideoDecoderIntel,
    ForceVideoDecoderVulkan,
    ForceVideoDecoderHardware
};

bool completeInit();
void setDebugLevel(int level);
void *createVideoSink(QQuickItem *widget, QObject *parent = nullptr);
void releaseVideoSink(void *sink);
VideoReceiver *createVideoReceiver(QObject *parent = nullptr);

/// Connect the appsink inside @p sinkBin to @p videoSink. Returns true on success.
bool setupAppSinkAdapter(void *sinkBin, QVideoSink *videoSink, QObject *adapterParent);

/// Toggle every appsink adapter parented under @p adapterParent. Used to drop frames at
/// the appsink while the host window is hidden/minimized — saves CPU vs. running the
/// full decode→render path against a non-visible sink. Safe to call repeatedly; no-op
/// when no adapters exist.
void setAppSinkAdaptersActive(QObject *adapterParent, bool active);

// Functions called from VideoManager. Stubbed when QGC_GST_STREAMING is off so
// callers don't need to ifdef their call sites — gstreamerEnabled() at runtime
// (constexpr) guards the behavior, the linker resolves the no-op stubs.
#ifdef QGC_GST_STREAMING
void prepareEnvironment();
bool initialize();
void setCodecPriorities(VideoDecoderOptions option);
void attachAppSink(QObject *receiver, void *sink, QQuickItem *widget);
void bindDebugLevelFact(Fact *fact, QObject *context);
void onMainWindowReady(QQuickWindow *window);
#else
inline void prepareEnvironment() {}
inline bool initialize() { return false; }
inline void setCodecPriorities(VideoDecoderOptions) {}
inline void attachAppSink(QObject *, void *, QQuickItem *) {}
inline void bindDebugLevelFact(Fact *, QObject *) {}
inline void onMainWindowReady(QQuickWindow *) {}
#endif

}
