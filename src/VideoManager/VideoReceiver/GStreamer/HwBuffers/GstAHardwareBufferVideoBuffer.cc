#include "GstAHardwareBufferVideoBuffer.h"

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include "QGCLoggingCategory.h"

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSize>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include <gst/android/gstandroid.h>
#include <gst/video/video.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>

#include <android/hardware_buffer.h>

#include <array>
#include <atomic>
#include <limits>

QGC_LOGGING_CATEGORY(GstAHWBufLog, "Video.GStreamer.HwBuffers.GstAHWBuf")

namespace {

constexpr int kMaxPlanes = 4;

std::atomic<quint64> s_mapFailureCount{0};
std::atomic<bool> s_loggedBadBackend{false};

// EGL_ANDROID_image_native_buffer presence cache — one query per EGLDisplay.
QMutex s_extCacheMutex;
QHash<EGLDisplay, bool> s_extCache;

bool queryHasNativeBufferExt(EGLDisplay display)
{
    QMutexLocker lock(&s_extCacheMutex);
    auto it = s_extCache.find(display);
    if (it != s_extCache.end()) {
        return it.value();
    }
    const char *exts = eglQueryString(display, EGL_EXTENSIONS);
    const bool supported = exts != nullptr
        && strstr(exts, "EGL_ANDROID_image_native_buffer") != nullptr;
    s_extCache.insert(display, supported);
    return supported;
}

QVideoFrameTexturesUPtr fail()
{
    s_mapFailureCount.fetch_add(1, std::memory_order_relaxed);
    return {};
}

class FrameTextures final : public QVideoFrameTextures
{
public:
    // AHardwareBuffer is always single-plane external-OES; pixelFormat must be Format_SamplerExternalOES.
    FrameTextures(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, GLuint name)
        : _rhi(rhi)
        , _name(name)
    {
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            qCWarning(GstAHWBufLog) << "no QVideoTextureHelper description for format" << pixelFormat;
            return;
        }
        const QSize planeSize = desc->rhiPlaneSize(size, 0, rhi);
        // ExternalOES: bind GL_TEXTURE_EXTERNAL_OES + emit SamplerExternalOES — without it OES_EGL_image_external samples black on GLES2.
        _texture.reset(rhi->newTexture(
            desc->rhiTextureFormat(0, rhi, QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable),
            planeSize, 1, QRhiTexture::ExternalOES));
        if (_texture && !_texture->createFrom({_name, 0})) {
            qCWarning(GstAHWBufLog) << "QRhiTexture::createFrom failed for AHardwareBuffer plane 0";
            _texture.reset();
        }
    }

    ~FrameTextures() override
    {
        releaseGLTextures();
    }

    void onFrameEndInvoked() override
    {
        releaseGLTextures();
    }

    QRhiTexture *texture(uint plane) const override
    {
        return plane == 0 ? _texture.get() : nullptr;
    }

private:
    void releaseGLTextures()
    {
        if (_released || !_rhi) return;
        _released = true;
        _rhi->makeThreadLocalNativeContextCurrent();
        if (auto *ctx = QOpenGLContext::currentContext()) {
            ctx->functions()->glDeleteTextures(1, &_name);
        }
    }

    QRhi *_rhi = nullptr;
    GLuint _name = 0;
    bool _released = false;
    std::unique_ptr<QRhiTexture> _texture;
};

} // namespace

GstAHardwareBufferVideoBuffer::GstAHardwareBufferVideoBuffer(GstSample *sample,
                                                             const GstVideoInfo &videoInfo,
                                                             const QVideoFrameFormat &format,
                                                             EGLDisplay eglDisplay)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
    , _eglDisplay(eglDisplay)
{
}

GstAHardwareBufferVideoBuffer::~GstAHardwareBufferVideoBuffer() = default;

QAbstractVideoBuffer::MapData GstAHardwareBufferVideoBuffer::map(QVideoFrame::MapMode /*mode*/)
{
    return {};
}

bool GstAHardwareBufferVideoBuffer::validatePlaneHandles() const
{
    if (!_sample) return false;
    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) return false;
    const int memCount = qMin(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    if (memCount <= 0) return false;
    for (int i = 0; i < memCount; ++i) {
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_ahardware_buffer_memory(mem)) return false;
        if (!gst_ahardware_buffer_memory_get_buffer(GST_AHARDWARE_BUFFER_MEMORY_CAST(mem))) {
            return false;
        }
    }
    return true;
}

QVideoFrameTexturesUPtr GstAHardwareBufferVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr & /*old*/)
{
    Q_ASSERT(rhi.thread()->isCurrentThread()); // Qt's contract: mapTextures runs on the QRhi (render) thread.
    // TODO(android-test): needs on-device fixture; no Android CI rig for the test harness here.

    if (!_sample || _eglDisplay == EGL_NO_DISPLAY) {
        return fail();
    }
    if (rhi.backend() != QRhi::OpenGLES2) {
        if (!s_loggedBadBackend.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstAHWBufLog) << "QRhi backend is not OpenGLES2; AHardwareBuffer path unsupported";
        }
        return fail();
    }

    // Bind Qt's GL context on the render thread before any EGL/GL call — without this glBindTexture / glEGLImageTargetTexture2DOES silently no-op into a foreign or null context.
    rhi.makeThreadLocalNativeContextCurrent();

    // Prefer Qt's actual EGLDisplay over the constructor-passed handle; eglGetDisplay(EGL_DEFAULT_DISPLAY) at construction time can resolve to a different EGLDisplay than the one Qt's context is bound to, which makes EGLImage import succeed but sampling return black.
    EGLDisplay eglDpy = eglGetCurrentDisplay();
    if (eglDpy == EGL_NO_DISPLAY) {
        eglDpy = _eglDisplay;
    }

    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) return fail();

    GstMemory *mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0 || !gst_is_ahardware_buffer_memory(mem0)) {
        return fail();
    }

    if (!queryHasNativeBufferExt(eglDpy)) {
        static bool s_warned = false;
        if (!s_warned) {
            s_warned = true;
            qCWarning(GstAHWBufLog) << "EGL_ANDROID_image_native_buffer unavailable; AHardwareBuffer path disabled";
        }
        return fail();
    }

    static const auto eglGetNativeClientBufferANDROID_ =
        reinterpret_cast<PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC>(
            eglGetProcAddress("eglGetNativeClientBufferANDROID"));
    static const auto eglCreateImageKHR_ =
        reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
            eglGetProcAddress("eglCreateImageKHR"));
    static const auto eglDestroyImageKHR_ =
        reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
            eglGetProcAddress("eglDestroyImageKHR"));
    static const auto glEGLImageTargetTexture2DOES_ =
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
            eglGetProcAddress("glEGLImageTargetTexture2DOES"));

    if (!eglGetNativeClientBufferANDROID_ || !eglCreateImageKHR_
            || !eglDestroyImageKHR_ || !glEGLImageTargetTexture2DOES_) {
        qCWarning(GstAHWBufLog) << "Required EGL/GL proc addresses unavailable";
        return fail();
    }

    const auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi.nativeHandles());
    if (!nativeHandles || !nativeHandles->context) {
        qCWarning(GstAHWBufLog) << "QRhi exposes no GL context";
        return fail();
    }

    AHardwareBuffer *ahwb = gst_ahardware_buffer_memory_get_buffer(
        GST_AHARDWARE_BUFFER_MEMORY_CAST(mem0));
    if (!ahwb) {
        qCWarning(GstAHWBufLog) << "gst_ahardware_buffer_memory_get_buffer returned null";
        return fail();
    }

    EGLClientBuffer clientBuffer = eglGetNativeClientBufferANDROID_(ahwb);
    if (!clientBuffer) {
        qCWarning(GstAHWBufLog) << "eglGetNativeClientBufferANDROID returned null";
        return fail();
    }

    const EGLint attribs[] = { EGL_NONE };
    EGLImageKHR image = eglCreateImageKHR_(eglDpy, EGL_NO_CONTEXT,
                                           EGL_NATIVE_BUFFER_ANDROID,
                                           clientBuffer, attribs);
    if (image == EGL_NO_IMAGE_KHR) {
        qCWarning(GstAHWBufLog) << "eglCreateImageKHR failed, err=" << Qt::hex << eglGetError();
        return fail();
    }

    // AHardwareBuffer is single-plane; GL_TEXTURE_EXTERNAL_OES requires Format_SamplerExternalOES.
    GLuint name = 0;
    QOpenGLFunctions functions(nativeHandles->context);
    functions.glGenTextures(1, &name);
    functions.glBindTexture(GL_TEXTURE_EXTERNAL_OES, name);
    glEGLImageTargetTexture2DOES_(GL_TEXTURE_EXTERNAL_OES, image);
    eglDestroyImageKHR_(eglDpy, image);

    auto textures = std::make_unique<FrameTextures>(&rhi, _format.frameSize(),
                                                    QVideoFrameFormat::Format_SamplerExternalOES, name);
    if (!textures->texture(0)) {
        qCWarning(GstAHWBufLog) << "createFrom failed for plane 0 (SamplerExternalOES)";
        functions.glDeleteTextures(1, &name);
        return fail();
    }
    return textures;
}

quint64 GstAHardwareBufferVideoBuffer::takeMapFailureCount()
{
    return s_mapFailureCount.exchange(0, std::memory_order_relaxed);
}

quint64 GstAHardwareBufferVideoBuffer::peekMapFailureCount()
{
    return s_mapFailureCount.load(std::memory_order_relaxed);
}

#endif // QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
