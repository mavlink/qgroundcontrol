#include "GStreamerVideoBackend.h"

#include <QtCore/QCoreApplication>

#include "GStreamer.h"
#include "GStreamerDecoderRanks.h"
#include "GStreamerEnvironment.h"
#include "GStreamerSelfTest.h"
#include "GstVideoReceiver.h"
#include "HwBuffers/HwBuffers.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GStreamerVideoBackendLog, "Video.GStreamer.VideoBackend")

void GStreamerVideoBackend::configureQtGraphics(bool softwareRendering)
{
    HwBuffers::configureQtGraphics(softwareRendering);
}

VideoBackend::Capabilities GStreamerVideoBackend::capabilities() const
{
#ifdef QGC_HAS_ANY_GPU_PATH
    constexpr bool gpuVideoPath = true;
#else
    constexpr bool gpuVideoPath = false;
#endif
    return {.pipelineSettings = true, .gpuVideoPath = gpuVideoPath, .asyncInit = true};
}

VideoReceiver* GStreamerVideoBackend::createOwnReceiver(QObject* parent)
{
    // GStreamer's setup isn't retried, so nothing plays through it in this process any more.
    if (state() == State::Failed) {
        return nullptr;
    }
    return new GstVideoReceiver(parent);
}

VideoBackend::EnvPrepResult GStreamerVideoBackend::prepareEnvironment()
{
    if (state() != State::Unprepared) {
        qCWarning(GStreamerVideoBackendLog) << "Environment prepared again after GStreamer setup started";
    }
    EnvPrepResult result = GStreamer::Environment::prepareEnvironment();
    State expected = State::Unprepared;
    (void) _state.compare_exchange_strong(expected, State::Prepared, std::memory_order_acq_rel);
    return result;
}

bool GStreamerVideoBackend::initialize(const InitParams& params)
{
    switch (state()) {
        case State::Initialized:
            return true;
        case State::Failed:
            return false;
        case State::Unprepared:
            qCWarning(GStreamerVideoBackendLog) << "Initializing GStreamer without preparing its environment";
            break;
        case State::Prepared:
            break;
    }
    const bool initialized = GStreamer::initialize(params);
    _state.store(initialized ? State::Initialized : State::Failed, std::memory_order_release);
    return initialized;
}

QString GStreamerVideoBackend::initFailureMessage() const
{
    return QCoreApplication::translate("GStreamerVideoBackend",
                                       "GStreamer failed to initialize, so video streams are played by Qt Multimedia "
                                       "instead. The application log has the details.");
}

bool GStreamerVideoBackend::selfTest()
{
    if (state() != State::Initialized) {
        qCWarning(GStreamerVideoBackendLog) << "GStreamer self-test skipped: GStreamer isn't initialized";
        return false;
    }
    return GStreamer::SelfTest::run(GStreamer::Environment::bundledRuntime());
}

void GStreamerVideoBackend::applyDecoderPriorities(VideoDecoderOption option)
{
    GStreamer::setCodecPriorities(option);
}

QList<VideoDecoderOption> GStreamerVideoBackend::availableDecoderFamilies()
{
    return GStreamer::availableDecoderFamilies();
}

void GStreamerVideoBackend::setDebugLevel(int level)
{
    GStreamer::setDebugLevel(level);
}

void GStreamerVideoBackend::onMainWindowReady(QQuickWindow* window)
{
    // Which decoders rank first depends on the device Qt renders with: its API, its adapter and what it opens. The
    // decoders wait for it. The render thread publishes it; the ranks and the receivers are the GUI thread's.
    HwBuffers::setGraphicsDevicePublishedHandler([] {
        if (QCoreApplication* const app = QCoreApplication::instance()) {
            (void) QMetaObject::invokeMethod(
                app,
                [] {
                    GStreamer::DecoderRankApplier::instance().reapply();
                    GstVideoReceiver::onGraphicsDeviceSettled();
                },
                Qt::QueuedConnection);
        }
    });
    GStreamer::onMainWindowReady(window);
}
