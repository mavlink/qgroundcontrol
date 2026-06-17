#include "GstGlVideoBuffer.h"

#include "GstGlFrameTextures.h"
#include "GstHwImportCache.h"
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

// Pool-ring recycle key: the gst-gl texture id tuple for a frame. gst-gl rotates a fixed set of ids, so re-seeing a
// tuple means a pool slot recycled (the immediate `old`-bundle reuse below only covers the last frame, not the ring).
struct GlTexKey
{
    std::array<GLuint, kMaxPlanes> names{};
    int count = 0;

    bool operator==(const GlTexKey& o) const noexcept { return count == o.count && names == o.names; }
};

struct GlTexKeyHash
{
    std::size_t operator()(const GlTexKey& k) const noexcept
    {
        std::size_t h = std::hash<int>{}(k.count);
        for (int i = 0; i < k.count; ++i) {
            h ^= std::hash<GLuint>{}(k.names[i]) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        return h;
    }
};

// Render-thread-confined (mapTextures runs on the QRhi thread): no locking needed. Values are non-owning markers — the
// GL textures belong to gst-gl, so the deleter is a no-op; the cache only tracks ring membership for reuse telemetry.
constexpr std::size_t kGlRingCapacity = 8;
GstHw::GstHwImportCache<GlTexKey, char, GlTexKeyHash> s_glRingCache{kGlRingCapacity, [](const GlTexKey&, char&) {}};

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

    // Mirror Qt's mapFromGlTexture: set_sync_point + GPU-side wait; synthesize meta when upstream omits one.
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
            gst_gl_sync_meta_wait(syncMeta, glCtx);
            GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::GlMemory, /*gpuSide=*/true);
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

    if (auto* prev = GstHwFrameTexturesBase::reusableBundle<FrameTextures>(old, HwVideoBufferPath::GlMemory)) {
        if (prev->matches(&rhi, _format.frameSize(), _format.pixelFormat(), names, planeCount)) {
            GstHwPathTelemetry::recordTextureReuse(HwVideoBufferPath::GlMemory);
            prev->setSourceSample(takeSample());
            QVideoFrameTexturesUPtr reused = std::move(old);
            return reused;
        }
    }

    // Ring recycle: the `old` bundle missed (different frame) but this id tuple was validated within the last
    // kGlRingCapacity frames, so the pool reused a slot. Record the reuse; the createFrom below is a cheap non-owning
    // view over the existing gst-gl texture (no real re-import).
    const GlTexKey ringKey{names, planeCount};
    const bool ringHit = (s_glRingCache.find(ringKey) != nullptr);

    // Pre-flight the per-plane RHI format/size before createFrom() so an unsupported import demotes to CPU on a cheap
    // capability query instead of a driver error.
    if (!GstHwImportPreflight::preflightOrRecord(&rhi, HwVideoBufferPath::GlMemory, _format.pixelFormat(),
                                                 _format.frameSize())) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
    }

    auto textures =
        std::make_unique<FrameTextures>(&rhi, _format.frameSize(), _format.pixelFormat(), names, planeCount);
    // For NV12/I420 chroma can fail while luma succeeds; must verify all planes, not just plane 0.
    for (int i = 0; i < planeCount; ++i) {
        if (!textures->texture(static_cast<uint>(i))) {
            qCWarning(GstGlBufLog) << "createFrom failed for plane" << i << "format=" << _format.pixelFormat();
            GstHwPathTelemetry::recordFallbackReason(HwVideoBufferPath::GlMemory,
                                                     GstHwPathTelemetry::HwFallbackReason::MapFailed);
            return GstHwPathTelemetry::fail(HwVideoBufferPath::GlMemory);
        }
    }
    if (ringHit) {
        GstHwPathTelemetry::recordTextureReuse(HwVideoBufferPath::GlMemory);
    }
    s_glRingCache.insert(ringKey, char{});
    textures->setSourceSample(takeSample());
    return textures;
}

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
