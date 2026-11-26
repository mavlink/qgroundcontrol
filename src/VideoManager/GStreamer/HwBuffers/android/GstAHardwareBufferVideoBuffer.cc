#include "GstAHardwareBufferVideoBuffer.h"

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include <android/hardware_buffer.h>
#include <atomic>
#include <gst/android/gstandroid.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/video/video.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>
#include <span>

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

#include "GstEglHelpers.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

QGC_LOGGING_CATEGORY(GstAHWBufLog, "Video.GStreamer.HwBuffers.GstAHWBuf")

namespace {

constexpr const char* kNativeBufferExt = "EGL_ANDROID_image_native_buffer";

/// Imports the driver refused in a row that disable the path: a stream it can't import fails every frame.
constexpr int kMaxConsecutiveImportFailures = 30;

// Set when the import can't work until the scene graph's device changes: the EGL display or the GL implementation
// lacks what it needs, or the driver refused frame after frame. A frame of this path is external-OES, without a CPU
// layout Qt could show instead, so admitAhb() then keeps frames off the path. Cleared by rearmImport().
std::atomic<bool> s_importDisabled{false};
std::atomic<int> s_consecutiveImportFailures{0};

void disableImport(const char* why)
{
    if (!s_importDisabled.exchange(true, std::memory_order_relaxed)) {
        qCWarning(GstAHWBufLog) << "AHardwareBuffer import disabled:" << why;
    }
}

void noteImportRefused()
{
    if ((s_consecutiveImportFailures.fetch_add(1, std::memory_order_relaxed) + 1) >= kMaxConsecutiveImportFailures) {
        disableImport("the driver refused frame after frame");
    }
}

void rearmImport()
{
    s_importDisabled.store(false, std::memory_order_relaxed);
    s_consecutiveImportFailures.store(0, std::memory_order_relaxed);
}

/// One frame's import, which the bundle owns: an EGLImage of the frame's AHardwareBuffer and the external-OES texture
/// bound to it. The frame's sample, which the bundle holds too, keeps the buffer.
class FrameTextures final : public GstHwFrameTexturesBase
{
public:
    FrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, EGLDisplay display,
                  EGLImageKHR image, GLuint name)
        : _display(display)
        , _image(image)
        , _name(name)
    {
        // ExternalOES: bind GL_TEXTURE_EXTERNAL_OES + emit SamplerExternalOES, else OES_EGL_image_external samples
        // black on GLES2.
        wrapPlanes(rhi, size, pixelFormat, std::span<const GLuint>(&name, 1), 0, QRhiTexture::ExternalOES);
    }

    ~FrameTextures() override
    {
        // Qt destroys the bundle on the QRhi's render thread, whose context the texture name belongs to.
        if (_rhi) {
            _rhi->makeThreadLocalNativeContextCurrent();
        }
        if (QOpenGLContext* context = QOpenGLContext::currentContext()) {
            context->functions()->glDeleteTextures(1, &_name);
        }
        if (const auto destroyImage = GstEglHelpers::destroyImageKHR()) {
            destroyImage(_display, _image);
        }
    }

private:
    const EGLDisplay _display;
    const EGLImageKHR _image;
    const GLuint _name;
};

// MediaCodec writes the AHB asynchronously; without a producer-side wait the EGLImage import can sample a half-written,
// recycled buffer (tearing). MediaCodec AHB buffers carry no GstGLSyncMeta and the gstandroid memory API exposes no
// acquire fence, so in practice no producer wait happens here; a sync meta is only present when a gst-gl element
// produced the buffer. A consumer-side EGL fence cannot observe MediaCodec's write, so none is used.
void waitProducerComplete(GstBuffer* buffer)
{
    if (GstGLSyncMeta* syncMeta = buffer ? gst_buffer_get_gl_sync_meta(buffer) : nullptr) {
        if (GstGLContext* glObj = syncMeta->context) {
            // CPU wait: gst_gl_sync_meta_wait() would only order the producer's own context, not Qt's.
            gst_gl_sync_meta_wait_cpu(syncMeta, glObj);
            return;
        }
    }

    QGC_HW_WARN_ONCE(GstAHWBufLog,
                     "AHardwareBuffer: no GstGLSyncMeta producer fence; frames may tear on recycle (no consumer-side "
                     "barrier can substitute)");
}

}  // namespace

GstAHardwareBufferVideoBuffer::GstAHardwareBufferVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                                             const QVideoFrameFormat& format, EGLDisplay eglDisplay)
    : GstHwVideoBuffer(sample, videoInfo, format)
    , _eglDisplay(eglDisplay)
{}

// The AHB is imported directly as an EGLImage (no SurfaceTexture::updateTexImage()), so no per-buffer
// getTransformMatrix() exists here; external-OES sampling of a directly-imported buffer is top-left origin while GL
// texcoords are bottom-left, so apply the static Y-flip Qt's SurfaceTexture path also applies (flipV, row-major).
QMatrix4x4 GstAHardwareBufferVideoBuffer::externalTextureMatrix() const
{
    // GStreamer exposes no per-buffer decoder transform at this layer, so we apply the static external-OES Y-flip
    // (top-left vs GL bottom-left origin). If a device ever needs crop-inset/rotation, source it from GstVideoCropMeta.
    static const QMatrix4x4 flipV(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 1.0f);
    return flipV;
}

bool GstAHardwareBufferVideoBuffer::validatePlaneHandles() const
{
    // Qt 6.10-6.12 has the right ExternalOES rendering path, but upstream GStreamer still does not expose this
    // AHardwareBuffer memory API. If a future GStreamer/Qt release grows an official API, replace these vendor hooks.
    return validatePlanes([](GstMemory* mem) {
        if (!mem || !gst_is_ahardware_buffer_memory(mem))
            return false;
        return gst_ahardware_buffer_memory_get_buffer(GST_AHARDWARE_BUFFER_MEMORY_CAST(mem)) != nullptr;
    });
}

QVideoFrameTexturesUPtr GstAHardwareBufferVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::OpenGLES2), GstAHWBufLog(), buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    QOpenGLContext* glContext = GstEglHelpers::bindRhiContext(rhi);
    if (!glContext) {
        QGC_HW_WARN_ONCE(GstAHWBufLog, "QRhi's GL context can't be made current; frame not shown");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }
    // Sampling an EGLImage created on a foreign display silently returns black.
    const EGLDisplay eglDpy = GstEglHelpers::renderDisplay(glContext, _eglDisplay);
    if (eglDpy == EGL_NO_DISPLAY) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0 || !gst_is_ahardware_buffer_memory(mem0)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    if (!GstEglHelpers::displaySupportsExtension(eglDpy, kNativeBufferExt)) {
        disableImport("EGL_ANDROID_image_native_buffer unavailable");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    static const auto eglGetNativeClientBufferANDROID_ =
        reinterpret_cast<PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC>(eglGetProcAddress("eglGetNativeClientBufferANDROID"));
    static const auto eglCreateImageKHR_ =
        reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    const auto eglDestroyImageKHR_ = GstEglHelpers::destroyImageKHR();
    const auto glEGLImageTargetTexture2DOES_ = GstEglHelpers::imageTargetTexture2D();

    if (!eglGetNativeClientBufferANDROID_ || !eglCreateImageKHR_ || !eglDestroyImageKHR_ ||
        !glEGLImageTargetTexture2DOES_) {
        disableImport("required EGL/GL functions unavailable");
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    AHardwareBuffer* ahwb = gst_ahardware_buffer_memory_get_buffer(GST_AHARDWARE_BUFFER_MEMORY_CAST(mem0));
    if (!ahwb) {
        qCWarning(GstAHWBufLog) << "gst_ahardware_buffer_memory_get_buffer returned null";
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    // Block until the producer's GPU write completes before importing/sampling the recycled AHB.
    waitProducerComplete(buffer);

    // Before the import: a frame Qt can't wrap must not cost one.
    const QSize frameSize = _format.frameSize();
    if (!GstHwImportPreflight::preflightOrRecord(rhi, HwVideoBufferPath::AHardwareBuffer,
                                                 QVideoFrameFormat::Format_SamplerExternalOES, frameSize,
                                                 QRhiTexture::ExternalOES)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    EGLClientBuffer clientBuffer = eglGetNativeClientBufferANDROID_(ahwb);
    if (!clientBuffer) {
        QGC_HW_WARN_ONCE(GstAHWBufLog, "eglGetNativeClientBufferANDROID returned null; frame not shown");
        noteImportRefused();
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    const EGLint attribs[] = {EGL_NONE};
    const EGLImageKHR image =
        eglCreateImageKHR_(eglDpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, attribs);
    if (image == EGL_NO_IMAGE_KHR) {
        QGC_HW_WARN_ONCE(GstAHWBufLog,
                         "eglCreateImageKHR failed, err=" << Qt::hex << eglGetError() << "; frame not shown");
        noteImportRefused();
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }

    QOpenGLFunctions functions(glContext);
    // Errors still pending from earlier GL calls would read as this import failing. Bounded: a lost context
    // reports an error forever.
    for (int i = 0; (i < 8) && (functions.glGetError() != GL_NO_ERROR); ++i) {
    }
    GLuint name = 0;
    functions.glGenTextures(1, &name);
    functions.glBindTexture(GL_TEXTURE_EXTERNAL_OES, name);
    glEGLImageTargetTexture2DOES_(GL_TEXTURE_EXTERNAL_OES, image);
    // A failed bind leaves an empty texture, which would show black.
    if (const GLenum glErr = functions.glGetError(); glErr != GL_NO_ERROR) {
        QGC_HW_WARN_ONCE(GstAHWBufLog, "glEGLImageTargetTexture2DOES failed, glError="
                                           << Qt::hex << glErr << "eglError=" << eglGetError() << "; frame not shown");
        functions.glDeleteTextures(1, &name);
        eglDestroyImageKHR_(eglDpy, image);
        noteImportRefused();
        return GstHwPathTelemetry::fail(HwVideoBufferPath::AHardwareBuffer);
    }
    s_consecutiveImportFailures.store(0, std::memory_order_relaxed);

    const GstHwMapTail tail{.path = HwVideoBufferPath::AHardwareBuffer,
                            .frameSize = frameSize,
                            .pixelFormat = QVideoFrameFormat::Format_SamplerExternalOES,
                            .textureFlags = QRhiTexture::ExternalOES,
                            .log = &GstAHWBufLog(),
                            .preflight = false};
    // No reuse: each frame has an import of its own, which its bundle takes over here and releases.
    return GstHwFrameTexturesBase::finishMap<FrameTextures>(*this, rhi, old, tail, nullptr, [&] {
        return std::make_unique<FrameTextures>(&rhi, tail.frameSize, tail.pixelFormat, eglDpy, image, name);
    });
}

namespace {

// GL_TEXTURE_EXTERNAL_OES needs the SamplerExternalOES pixel format for Qt's shader path.
QVideoFrameFormat externalOesFormat(const QVideoFrameFormat& format)
{
    return GstHw::withPixelFormat(format, QVideoFrameFormat::Format_SamplerExternalOES);
}

void prepareAhbContext(HwVideoBufferContext& context)
{
    context.ahbEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (context.ahbEglDisplay == EGL_NO_DISPLAY) {
        qCWarning(GstAHWBufLog) << "AHardwareBuffer path: EGLDisplay unavailable";
    }
}

// Streaming thread, for every frame. A frame kept off the path goes to the sink's CPU path, which shows it only if
// its memory maps on the CPU; wrapped, a frame that then fails to import isn't shown at all.
bool admitAhb(GstMemory*, const HwVideoBufferContext& context)
{
    using GstHwPathTelemetry::HwFallbackReason;
    // The import binds an external-OES texture in Qt's OpenGL context: nothing to import into under another QRhi.
    const int backend = QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire);
    const bool otherRenderer = (backend >= 0) && (backend != static_cast<int>(QRhi::OpenGLES2));
    if ((context.ahbEglDisplay != EGL_NO_DISPLAY) && !otherRenderer &&
        !s_importDisabled.load(std::memory_order_relaxed)) {
        return true;
    }
    GstHwPathTelemetry::recordFallbackReason(
        HwVideoBufferPath::AHardwareBuffer,
        (context.ahbEglDisplay == EGL_NO_DISPLAY) ? HwFallbackReason::NoExt : HwFallbackReason::ImportUnsupported);
    return false;
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kAHardwareBuffer = {
    .path = HwVideoBufferPath::AHardwareBuffer,
    .matches = [](GstMemory* mem) -> bool { return gst_is_ahardware_buffer_memory(mem); },
    .admits = &admitAhb,
    .construct = [](GstSample* sample, const GstVideoInfo& info, QVideoFrameFormat format,
                    const HwVideoBufferContext& context) -> std::unique_ptr<GstHwVideoBuffer> {
        return std::make_unique<GstAHardwareBufferVideoBuffer>(sample, info, externalOesFormat(format),
                                                               context.ahbEglDisplay);
    },
    .prepareContext = &prepareAhbContext,
    .resetCaches =
        [] {
            GstEglHelpers::resetExtensionCache();
            rearmImport();
        },
};

#endif  // QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
