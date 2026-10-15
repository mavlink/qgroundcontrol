#include "GstGlVideoBuffer.h"

#include "GstGlFrameTextures.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <gst/gl/gl.h>
#include <gst/gl/gstglbasememory.h>
#include <gst/gl/gstglmemory.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/video/video.h>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include "QGCLoggingCategory.h"

// Qt's portable GL types (GLuint et al.) — works on Linux/macOS/Android without GLES2 SDK headers.
#include <QtGui/qopengl.h>
#include <algorithm>
#include <array>

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLExtraFunctions>

QGC_LOGGING_CATEGORY(GstGlBufLog, "Video.GStreamer.HwBuffers.GstGlBuf")

namespace {

using GstHw::kMaxPlanes;

class FrameTextures final : public GstGlFrameTextures
{
public:
    using GstGlFrameTextures::GstGlFrameTextures;

    HwVideoBufferPath sourcePath() const override { return HwVideoBufferPath::GlMemory; }

    // Reuse-eligible when ids match: gst-gl rotates ids within a fixed pool, so the QRhiTexture view transparently
    // samples new data.
    bool matches(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                 const std::array<GLuint, kMaxPlanes>& names, int count) const
    {
        if (_rhi != rhi || _size != size || _pixelFormat != pixelFormat || _count != count) {
            return false;
        }
        for (int i = 0; i < _count; ++i) {
            if (_names[i] == 0 || _names[i] != names[i] || !_textures[i]) {
                return false;
            }
        }
        return true;
    }
};

GstHw::MapDiagnostics s_diag;

// Orders Qt's context after gst-gl's fence. gst_gl_sync_meta_wait() would wait inside gst-gl's own context and leave
// Qt's unordered; GL sync objects are shared across the share group, so Qt's context waits on the GLsync server-side.
// CPU wait when Qt's context has no sync objects or the meta carries no GLsync (a custom meta, or gst-gl without
// fences).
void waitInQtContext(GstGLSyncMeta* syncMeta, GstGLContext* glCtx)
{
    QOpenGLContext* qtCtx = QOpenGLContext::currentContext();
    const auto sync = static_cast<GLsync>(syncMeta->data);
    if (qtCtx && sync) {
        const QSurfaceFormat format = qtCtx->format();
        const bool hasSyncObjects = qtCtx->isOpenGLES() ? format.majorVersion() >= 3
                                                        : (format.version() >= qMakePair(3, 2) ||
                                                           qtCtx->hasExtension(QByteArrayLiteral("GL_ARB_sync")));
        QOpenGLExtraFunctions* gl = hasSyncObjects ? qtCtx->extraFunctions() : nullptr;
        if (gl && gl->glIsSync(sync)) {
            gl->glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
            GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::GlMemory, /*gpuSide=*/true);
            return;
        }
    }
    gst_gl_sync_meta_wait_cpu(syncMeta, glCtx);
    GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::GlMemory, /*gpuSide=*/false);
}

}  // namespace

GstGlVideoBuffer::GstGlVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{}

bool GstGlVideoBuffer::validatePlaneHandles() const
{
    // Allocator-only — GLuint check needs GST_MAP_GL, too expensive on streaming thread.
    return validatePlanes([](GstMemory* mem) { return mem && gst_is_gl_memory(mem); });
}

QVideoFrameTexturesUPtr GstGlVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    const GstHwPathTelemetry::ScopedMapTimer mapTimer(HwVideoBufferPath::GlMemory);
    // QRhi::OpenGLES2 covers both desktop GL and GLES — Qt collapses both; there is no separate QRhi::OpenGL.
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::OpenGLES2), GstGlBufLog(), s_diag, buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
    }

    // Defensive: custom RHI integrations may not have Qt's GL context current on QSGRenderThread entry.
    rhi.makeThreadLocalNativeContextCurrent();

    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0 || !gst_is_gl_memory(mem0)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
    }

    GstVideoFrame frame = {};
    // GST_MAP_GL (a gst-gl extension flag) OR'd with GST_MAP_READ triggers GstGLMemory upload.
    if (!gst_video_frame_map(&frame, &_videoInfo, buffer, static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL))) {
        qCWarning(GstGlBufLog) << "gst_video_frame_map(GST_MAP_READ | GST_MAP_GL) failed";
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::GlMemory,
                                                 GstHwPathTelemetry::HwFallbackReason::MapFailed);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
    }

    // Always set a fresh sync point after the GST_MAP_GL map above: glupload attaches its meta without one and uploads
    // lazily on that map, so an upstream sync point would predate the upload. Synthesize a meta when upstream omits
    // one.
    GstGLContext* glCtx = GST_GL_BASE_MEMORY_CAST(mem0)->context;
    if (glCtx) {
        GstGLSyncMeta* syncMeta = gst_buffer_get_gl_sync_meta(buffer);
        GstBuffer* throwaway = nullptr;
        if (!syncMeta) {
            throwaway = gst_buffer_new();
            if (!throwaway) {
                qCWarning(GstGlBufLog) << "gst_buffer_new() failed while creating GL sync meta holder";
                GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::GlMemory,
                                                         GstHwPathTelemetry::HwFallbackReason::MapFailed);
                gst_video_frame_unmap(&frame);
                return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
            }
            syncMeta = gst_buffer_add_gl_sync_meta(glCtx, throwaway);
        }
        if (syncMeta) {
            gst_gl_sync_meta_set_sync_point(syncMeta, glCtx);
            waitInQtContext(syncMeta, glCtx);
        }
        if (throwaway) {
            gst_buffer_unref(throwaway);
        }
    }

    const int planeCount = std::clamp(int(GST_VIDEO_FRAME_N_PLANES(&frame)), 1, kMaxPlanes);
    std::array<GLuint, kMaxPlanes> names{};
    for (int i = 0; i < planeCount; ++i) {
        // GST_MAP_GL maps return a *pointer to* the GLuint texture id, not the id itself.
        const GLuint* texIdPtr = static_cast<const GLuint*>(GST_VIDEO_FRAME_PLANE_DATA(&frame, i));
        names[i] = texIdPtr ? *texIdPtr : 0;
    }

    gst_video_frame_unmap(&frame);

    const GstHwMapTail tail{.path = HwVideoBufferPath::GlMemory,
                            .frameSize = _format.frameSize(),
                            .pixelFormat = _format.pixelFormat(),
                            .log = &GstGlBufLog(),
                            .diag = &s_diag};
    return GstHwFrameTexturesBase::finishMap<FrameTextures>(
        *this, rhi, old, tail,
        [&](const FrameTextures& prev) {
            return prev.matches(&rhi, tail.frameSize, tail.pixelFormat, names, planeCount);
        },
        [&] { return std::make_unique<FrameTextures>(&rhi, tail.frameSize, tail.pixelFormat, names, planeCount); });
}

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
