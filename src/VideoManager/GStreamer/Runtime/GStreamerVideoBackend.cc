#include "GStreamerVideoBackend.h"

#include "GStreamer.h"
#include "GStreamerDecoderRanks.h"
#include "HwBuffers/common/HwBuffers.h"

#ifdef Q_OS_ANDROID
#include <jni.h>

// GStreamer's Android plugins (e.g. androidmedia) look up the JNI state in libgstreamer_android.so, which keeps what
// this sets (Platform/gst_android_jni.c).
extern "C" void qgc_gst_android_set_jni_state(JavaVM* javaVm, jobject context, jobject classLoader);
#endif

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
    return {.streaming = true, .pipelineSettings = true, .gpuVideoPath = gpuVideoPath, .asyncInit = true};
}

VideoReceiver* GStreamerVideoBackend::createReceiver(QObject* parent)
{
    return GStreamer::createVideoReceiver(parent);
}

VideoBackend::EnvPrepResult GStreamerVideoBackend::prepareEnvironment()
{
    const GStreamer::Environment::ValidationResult result = GStreamer::prepareEnvironment();
    return {result.ok, result.error};
}

bool GStreamerVideoBackend::initialize(const InitParams& params)
{
    return GStreamer::initialize(params.arguments, {params.environment.ok, params.environment.error},
                                 params.debugLevel);
}

bool GStreamerVideoBackend::selfTest()
{
    return GStreamer::selfTest();
}

void GStreamerVideoBackend::applyDecoderPriorities(int rawOption)
{
    GStreamer::setCodecPriorities(rawOption);
}

QList<VideoDecoder::Options> GStreamerVideoBackend::availableDecoderFamilies()
{
    return GStreamer::availableDecoderFamilies();
}

void GStreamerVideoBackend::setDebugLevel(int level)
{
    GStreamer::setDebugLevel(level);
}

void GStreamerVideoBackend::onMainWindowReady(QQuickWindow* window)
{
    GStreamer::onMainWindowReady(window);
}

void GStreamerVideoBackend::setAndroidJniState([[maybe_unused]] void* javaVm, [[maybe_unused]] void* appContext,
                                               [[maybe_unused]] void* classLoader)
{
#ifdef Q_OS_ANDROID
    qgc_gst_android_set_jni_state(static_cast<JavaVM*>(javaVm), static_cast<jobject>(appContext),
                                  static_cast<jobject>(classLoader));
#endif
}
