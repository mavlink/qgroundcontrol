#include "GstGlVideoBuffer.h"

#include "CpuVideoFramePool.h"
#include "GstGlContextBridge.h"
#include "GstGlFrameTextures.h"
#include "GstHwImportPreflight.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "QGCRhiCapture.h"

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
#include <algorithm>
#include <array>
#include <atomic>

#include <QtGui/qopengl.h>

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLExtraFunctions>

QGC_LOGGING_CATEGORY(GstGlBufLog, "Video.GStreamer.HwBuffers.GstGlBuf")

namespace {

using GstHw::kMaxPlanes;

// Its own type, so reuse never adopts a DMABuf bundle (also a GstGlFrameTextures).
class FrameTextures final : public GstGlFrameTextures
{
public:
    using GstGlFrameTextures::GstGlFrameTextures;
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
            GstHwPathTelemetry::record(HwVideoBufferPath::GlMemory, GstHwPathTelemetry::Counter::GpuWaits);
            return;
        }
    }
    gst_gl_sync_meta_wait_cpu(syncMeta, glCtx);
    GstHwPathTelemetry::record(HwVideoBufferPath::GlMemory, GstHwPathTelemetry::Counter::CpuWaits);
}

// mapTextures() binds GLMemory only under Qt's OpenGL(ES2) QRhi, but the glupload bin still emits GLMemory under a
// non-GL RHI (forced Vulkan): those frames take the CPU path. Streaming thread, so read the published backend rather
// than the render-thread-owned QRhi. Records each rejected frame.
bool admitGlMemory()
{
    const int backend = QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire);
    if ((backend < 0) || (backend == static_cast<int>(QRhi::OpenGLES2))) {
        return true;
    }
    static std::atomic<bool> s_warned{false};
    if (!s_warned.exchange(true, std::memory_order_relaxed)) {
        qCWarning(GstGlBufLog) << "GLMemory frame but QRhi backend is not OpenGL — routing to CPU pool";
    }
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::GlMemory,
                                             GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
    return false;
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

    CpuVideoFramePool::FrameMapping mapping;
    // GST_MAP_GL (a gst-gl extension flag) OR'd with GST_MAP_READ triggers GstGLMemory upload.
    if (!mapping.mapFrame(buffer, _videoInfo, static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL))) {
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

    const GstVideoFrame& frame = mapping.frame();
    const int planeCount = std::clamp(int(GST_VIDEO_FRAME_N_PLANES(&frame)), 1, kMaxPlanes);
    std::array<GLuint, kMaxPlanes> names{};
    for (int i = 0; i < planeCount; ++i) {
        // GST_MAP_GL maps return a *pointer to* the GLuint texture id, not the id itself.
        const GLuint* texIdPtr = static_cast<const GLuint*>(GST_VIDEO_FRAME_PLANE_DATA(&frame, i));
        names[i] = texIdPtr ? *texIdPtr : 0;
    }

    mapping.unmap();

    const GstHwMapTail tail{.path = HwVideoBufferPath::GlMemory,
                            .frameSize = _format.frameSize(),
                            .pixelFormat = _format.pixelFormat(),
                            .log = &GstGlBufLog(),
                            .diag = &s_diag};
    // gst-gl rotates texture ids within a fixed pool: a bundle over the same ids samples the new frame unchanged.
    return GstHwFrameTexturesBase::finishMap<FrameTextures>(
        *this, rhi, old, tail,
        [&](const FrameTextures& prev) {
            return prev.matchesImport(&rhi, tail.frameSize, tail.pixelFormat,
                                      std::span<const GLuint>(names.data(), std::size_t(planeCount)));
        },
        [&] { return std::make_unique<FrameTextures>(&rhi, tail.frameSize, tail.pixelFormat, names, planeCount); });
}

constinit const HwPathDescriptor GstHwPaths::kGlMemory = {
    .path = HwVideoBufferPath::GlMemory,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool { return gst_is_gl_memory(mem); },
    .admits = &admitGlMemory,
    .construct = &GstHwPaths::construct<GstGlVideoBuffer>,
    .bridge = &GstGlContextBridge::instance,
};

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
