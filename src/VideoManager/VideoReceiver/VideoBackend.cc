#include "VideoBackend.h"

#include <QtQuick/QQuickWindow>

#include "AppMessages.h"
#include "QGCApplication.h"

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#endif

#ifdef QGC_GST_STREAMING
namespace {

bool d3d12ZeroCopyUnsupported()
{
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
    return QQuickWindow::graphicsApi() == QSGRendererInterface::Direct3D12 ||
           qEnvironmentVariable("QSG_RHI_BACKEND").compare(QLatin1String("d3d12"), Qt::CaseInsensitive) == 0;
#else
    return false;
#endif
}

}  // namespace
#endif

bool VideoBackend::gpuZeroCopyAllowedForCurrentGraphicsApi(bool forceCpuVideoPath, bool forceSoftwareDecoder)
{
#ifdef QGC_GST_STREAMING
    return !forceCpuVideoPath && !forceSoftwareDecoder && !d3d12ZeroCopyUnsupported();
#else
    Q_UNUSED(forceCpuVideoPath);
    Q_UNUSED(forceSoftwareDecoder);
    return false;
#endif
}

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
