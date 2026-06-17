#include "VideoBackend.h"

#include "AppMessages.h"
#include "QGCApplication.h"

#include <QtCore/QThread>

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "SettingsManager.h"
#include "VideoSettings.h"
#include "Fact.h"
#else
#include "QtMultimediaReceiver.h"
#endif

VideoReceiver *VideoBackend::createReceiver(QObject *parent)
{
#ifdef QGC_GST_STREAMING
    return GStreamer::createVideoReceiver(parent);
#else
    return QtMultimediaReceiver::createVideoReceiver(parent);
#endif
}

void *VideoBackend::createSink(QQuickItem *widget, QObject *parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
#ifdef QGC_GST_STREAMING
    Q_UNUSED(widget);
    Q_UNUSED(parent);
    // Resolve construct-only sink tunables from settings here (the layer that owns the
    // settings singleton) so the GStreamer facade takes config as an argument.
    VideoSettings *const vs = SettingsManager::instance()->videoSettings();
    GStreamer::VideoSinkConfig config;
    config.conversionElement = vs->videoConversionElement()->rawValue().toString().toUtf8();
    config.disablePixelAspectRatio = vs->disablePixelAspectRatio()->rawValue().toBool();
    const bool forceCpu = vs->forceCpuVideoPath()->rawValue().toBool();
    const bool swDecoder = vs->forceVideoDecoder()->rawValue().toInt() == GStreamer::ForceVideoDecoderSoftware;
    config.gpuZeroCopy = !forceCpu && !swDecoder;
    return GStreamer::createVideoSink(config);
#else
    return QtMultimediaReceiver::createVideoSink(widget, parent);
#endif
}

void VideoBackend::releaseSink(void *sink)
{
#ifdef QGC_GST_STREAMING
    GStreamer::releaseVideoSink(sink);
#else
    QtMultimediaReceiver::releaseVideoSink(sink);
#endif
}

bool VideoBackend::disabledForUnitTests()
{
    return qgcApp() && QGC::runningUnitTests() && !qEnvironmentVariableIsSet("QGC_TEST_ENABLE_GSTREAMER");
}

VideoBackend::EnvPrepResult VideoBackend::prepareEnvironment()
{
#ifdef QGC_GST_STREAMING
    const GStreamer::Environment::ValidationResult r = GStreamer::prepareEnvironment();
    return { r.ok, r.error };
#else
    return {};
#endif
}

bool VideoBackend::initialize(const QStringList &arguments, const EnvPrepResult &envResult)
{
#ifdef QGC_GST_STREAMING
    return GStreamer::initialize(arguments, { envResult.ok, envResult.error });
#else
    Q_UNUSED(arguments);
    Q_UNUSED(envResult);
    return true;
#endif
}

void VideoBackend::applyDecoderPriorities(int rawOption)
{
#ifdef QGC_GST_STREAMING
    GStreamer::setCodecPriorities(rawOption);
#else
    Q_UNUSED(rawOption);
#endif
}

void VideoBackend::onMainWindowReady(QQuickWindow *window)
{
#ifdef QGC_GST_STREAMING
    GStreamer::onMainWindowReady(window);
#else
    Q_UNUSED(window);
#endif
}

void VideoBackend::bindDebugLevelFact(Fact *fact, QObject *context)
{
#ifdef QGC_GST_STREAMING
    GStreamer::bindDebugLevelFact(fact, context);
#else
    Q_UNUSED(fact);
    Q_UNUSED(context);
#endif
}

void VideoBackend::attachSink(QObject *receiver, void *sink, QQuickItem *widget)
{
#ifdef QGC_GST_STREAMING
    GStreamer::attachAppSink(receiver, sink, widget);
#else
    Q_UNUSED(receiver);
    Q_UNUSED(sink);
    Q_UNUSED(widget);
#endif
}
