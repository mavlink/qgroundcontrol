#include "GstDmaBufRhiVideoBuffer.h"

#ifdef QGC_GST_DMABUF

#include <QtCore/qscopeguard.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/qopengl.h>
#include <QtGui/rhi/qrhi.h>
#include <QtGui/rhi/qrhi_platform.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <array>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/video-frame.h>

#include "QGCLoggingCategory.h"

#if defined(Q_OS_LINUX)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

QGC_LOGGING_CATEGORY(GstDmaBufRhiVideoBufferLog, "Video.GstDmaBufRhiVideoBuffer")

namespace {

constexpr uint32_t fourcc(char a, char b, char c, char d)
{
    return static_cast<uint32_t>(a) |
           (static_cast<uint32_t>(b) << 8) |
           (static_cast<uint32_t>(c) << 16) |
           (static_cast<uint32_t>(d) << 24);
}

constexpr int kDrmFormatRgb888 = fourcc('R', 'G', '2', '4');
constexpr int kDrmFormatBgr888 = fourcc('B', 'G', '2', '4');
constexpr int kDrmFormatArgb8888 = fourcc('A', 'R', '2', '4');
constexpr int kDrmFormatAbgr8888 = fourcc('A', 'B', '2', '4');
constexpr int kDrmFormatYuyv = fourcc('Y', 'U', 'Y', 'V');
constexpr int kDrmFormatUyvy = fourcc('U', 'Y', 'V', 'Y');
constexpr int kDrmFormatAyuv = fourcc('A', 'Y', 'U', 'V');
constexpr int kDrmFormatR8 = fourcc('R', '8', ' ', ' ');
constexpr int kDrmFormatR16 = fourcc('R', '1', '6', ' ');
constexpr int kDrmFormatRg88 = fourcc('R', 'G', '8', '8');
constexpr int kDrmFormatRg1616 = fourcc('R', 'G', '3', '2');
constexpr int kDrmFormatBgra1010102 = fourcc('B', 'A', '3', '0');

int drmFourccForPlane(const GstVideoInfo& info, int plane)
{
    const GstVideoFormat format = GST_VIDEO_INFO_FORMAT(&info);
    switch (format) {
    case GST_VIDEO_FORMAT_RGB:
        return kDrmFormatRgb888;
    case GST_VIDEO_FORMAT_BGR:
        return kDrmFormatBgr888;
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
        return kDrmFormatArgb8888;
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_xBGR:
        return kDrmFormatAbgr8888;
    case GST_VIDEO_FORMAT_YUY2:
        return kDrmFormatYuyv;
    case GST_VIDEO_FORMAT_UYVY:
        return kDrmFormatUyvy;
    case GST_VIDEO_FORMAT_AYUV:
        return kDrmFormatAyuv;
    case GST_VIDEO_FORMAT_GRAY8:
        return kDrmFormatR8;
    case GST_VIDEO_FORMAT_GRAY16_LE:
    case GST_VIDEO_FORMAT_GRAY16_BE:
        return kDrmFormatR16;
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
        return plane == 0 ? kDrmFormatR8 : kDrmFormatRg88;
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:
    case GST_VIDEO_FORMAT_Y41B:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
        return kDrmFormatR8;
    case GST_VIDEO_FORMAT_P010_10LE:
    case GST_VIDEO_FORMAT_P010_10BE:
        return plane == 0 ? kDrmFormatR16 : kDrmFormatRg1616;
#if GST_CHECK_VERSION(1, 16, 0)
    case GST_VIDEO_FORMAT_BGR10A2_LE:
        return kDrmFormatBgra1010102;
#endif
    default:
        return -1;
    }
}

struct ImportedGlTextures
{
    uint count = 0;
    std::array<GLuint, QVideoTextureHelper::TextureDescription::maxPlanes> names{};
};

class GstDmaBufRhiTextures : public QVideoFrameTextures
{
public:
    GstDmaBufRhiTextures(QRhi& rhi, const QSize& size, QVideoFrameFormat::PixelFormat format,
                         ImportedGlTextures textures)
        : _rhi(&rhi)
        , _textures(textures)
    {
        const QVideoTextureHelper::TextureDescription* desc = QVideoTextureHelper::textureDescription(format);
        if (!desc)
            return;

        const auto fallbackPolicy = QVideoTextureHelper::TextureDescription::FallbackPolicy::Disable;
        for (uint plane = 0; plane < _textures.count; ++plane) {
            const QSize planeSize = desc->rhiPlaneSize(size, static_cast<int>(plane), nullptr);
            const QRhiTexture::Format rhiFormat = desc->rhiTextureFormat(static_cast<int>(plane), _rhi, fallbackPolicy);
            _rhiTextures[plane].reset(_rhi->newTexture(rhiFormat, planeSize, 1));
            if (_rhiTextures[plane] && !_rhiTextures[plane]->createFrom({_textures.names[plane], 0})) {
                qCWarning(GstDmaBufRhiVideoBufferLog)
                    << "Failed to wrap imported DMA-BUF GL texture" << plane;
                _rhiTextures[plane].reset();
            }
        }
    }

    ~GstDmaBufRhiTextures() override
    {
        if (!_rhi || _textures.count == 0)
            return;

        if (!_rhi->makeThreadLocalNativeContextCurrent())
            return;

        auto* nativeHandles = static_cast<const QRhiGles2NativeHandles*>(_rhi->nativeHandles());
        QOpenGLContext* context = nativeHandles ? nativeHandles->context : QOpenGLContext::currentContext();
        if (!context)
            return;

        context->functions()->glDeleteTextures(static_cast<GLsizei>(_textures.count), _textures.names.data());
    }

    QRhiTexture* texture(uint plane) const override
    {
        return plane < _textures.count ? _rhiTextures[plane].get() : nullptr;
    }

    [[nodiscard]] bool valid() const
    {
        if (_textures.count == 0)
            return false;

        for (uint plane = 0; plane < _textures.count; ++plane) {
            if (!_rhiTextures[plane])
                return false;
        }

        return true;
    }

private:
    QRhi* _rhi = nullptr;
    ImportedGlTextures _textures;
    std::unique_ptr<QRhiTexture> _rhiTextures[QVideoTextureHelper::TextureDescription::maxPlanes];
};

#if defined(Q_OS_LINUX)
ImportedGlTextures importDmaBufTextures(QRhi& rhi, GstBuffer* buffer, const GstVideoInfo& videoInfo)
{
    ImportedGlTextures textures;

    if (rhi.backend() != QRhi::OpenGLES2)
        return textures;
    if (!rhi.makeThreadLocalNativeContextCurrent())
        return textures;

    auto* nativeHandles = static_cast<const QRhiGles2NativeHandles*>(rhi.nativeHandles());
    QOpenGLContext* context = nativeHandles ? nativeHandles->context : QOpenGLContext::currentContext();
    if (!context)
        return textures;

    EGLDisplay display = eglGetCurrentDisplay();
    if (display == EGL_NO_DISPLAY)
        return textures;

    auto* eglImageTargetTexture2D =
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
    if (!eglImageTargetTexture2D)
        return textures;

    GstVideoFrame frame{};
    GstVideoInfo info = videoInfo;
    if (!gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ))
        return textures;

    const auto unmapFrame = qScopeGuard([&frame]() { gst_video_frame_unmap(&frame); });

    constexpr int kMaxPlanes = QVideoTextureHelper::TextureDescription::maxPlanes;
    const int nPlanes = GST_VIDEO_FRAME_N_PLANES(&frame);
    const int nMemoryBlocks = static_cast<int>(gst_buffer_n_memory(buffer));
    if (nPlanes <= 0 || nPlanes > kMaxPlanes || nMemoryBlocks <= 0)
        return textures;

    std::array<int, kMaxPlanes> fds{-1, -1, -1};
    for (int memory = 0; memory < nMemoryBlocks && memory < kMaxPlanes; ++memory) {
        GstMemory* gstMemory = gst_buffer_peek_memory(buffer, static_cast<guint>(memory));
        if (!gstMemory || !gst_is_dmabuf_memory(gstMemory))
            return {};
        fds[memory] = gst_dmabuf_memory_get_fd(gstMemory);
    }

    auto fdForPlane = [&](int plane) {
        if (plane >= 0 && plane < nMemoryBlocks && fds[plane] >= 0)
            return fds[plane];
        return fds[0];
    };

    QOpenGLFunctions* functions = context->functions();
    functions->glGenTextures(nPlanes, textures.names.data());
    textures.count = static_cast<uint>(nPlanes);
    auto cleanupTextures = qScopeGuard([&functions, &textures]() {
        if (textures.count > 0)
            functions->glDeleteTextures(static_cast<GLsizei>(textures.count), textures.names.data());
        textures.count = 0;
    });

    for (int plane = 0; plane < nPlanes; ++plane) {
        const int fd = fdForPlane(plane);
        const int drmFormat = drmFourccForPlane(info, plane);
        if (fd < 0 || drmFormat < 0)
            return {};

        std::array<EGLAttrib, 13> attributes{};
        int index = 0;
        attributes[index++] = EGL_WIDTH;
        attributes[index++] = GST_VIDEO_FRAME_COMP_WIDTH(&frame, plane);
        attributes[index++] = EGL_HEIGHT;
        attributes[index++] = GST_VIDEO_FRAME_COMP_HEIGHT(&frame, plane);
        attributes[index++] = EGL_LINUX_DRM_FOURCC_EXT;
        attributes[index++] = drmFormat;
        attributes[index++] = EGL_DMA_BUF_PLANE0_FD_EXT;
        attributes[index++] = fd;
        attributes[index++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
        attributes[index++] = static_cast<EGLAttrib>(GST_VIDEO_FRAME_PLANE_OFFSET(&frame, plane));
        attributes[index++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
        attributes[index++] = static_cast<EGLAttrib>(GST_VIDEO_FRAME_PLANE_STRIDE(&frame, plane));
        attributes[index++] = EGL_NONE;

        EGLImage image = eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attributes.data());
        if (image == EGL_NO_IMAGE)
            return {};

        functions->glBindTexture(GL_TEXTURE_2D, textures.names[plane]);
        eglImageTargetTexture2D(GL_TEXTURE_2D, image);
        eglDestroyImage(display, image);
    }

    cleanupTextures.dismiss();
    return textures;
}
#else
ImportedGlTextures importDmaBufTextures(QRhi&, GstBuffer*, const GstVideoInfo&)
{
    return {};
}
#endif

}  // namespace

GstDmaBufRhiVideoBuffer::GstDmaBufRhiVideoBuffer(GstBuffer* buffer, const GstVideoInfo& videoInfo,
                                                 const QVideoFrameFormat& format)
    : QHwVideoBuffer(QVideoFrame::RhiTextureHandle)
    , _buffer(gst_buffer_ref(buffer))
    , _videoInfo(videoInfo)
    , _format(format)
    , _cpuFallback(buffer, videoInfo, format)
{
}

GstDmaBufRhiVideoBuffer::~GstDmaBufRhiVideoBuffer()
{
    if (_buffer)
        gst_buffer_unref(_buffer);
}

QAbstractVideoBuffer::MapData GstDmaBufRhiVideoBuffer::map(QVideoFrame::MapMode mode)
{
    return _cpuFallback.map(mode);
}

void GstDmaBufRhiVideoBuffer::unmap()
{
    _cpuFallback.unmap();
}

QVideoFrameFormat GstDmaBufRhiVideoBuffer::format() const
{
    return _format;
}

bool GstDmaBufRhiVideoBuffer::isDmaBuf() const
{
    return true;
}

QVideoFrameTexturesUPtr GstDmaBufRhiVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures)
{
    Q_UNUSED(oldTextures)

    const QVideoTextureHelper::TextureDescription* desc =
        QVideoTextureHelper::textureDescription(_format.pixelFormat());
    if (!desc)
        return {};

    const ImportedGlTextures textures = importDmaBufTextures(rhi, _buffer, _videoInfo);
    if (textures.count == 0 || textures.count != static_cast<uint>(desc->nplanes))
        return {};

    auto result = std::make_unique<GstDmaBufRhiTextures>(rhi, _format.frameSize(), _format.pixelFormat(), textures);
    return result->valid() ? std::move(result) : nullptr;
}

#endif  // QGC_GST_DMABUF
