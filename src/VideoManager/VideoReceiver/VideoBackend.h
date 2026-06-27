#pragma once

#include <QtCore/QString>
#include <QtCore/qcontainerfwd.h>

class Fact;
class QObject;
class QQuickItem;
class QQuickWindow;
class VideoReceiver;

/// Backend-neutral video backend facade: object creation plus process/runtime lifecycle.
/// Selects GStreamer or QtMultimedia at compile time via QGC_GST_STREAMING so callers
/// (VideoManager, QGCCorePlugin) never name a concrete backend.
namespace VideoBackend
{

VideoReceiver *createReceiver(QObject *parent);
void *createSink(QQuickItem *widget, QObject *parent);
void releaseSink(void *sink);

/// Applies backend-wide sink policy before constructing the GStreamer sink.
bool gpuZeroCopyAllowedForCurrentGraphicsApi(bool forceCpuVideoPath, bool forceSoftwareDecoder);

/// True when a streaming backend that requires asynchronous global init is compiled in
/// (GStreamer). QtMultimedia needs no global init, so callers treat it as ready immediately.
constexpr bool needsAsyncInit() noexcept
{
#ifdef QGC_GST_STREAMING
    return true;
#else
    return false;
#endif
}

/// True when the backend should be skipped under unit tests (opt back in with QGC_TEST_ENABLE_GSTREAMER).
bool disabledForUnitTests();

/// Outcome of prepareEnvironment(), threaded into initialize() so backend setup
/// validity is passed as a value rather than stashed in process-global state.
struct EnvPrepResult {
    bool ok = true;
    QString error;
};

EnvPrepResult prepareEnvironment();
bool initialize(const QStringList &arguments, const EnvPrepResult &envResult);
void applyDecoderPriorities(int rawOption);
void onMainWindowReady(QQuickWindow *window);
void bindDebugLevelFact(Fact *fact, QObject *context);
void attachSink(QObject *receiver, void *sink, QQuickItem *widget);

} // namespace VideoBackend
