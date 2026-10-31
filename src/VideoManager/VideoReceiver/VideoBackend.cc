#include "VideoBackend.h"

#include "AppMessages.h"
#include "QGCApplication.h"

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#endif

VideoReceiver* VideoBackend::createReceiver(QObject* parent)
{
#ifdef QGC_GST_STREAMING
    return GStreamer::createVideoReceiver(parent);
#else
    Q_UNUSED(parent);
    return nullptr;
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
    return {r.ok, r.error};
#else
    return {};
#endif
}

bool VideoBackend::initialize(const QStringList& arguments, const EnvPrepResult& envResult)
{
#ifdef QGC_GST_STREAMING
    return GStreamer::initialize(arguments, {envResult.ok, envResult.error});
#else
    Q_UNUSED(arguments);
    Q_UNUSED(envResult);
    return true;
#endif
}

bool VideoBackend::selfTest()
{
#ifdef QGC_GST_STREAMING
    return GStreamer::selfTest();
#else
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

void VideoBackend::onMainWindowReady(QQuickWindow* window)
{
#ifdef QGC_GST_STREAMING
    GStreamer::onMainWindowReady(window);
#else
    Q_UNUSED(window);
#endif
}

void VideoBackend::bindDebugLevelFact(Fact* fact, QObject* context)
{
#ifdef QGC_GST_STREAMING
    GStreamer::bindDebugLevelFact(fact, context);
#else
    Q_UNUSED(fact);
    Q_UNUSED(context);
#endif
}
