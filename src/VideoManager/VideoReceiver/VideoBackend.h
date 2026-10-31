#pragma once

#include <QtCore/QString>
#include <QtCore/qcontainerfwd.h>

class Fact;
class QObject;
class QQuickWindow;
class VideoReceiver;

/// Backend-neutral video backend facade: receiver creation plus process/runtime lifecycle.
/// Selects GStreamer at compile time via QGC_GST_STREAMING so callers (VideoManager, QGCCorePlugin) never name a
/// concrete backend. Without GStreamer there is no network video backend: createReceiver() returns nullptr.
namespace VideoBackend {

VideoReceiver* createReceiver(QObject* parent);

/// True when a streaming backend that requires asynchronous global init is compiled in (GStreamer).
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
struct EnvPrepResult
{
    bool ok = true;
    QString error;
};

EnvPrepResult prepareEnvironment();
bool initialize(const QStringList& arguments, const EnvPrepResult& envResult);
/// The boot test's check that the initialized backend decodes and records; true without GStreamer.
bool selfTest();
void applyDecoderPriorities(int rawOption);
void onMainWindowReady(QQuickWindow* window);
void bindDebugLevelFact(Fact* fact, QObject* context);

}  // namespace VideoBackend
