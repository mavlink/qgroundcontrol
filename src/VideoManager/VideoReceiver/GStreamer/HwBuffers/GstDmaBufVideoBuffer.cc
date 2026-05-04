#include "GstDmaBufVideoBuffer.h"
#include "GstHwVideoBuffer.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include "QGCLoggingCategory.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSize>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video.h>
#if defined(QGC_GST_BUILD_VERSION_MAJOR) && \
    (QGC_GST_BUILD_VERSION_MAJOR > 1 || \
     (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR >= 24))
#include <gst/video/video-info-dma.h>
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

#include <array>
#include <atomic>
#include <cstring>

QGC_LOGGING_CATEGORY(GstDmaBufLog, "Video.GStreamer.HwBuffers.GstDmaBuf")

namespace {

constexpr int kMaxPlanes = 4;

// Process-wide failure tally; read+reset by GstDmaBufVideoBuffer::takeMapFailureCount.
std::atomic<quint64> s_mapFailureCount{0};
std::atomic<bool> s_loggedBadBackend{false};

QMutex s_modExtMutex;
QHash<EGLDisplay, bool> s_modExtCache;

bool queryHasModifiersExt(EGLDisplay display)
{
    QMutexLocker lock(&s_modExtMutex);
    auto it = s_modExtCache.find(display);
    if (it != s_modExtCache.end()) {
        return it.value();
    }
    // eglQueryString(display, EGL_EXTENSIONS) returns NULL on un-initialized displays.
    // eglInitialize is idempotent, so this is a safe defensive call when we got the display
    // from eglGetDisplay(EGL_DEFAULT_DISPLAY) and Qt happens to have init'd a *different* one.
    EGLint major = 0, minor = 0;
    eglInitialize(display, &major, &minor);
    const char *exts = eglQueryString(display, EGL_EXTENSIONS);
    const bool supported = exts != nullptr
        && std::strstr(exts, "EGL_EXT_image_dma_buf_import_modifiers") != nullptr;
    qCDebug(GstDmaBufLog) << "EGL display" << display
                          << "modifiers ext supported:" << supported
                          << " (driver:" << eglQueryString(display, EGL_VENDOR) << ")";
    s_modExtCache.insert(display, supported);
    return supported;
}

// DRM fourcc for a given GstVideoInfo plane. Modeled on Qt's
// fourccFromVideoInfo() in qgstvideobuffer.cpp (LGPL-3).
int drmFourccFor(const GstVideoInfo *info, int plane)
{
    const GstVideoFormat fmt = GST_VIDEO_INFO_FORMAT(info);
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    constexpr int rgbaFourcc = DRM_FORMAT_ABGR8888;
    constexpr int rgFourcc = DRM_FORMAT_GR88;
#else
    constexpr int rgbaFourcc = DRM_FORMAT_RGBA8888;
    constexpr int rgFourcc = DRM_FORMAT_RG88;
#endif

    switch (fmt) {
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_AYUV:
        return rgbaFourcc;
    case GST_VIDEO_FORMAT_GRAY8:
        return DRM_FORMAT_R8;
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_GRAY16_LE:
        return rgFourcc;
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
        return plane == 0 ? DRM_FORMAT_R8 : rgFourcc;
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
        return DRM_FORMAT_R8;
    case GST_VIDEO_FORMAT_P010_10LE:
        return plane == 0 ? DRM_FORMAT_R16 : DRM_FORMAT_GR1616;
    default:
        return -1;
    }
}

// DRM fourcc for formats that can be imported as a single EGLImage (all planes, one fd).
int drmFourccForSingleFd(const GstVideoInfo *info)
{
    switch (GST_VIDEO_INFO_FORMAT(info)) {
    case GST_VIDEO_FORMAT_NV12:       return DRM_FORMAT_NV12;
    case GST_VIDEO_FORMAT_NV21:       return DRM_FORMAT_NV21;
    case GST_VIDEO_FORMAT_P010_10LE:  return DRM_FORMAT_P010;
    case GST_VIDEO_FORMAT_I420:       return DRM_FORMAT_YUV420;
    case GST_VIDEO_FORMAT_YV12:       return DRM_FORMAT_YVU420;
    // packed single-memory formats: planeCount==1, memCount==1
    case GST_VIDEO_FORMAT_YUY2:       return DRM_FORMAT_YUYV;
    case GST_VIDEO_FORMAT_UYVY:       return DRM_FORMAT_UYVY;
#ifdef DRM_FORMAT_YVYU
    case GST_VIDEO_FORMAT_YVYU:       return DRM_FORMAT_YVYU;
#endif
#ifdef DRM_FORMAT_VYUY
    case GST_VIDEO_FORMAT_VYUY:       return DRM_FORMAT_VYUY;
#endif
    // AYUV excluded: EGL importers don't support DRM_FORMAT_AYUV; per-plane RGBA path handles it.
#ifdef DRM_FORMAT_Y210
    case GST_VIDEO_FORMAT_Y210:       return DRM_FORMAT_Y210;
#endif
#ifdef DRM_FORMAT_Y410
    case GST_VIDEO_FORMAT_Y410:       return DRM_FORMAT_Y410;
#endif
    default:                          return -1;
    }
}

// Per-plane EGL attrib keys — spec defines distinct enums per plane index.
static constexpr EGLint kPlFd[4] = {
    EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE1_FD_EXT,
    EGL_DMA_BUF_PLANE2_FD_EXT, EGL_DMA_BUF_PLANE3_FD_EXT,
};
static constexpr EGLint kPlOffset[4] = {
    EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT,
    EGL_DMA_BUF_PLANE2_OFFSET_EXT, EGL_DMA_BUF_PLANE3_OFFSET_EXT,
};
static constexpr EGLint kPlPitch[4] = {
    EGL_DMA_BUF_PLANE0_PITCH_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT,
    EGL_DMA_BUF_PLANE2_PITCH_EXT, EGL_DMA_BUF_PLANE3_PITCH_EXT,
};
static constexpr EGLint kPlModLo[4] = {
    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT,
};
static constexpr EGLint kPlModHi[4] = {
    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT, EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT,
};

class FrameTextures final : public QVideoFrameTextures
{
public:
    FrameTextures(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                  std::array<GLuint, kMaxPlanes> names, int count)
        : _rhi(rhi)
        , _names(names)
        , _count(count)
    {
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            qCWarning(GstDmaBufLog) << "no QVideoTextureHelper description for format" << pixelFormat;
            return;
        }
        for (int i = 0; i < _count; ++i) {
            const QSize planeSize = desc->rhiPlaneSize(size, i, rhi);
            _textures[i].reset(rhi->newTexture(
                desc->rhiTextureFormat(i, rhi, QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable),
                planeSize, 1, {}));
            if (_textures[i] && !_textures[i]->createFrom({_names[i], 0})) {
                qCWarning(GstDmaBufLog) << "QRhiTexture::createFrom failed for plane" << i;
                _textures[i].reset();
            }
        }
    }

    ~FrameTextures() override
    {
        releaseGLTextures(); // safety net if onFrameEndInvoked() was never called
    }

    void onFrameEndInvoked() override
    {
        releaseGLTextures();
    }

    QRhiTexture *texture(uint plane) const override
    {
        return (int(plane) < _count) ? _textures[plane].get() : nullptr;
    }

private:
    void releaseGLTextures()
    {
        if (_released || !_rhi || _count == 0) return;
        _released = true;
        // makeThreadLocalNativeContextCurrent avoids crash when Qt drops frame off the render thread.
        _rhi->makeThreadLocalNativeContextCurrent();
        if (auto *ctx = QOpenGLContext::currentContext()) {
            ctx->functions()->glDeleteTextures(_count, _names.data());
        }
    }

    QRhi *_rhi = nullptr;
    std::array<GLuint, kMaxPlanes> _names{};
    int _count = 0;
    bool _released = false;
    std::unique_ptr<QRhiTexture> _textures[kMaxPlanes];
};

} // namespace

GstDmaBufVideoBuffer::GstDmaBufVideoBuffer(GstSample *sample,
                                           const GstVideoInfo &videoInfo,
                                           const QVideoFrameFormat &format,
                                           EGLDisplay eglDisplay)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
    , _eglDisplay(eglDisplay)
{
}

GstDmaBufVideoBuffer::~GstDmaBufVideoBuffer() = default;

QAbstractVideoBuffer::MapData GstDmaBufVideoBuffer::map(QVideoFrame::MapMode /*mode*/)
{
    // GPU-only buffer; CPU map intentionally unsupported. Qt will draw black if it
    // ever calls map() — that means mapTextures() failed, which is logged below.
    return {};
}

bool GstDmaBufVideoBuffer::validatePlaneHandles() const
{
    if (!_sample) return false;
    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) return false;
    const int memCount = qMin(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    if (memCount <= 0) return false;
    for (int i = 0; i < memCount; ++i) {
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_dmabuf_memory(mem)) return false;
        if (gst_dmabuf_memory_get_fd(mem) < 0) return false;
    }
    // Early non-LINEAR rejection — without this, Qt warns "Cannot map ReadOnly" and drops the frame.
#if defined(QGC_GST_BUILD_VERSION_MAJOR) && \
    (QGC_GST_BUILD_VERSION_MAJOR > 1 || \
     (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR >= 24))
    if (GstCaps *caps = gst_sample_get_caps(_sample); caps && gst_video_is_dma_drm_caps(caps)) {
        GstVideoInfoDmaDrm drmInfo{};
        gst_video_info_dma_drm_init(&drmInfo);
        if (gst_video_info_dma_drm_from_caps(&drmInfo, caps) && drmInfo.drm_modifier != 0) {
            return false;
        }
    }
#endif
    return true;
}

QVideoFrameTexturesUPtr GstDmaBufVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr & /*old*/)
{
    Q_ASSERT(rhi.thread()->isCurrentThread()); // Qt's contract: mapTextures runs on the QRhi (render) thread.

    auto fail = []() -> QVideoFrameTexturesUPtr {
        s_mapFailureCount.fetch_add(1, std::memory_order_relaxed);
        return {};
    };

    if (!_sample) {
        return fail();
    }

    // Parse modifier before the sync mmap — mmap assumes LINEAR; tiled strides fault libgstvideo.
    guint64 drmModifier = 0;
#if defined(QGC_GST_BUILD_VERSION_MAJOR) && \
    (QGC_GST_BUILD_VERSION_MAJOR > 1 || \
     (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR >= 24))
    if (GstCaps *caps = gst_sample_get_caps(_sample); caps && gst_video_is_dma_drm_caps(caps)) {
        GstVideoInfoDmaDrm drmInfo{};
        gst_video_info_dma_drm_init(&drmInfo);
        if (gst_video_info_dma_drm_from_caps(&drmInfo, caps)) {
            drmModifier = drmInfo.drm_modifier;
        }
    }
#endif

    // dmabuf mmap as a sync barrier — Intel iHD legacy LINEAR exporter has no implicit fence
    // and vaSyncSurface() is a no-op there. Tiled paths carry fences and aren't safe to mmap.
    if (drmModifier == 0 /* DRM_FORMAT_MOD_LINEAR / unknown */) {
        if (GstBuffer *syncBuf = gst_sample_get_buffer(_sample)) {
            GstVideoFrame syncFrame;
            if (gst_video_frame_map(&syncFrame, &_videoInfo, syncBuf, GST_MAP_READ)) {
                gst_video_frame_unmap(&syncFrame);
            }
        }
    }

    // Bind Qt's GL context on the render thread BEFORE any EGL/GL call. Without this,
    // glBindTexture / glEGLImageTargetTexture2DOES silently no-op into a foreign or
    // null context — symptom: import succeeds, texture samples empty (green).
    rhi.makeThreadLocalNativeContextCurrent();

    // Use Qt's actual EGLDisplay, not the adapter's eglGetDisplay(EGL_DEFAULT_DISPLAY) —
    // those can resolve to different EGLDisplay objects under xcb_egl, and EGLImages
    // imported in display A cannot be sampled by a context bound to display B.
    EGLDisplay eglDpy = eglGetCurrentDisplay();
    if (eglDpy == EGL_NO_DISPLAY) {
        eglDpy = _eglDisplay;
    }

    if (eglDpy == EGL_NO_DISPLAY) {
        return fail();
    }
    if (rhi.backend() != QRhi::OpenGLES2) {
        if (!s_loggedBadBackend.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstDmaBufLog) << "QRhi backend is not OpenGLES2; zero-copy DMABuf path unsupported";
        }
        return fail();
    }

    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer || !gst_is_dmabuf_memory(gst_buffer_peek_memory(buffer, 0))) {
        return fail();
    }
    const bool hasModifiersExt = queryHasModifiersExt(eglDpy);
    if (drmModifier != 0) {
        // Tiled+CCS (e.g. I915_FORMAT_MOD_Y_TILED_CCS) needs multi-plane EXTERNAL_OES import;
        // per-plane GL_TEXTURE_2D crashes the driver in glEGLImageTargetTexture2DOES.
        static std::atomic<bool> warned{false};
        if (!warned.exchange(true, std::memory_order_relaxed)) {
            qCWarning(GstDmaBufLog) << "DMABuf modifier" << Qt::hex << drmModifier
                                    << "is non-LINEAR; per-plane EGLImage import is unsafe"
                                       " on tiled/CCS layouts — falling back to CPU"
                                       " (modifiersExt=" << hasModifiersExt << ")";
        }
        return fail();
    }

    const auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi.nativeHandles());
    if (!nativeHandles || !nativeHandles->context) {
        qCWarning(GstDmaBufLog) << "QRhi exposes no GL context";
        return fail();
    }

    static const auto eglImageTargetTexture2D =
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
            eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!eglImageTargetTexture2D) {
        qCWarning(GstDmaBufLog) << "glEGLImageTargetTexture2DOES unavailable";
        return fail();
    }

    // gst_video_frame_map(GST_MAP_READ) on DMABuf triggers a CPU mmap defeating zero-copy; read offsets from GstVideoMeta directly.
    GstVideoMeta *vmeta = gst_buffer_get_video_meta(buffer);
    const int planeCount = qBound(1, int(GST_VIDEO_INFO_N_PLANES(&_videoInfo)), kMaxPlanes);
    const int memCount = int(gst_buffer_n_memory(buffer));
    // memCount==1: either multi-plane shared fd (NV12) or packed single-plane (YUY2/UYVY/…).
    const int cachedSingleFourcc = (memCount == 1) ? drmFourccForSingleFd(&_videoInfo) : -1;
    const bool singleFdPacking = (cachedSingleFourcc != -1);
    if (memCount != planeCount && !singleFdPacking) {
        return fail();
    }

    // No texture reuse: a previous attempt to cache QRhiTextures across frames
    // (keyed first on fd values, then on GstMemory pointers) caused periodic
    // green frames in real RTSP H.264 streams. The optimization is worth ~µs per
    // frame and not worth the lifetime hazards. Always rebuild.
    std::array<GLuint, kMaxPlanes> names{};
    QOpenGLFunctions functions(nativeHandles->context);
    functions.glGenTextures(planeCount, names.data());

    // Pass modifier attribs only when extension is available; otherwise the implementation
    // assumes LINEAR, which matches our earlier rejection of non-LINEAR without the ext.
    const EGLAttrib modLo = static_cast<EGLAttrib>(drmModifier & 0xFFFFFFFFu);
    const EGLAttrib modHi = static_cast<EGLAttrib>((drmModifier >> 32) & 0xFFFFFFFFu);

    bool ok = true;

    // Single-fd fast path: one EGLImage covering all planes, one eglCreateImage call.
    // Mesa VA-API NV12 default ships memCount==1; iHD rejects split-plane R8/GR88 imports.
    if (singleFdPacking) {
        if (planeCount > kMaxPlanes) {
            qCWarning(GstDmaBufLog) << "single-fd format has" << planeCount
                                    << "planes (>" << kMaxPlanes << "); falling back to per-plane";
            goto per_plane_path;
        }
        const int singleFourcc = cachedSingleFourcc;
        if (singleFourcc == -1) {
            goto per_plane_path;
        }
        const int fd0 = gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, 0));
        // maxAttr: WIDTH/HEIGHT/FOURCC + 3 attribs/plane * kMaxPlanes + 2 modifier attribs/plane * kMaxPlanes + EGL_NONE
        EGLAttrib attribs[3 + kMaxPlanes * 5 + 1];
        int n = 0;
#define ATTRIB_PUSH(k, v) do { Q_ASSERT(n < int(std::size(attribs)) - 1); attribs[n++] = (k); attribs[n++] = (v); } while (0)
        ATTRIB_PUSH(EGL_WIDTH,              GST_VIDEO_INFO_WIDTH(&_videoInfo));
        ATTRIB_PUSH(EGL_HEIGHT,             GST_VIDEO_INFO_HEIGHT(&_videoInfo));
        ATTRIB_PUSH(EGL_LINUX_DRM_FOURCC_EXT, singleFourcc);
        for (int p = 0; p < planeCount; ++p) {
            const auto off = vmeta ? vmeta->offset[p] : GST_VIDEO_INFO_PLANE_OFFSET(&_videoInfo, p);
            const auto pitch = vmeta ? vmeta->stride[p] : GST_VIDEO_INFO_PLANE_STRIDE(&_videoInfo, p);
            ATTRIB_PUSH(kPlFd[p],     fd0);
            ATTRIB_PUSH(kPlOffset[p], static_cast<EGLAttrib>(off));
            ATTRIB_PUSH(kPlPitch[p],  static_cast<EGLAttrib>(pitch));
            if (hasModifiersExt) {  // interleaved per spec: modifier attribs follow FD/OFFSET/PITCH for same plane
                ATTRIB_PUSH(kPlModLo[p], modLo);
                ATTRIB_PUSH(kPlModHi[p], modHi);
            }
        }
#undef ATTRIB_PUSH
        attribs[n++] = EGL_NONE;
        EGLImage singleImage = eglCreateImage(eglDpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
                                              nullptr, attribs);
        if (singleImage != EGL_NO_IMAGE_KHR) {
            for (int p = 0; p < planeCount; ++p) {
                functions.glBindTexture(GL_TEXTURE_2D, names[p]);
                eglImageTargetTexture2D(GL_TEXTURE_2D, singleImage);
            }
            eglDestroyImage(eglDpy, singleImage);
            goto textures_built;
        }
#if defined(DRM_FORMAT_Y210) || defined(DRM_FORMAT_Y410)
        // Y210/Y410 have no per-plane fallback; warn once so the user knows frames will be black.
        {
            static std::atomic<bool> s_warnedY210{false};
            static std::atomic<bool> s_warnedY410{false};
#if defined(DRM_FORMAT_Y210)
            if (singleFourcc == DRM_FORMAT_Y210 && !s_warnedY210.exchange(true, std::memory_order_relaxed))
                qCWarning(GstDmaBufLog) << "EGL DMABuf import of Y210 unsupported on this driver; frames will be black";
#endif
#if defined(DRM_FORMAT_Y410)
            if (singleFourcc == DRM_FORMAT_Y410 && !s_warnedY410.exchange(true, std::memory_order_relaxed))
                qCWarning(GstDmaBufLog) << "EGL DMABuf import of Y410 unsupported on this driver; frames will be black";
#endif
        }
#endif
        qCWarning(GstDmaBufLog) << "single-fd eglCreateImage failed (" << Qt::hex
                                << eglGetError() << "); falling back to per-plane";
    }

per_plane_path:
    for (int i = 0; i < planeCount && ok; ++i) {
        const int memIdx = singleFdPacking ? 0 : i;
        const int fd = gst_dmabuf_memory_get_fd(gst_buffer_peek_memory(buffer, memIdx));
        const auto offset = vmeta ? vmeta->offset[i] : GST_VIDEO_INFO_PLANE_OFFSET(&_videoInfo, i);
        const auto stride = vmeta ? vmeta->stride[i] : GST_VIDEO_INFO_PLANE_STRIDE(&_videoInfo, i);
        Q_ASSERT(stride > 0);
        Q_ASSERT(static_cast<quint64>(offset) <= static_cast<quint64>(std::numeric_limits<EGLAttrib>::max()));
        const int planeWidth = GST_VIDEO_INFO_COMP_WIDTH(&_videoInfo, i);
        const int planeHeight = GST_VIDEO_INFO_COMP_HEIGHT(&_videoInfo, i);
        const int fourcc = drmFourccFor(&_videoInfo, i);
        if (fourcc == -1) {
            qCWarning(GstDmaBufLog) << "no DRM fourcc for format"
                                    << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&_videoInfo));
            ok = false;
            break;
        }

        // EGL attribute list — sized for the maximum (with modifier ext) and zero-terminated.
        EGLAttrib attribs[18];
        int n = 0;
#define ATTRIB_PUSH(k, v) do { Q_ASSERT(n < int(std::size(attribs)) - 1); attribs[n++] = (k); attribs[n++] = (v); } while (0)
        ATTRIB_PUSH(EGL_WIDTH,                    planeWidth);
        ATTRIB_PUSH(EGL_HEIGHT,                   planeHeight);
        ATTRIB_PUSH(EGL_LINUX_DRM_FOURCC_EXT,     fourcc);
        ATTRIB_PUSH(kPlFd[0],                     fd);
        ATTRIB_PUSH(kPlOffset[0],                 static_cast<EGLAttrib>(offset));
        ATTRIB_PUSH(kPlPitch[0],                  static_cast<EGLAttrib>(stride));
        if (hasModifiersExt) {
            ATTRIB_PUSH(kPlModLo[i], modLo);  // plane-specific key per EGL_EXT_image_dma_buf_import_modifiers spec
            ATTRIB_PUSH(kPlModHi[i], modHi);
        }
#undef ATTRIB_PUSH
        attribs[n++] = EGL_NONE;
        EGLImage image = eglCreateImage(eglDpy, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
                                        nullptr, attribs);
        if (image == EGL_NO_IMAGE_KHR) {
            qCWarning(GstDmaBufLog) << "eglCreateImage failed plane" << i << "err"
                                    << Qt::hex << eglGetError();
            ok = false;
            break;
        }
        functions.glBindTexture(GL_TEXTURE_2D, names[i]);
        eglImageTargetTexture2D(GL_TEXTURE_2D, image);
        eglDestroyImage(eglDpy, image);
    }

textures_built:

    if (!ok) {
        functions.glDeleteTextures(planeCount, names.data());
        return fail();
    }

    auto frameTextures = std::make_unique<FrameTextures>(&rhi, _format.frameSize(),
                                                         _format.pixelFormat(), names, planeCount);
    // FrameTextures ctor null-resets any plane that QRhiTexture::createFrom() rejects;
    // returning a partially-empty textures bundle would render black without
    // incrementing the failure counter (Qt sees a "successful" frame). Bail loudly instead.
    for (int i = 0; i < planeCount; ++i) {
        if (!frameTextures->texture(i)) {
            qCWarning(GstDmaBufLog) << "FrameTextures plane" << i << "createFrom failed — dropping frame";
            // ~FrameTextures will glDeleteTextures via releaseGLTextures() — names are owned now.
            return fail();
        }
    }
    return frameTextures;
}

quint64 GstDmaBufVideoBuffer::takeMapFailureCount()
{
    return s_mapFailureCount.exchange(0, std::memory_order_relaxed);
}

quint64 GstDmaBufVideoBuffer::peekMapFailureCount()
{
    return s_mapFailureCount.load(std::memory_order_relaxed);
}

#endif // QGC_HAS_GST_DMABUF_GPU_PATH
