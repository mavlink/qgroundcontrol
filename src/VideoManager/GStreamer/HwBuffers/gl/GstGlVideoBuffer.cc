#include "GstGlVideoBuffer.h"

#include "GstGlContextBridge.h"
#include "GstHwFrameMapping.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#include "QGCRhiCapture.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include <gst/gl/gl.h>
#include <gst/gl/gstglbasememory.h>
#include <gst/gl/gstglmemory.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/video/video.h>
#include <rhi/qrhi.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QScopeGuard>
#include <QtCore/QSize>

#include "QGCLoggingCategory.h"

// Qt's portable GL types (GLuint et al.) — works on Linux/macOS/Android without GLES2 SDK headers.
#include <algorithm>
#include <array>
#include <atomic>
#include <utility>

#include <QtGui/qopengl.h>

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLExtraFunctions>

QGC_LOGGING_CATEGORY(GstGlBufLog, "Video.GStreamer.HwBuffers.GstGlBuf")

namespace {

using GstHw::kMaxPlanes;

class FrameTextures final : public GstHwFrameTexturesBase
{
public:
    FrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat, std::span<const GLuint> names)
    {
        wrapPlanes(rhi, size, pixelFormat, names);
    }

    ~FrameTextures() override { gst_clear_buffer(&_syncBuffer); }

    /// The sync buffer the bundle was mapped with (takeSyncBuffer()), kept for the next map of its pool slot.
    void setSyncBuffer(GstBuffer* buffer) noexcept
    {
        gst_clear_buffer(&_syncBuffer);
        _syncBuffer = buffer;
    }

    /// The caller's from here on; null when the bundle has none.
    GstBuffer* releaseSyncBuffer() noexcept { return std::exchange(_syncBuffer, nullptr); }

private:
    GstBuffer* _syncBuffer = nullptr;
};

// An empty buffer of this path's own with a GL sync meta for @p context, as upstream's Qt sink keeps one
// (gstqsg6material.cc): the one @p old was mapped with, when that is a bundle of this path on the same context, else a
// new one. The frame's own buffer may carry a sync meta too, but other elements and other windows showing the frame
// share that buffer, and setting a sync point deletes the fence the meta holds. Null when the meta can't be added.
GstBuffer* takeSyncBuffer(QVideoFrameTexturesUPtr& old, GstGLContext* context)
{
    if (auto* const previous = GstHwFrameTexturesBase::reusableBundle<FrameTextures>(old)) {
        GstBuffer* buffer = previous->releaseSyncBuffer();
        const GstGLSyncMeta* const meta = buffer ? gst_buffer_get_gl_sync_meta(buffer) : nullptr;
        if (meta && (meta->context == context)) {
            return buffer;
        }
        gst_clear_buffer(&buffer);
    }
    GstBuffer* buffer = gst_buffer_new();
    if (!gst_buffer_add_gl_sync_meta(context, buffer)) {
        gst_clear_buffer(&buffer);
    }
    return buffer;
}

// Orders Qt's context after gst-gl's fence. gst_gl_sync_meta_wait() would wait inside gst-gl's own context and leave
// Qt's unordered; GL sync objects are shared across the share group, so Qt's context waits on the GLsync server-side.
// CPU wait when Qt's context has no sync objects or the meta carries no GLsync (gst-gl without fences).
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
// than the render-thread-owned QRhi. So do frames from a GL context outside Qt's share group, which gst-gl creates
// when no element got the bridge's: their texture names mean nothing in Qt's context, which would sample another
// texture or black. Records each rejected frame.
bool admitGlMemory(GstMemory* mem, const HwVideoBufferContext&)
{
    const int backend = QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire);
    if ((backend >= 0) && (backend != static_cast<int>(QRhi::OpenGLES2))) {
        QGC_HW_WARN_ONCE(GstGlBufLog, "GLMemory frame but QRhi backend is not OpenGL — routing to CPU pool");
    } else if (!GstGlContextBridge::sharesQtContext(GST_GL_BASE_MEMORY_CAST(mem)->context)) {
        QGC_HW_WARN_ONCE(GstGlBufLog, "GLMemory from a GL context that doesn't share with Qt's — routing to CPU pool");
    } else {
        return true;
    }
    GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::GlMemory,
                                             GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
    return false;
}

}  // namespace

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
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::OpenGLES2), GstGlBufLog(), buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
    }

    // Defensive: custom RHI integrations may not have Qt's GL context current on QSGRenderThread entry.
    rhi.makeThreadLocalNativeContextCurrent();

    GstMemory* mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0 || !gst_is_gl_memory(mem0)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
    }

    GstHw::FrameMapping mapping;
    // GST_MAP_GL (a gst-gl extension flag) OR'd with GST_MAP_READ triggers GstGLMemory upload.
    if (!mapping.mapFrame(buffer, _videoInfo, static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL))) {
        qCWarning(GstGlBufLog) << "gst_video_frame_map(GST_MAP_READ | GST_MAP_GL) failed";
        GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::GlMemory,
                                                 GstHwPathTelemetry::HwFallbackReason::MapFailed);
        return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
    }

    // A sync point of our own, set after the GST_MAP_GL map above: glupload uploads lazily on that map, so a sync point
    // its producer set would predate the upload.
    GstGLContext* glCtx = GST_GL_BASE_MEMORY_CAST(mem0)->context;
    GstBuffer* syncBuffer = glCtx ? takeSyncBuffer(old, glCtx) : nullptr;
    const auto dropSyncBuffer = qScopeGuard([&syncBuffer] { gst_clear_buffer(&syncBuffer); });
    if (syncBuffer) {
        GstGLSyncMeta* const syncMeta = gst_buffer_get_gl_sync_meta(syncBuffer);
        gst_gl_sync_meta_set_sync_point(syncMeta, glCtx);
        waitInQtContext(syncMeta, glCtx);
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
    const std::span<const GLuint> planeNames(names.data(), std::size_t(planeCount));

    const GstHwMapTail tail{.path = HwVideoBufferPath::GlMemory,
                            .frameSize = _format.frameSize(),
                            .pixelFormat = _format.pixelFormat(),
                            .log = &GstGlBufLog()};
    // gst-gl rotates texture ids within a fixed pool: a bundle over the same ids samples the new frame unchanged.
    QVideoFrameTexturesUPtr textures = GstHwFrameTexturesBase::finishMap<FrameTextures>(
        *this, rhi, old, tail,
        [&](const FrameTextures& prev) {
            return prev.matchesImport(&rhi, tail.frameSize, tail.pixelFormat, planeNames);
        },
        [&] { return std::make_unique<FrameTextures>(&rhi, tail.frameSize, tail.pixelFormat, planeNames); });
    if (auto* const bundle = GstHwFrameTexturesBase::reusableBundle<FrameTextures>(textures)) {
        bundle->setSyncBuffer(std::exchange(syncBuffer, nullptr));
    }
    return textures;
}

constinit const HwPathDescriptor GstHwPaths::kGlMemory = {
    .path = HwVideoBufferPath::GlMemory,
    .matches = [](GstMemory* mem) -> bool { return gst_is_gl_memory(mem); },
    .admits = &admitGlMemory,
    .construct = &GstHwPaths::construct<GstGlVideoBuffer>,
    .bridge = &GstGlContextBridge::instance,
};

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
