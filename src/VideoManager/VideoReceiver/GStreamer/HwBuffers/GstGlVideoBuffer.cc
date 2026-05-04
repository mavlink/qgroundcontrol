#include "GstGlVideoBuffer.h"
#include "GstHwVideoBuffer.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include "QGCLoggingCategory.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include <gst/gl/gl.h>
#include <gst/gl/gstglbasememory.h>
#include <gst/gl/gstglmemory.h>
#include <gst/gl/gstglsyncmeta.h>
#include <gst/video/video.h>

// Qt's portable GL types (GLuint et al.) — works on Linux/macOS/Android without GLES2 SDK headers.
#include <QtGui/qopengl.h>

#include <array>
#include <atomic>

QGC_LOGGING_CATEGORY(GstGlBufLog, "Video.GStreamer.HwBuffers.GstGlBuf")

namespace {

constexpr int kMaxPlanes = 4;

std::atomic<quint64> s_mapFailureCount{0};

QVideoFrameTexturesUPtr fail()
{
    s_mapFailureCount.fetch_add(1, std::memory_order_relaxed);
    return {};
}

class FrameTextures final : public QVideoFrameTextures
{
public:
    FrameTextures(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                  std::array<GLuint, kMaxPlanes> names, int count)
        : _rhi(rhi), _size(size), _pixelFormat(pixelFormat), _names(names), _count(count)
    {
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc) {
            return;
        }
        for (int i = 0; i < _count; ++i) {
            // GL_NONE (0) would be silently accepted by createFrom and sample as black —
            // gst-gl can hand us 0 if a plane wasn't uploaded yet.
            if (names[i] == 0) {
                qCWarning(GstGlBufLog) << "FrameTextures: GL texture id is 0 for plane" << i;
                continue;
            }
            const QSize planeSize = desc->rhiPlaneSize(size, i, rhi);
            _textures[i].reset(rhi->newTexture(desc->rhiTextureFormat(i, rhi), planeSize, 1, {}));
            if (_textures[i] && !_textures[i]->createFrom({names[i], 0})) {
                _textures[i].reset();
            }
        }
    }

    QRhiTexture *texture(uint plane) const override
    {
        return (int(plane) < _count) ? _textures[plane].get() : nullptr;
    }

    // Reuse-eligibility: same rhi+size+format and identical, non-zero per-plane GL texture ids.
    // gst-gl rotates ids within a fixed pool; the QRhiTexture is just a thin view, so when the
    // pool re-binds id N to new frame contents the wrapper transparently samples the new data.
    bool matches(QRhi *rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                 const std::array<GLuint, kMaxPlanes> &names, int count) const
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

private:
    QRhi *_rhi = nullptr;
    QSize _size;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
    std::array<GLuint, kMaxPlanes> _names{};
    int _count = 0;
    std::unique_ptr<QRhiTexture> _textures[kMaxPlanes];
};

std::atomic<quint64> s_textureReuseHits{0};
std::atomic<quint64> s_gpuWaitCount{0};
std::atomic<quint64> s_cpuWaitCount{0};

std::atomic<bool> s_loggedNullSample{false};
std::atomic<bool> s_loggedBadBackend{false};

#define GL_WARN_ONCE(flag, ...) \
    do { if (!(flag).exchange(true, std::memory_order_relaxed)) qCWarning(GstGlBufLog) << __VA_ARGS__; } while (0)

} // namespace

GstGlVideoBuffer::GstGlVideoBuffer(GstSample *sample,
                                   const GstVideoInfo &videoInfo,
                                   const QVideoFrameFormat &format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{
}

GstGlVideoBuffer::~GstGlVideoBuffer() = default;

QAbstractVideoBuffer::MapData GstGlVideoBuffer::map(QVideoFrame::MapMode /*mode*/)
{
    return {};
}

bool GstGlVideoBuffer::validatePlaneHandles() const
{
    // Allocator-only — GLuint check needs GST_MAP_GL, too expensive on streaming thread.
    if (!_sample) return false;
    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) return false;
    const int memCount = qMin(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    if (memCount <= 0) return false;
    for (int i = 0; i < memCount; ++i) {
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_gl_memory(mem)) return false;
    }
    return true;
}

QVideoFrameTexturesUPtr GstGlVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr &old)
{
    Q_ASSERT(rhi.thread()->isCurrentThread()); // Qt's contract: mapTextures runs on the QRhi (render) thread.
    if (!_sample) {
        GL_WARN_ONCE(s_loggedNullSample, "mapTextures: GstSample is null");
        return fail();
    }
    // QRhi::OpenGLES2 covers both desktop GL and GLES — Qt collapses both into
    // the same backend enum value. There is no separate QRhi::OpenGL.
    if (rhi.backend() != QRhi::OpenGLES2) {
        GL_WARN_ONCE(s_loggedBadBackend, "QRhi backend is" << rhi.backendName()
                     << "(GL backend required); GLMemory path disabled");
        return fail();
    }

    // QRhi::createFrom calls glBindTexture/glTexParameteri internally — defensive bracket against custom RHI integrations where QSGRenderThread doesn't have Qt's GL context bound on entry.
    rhi.makeThreadLocalNativeContextCurrent();

    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) return fail();

    GstMemory *mem0 = gst_buffer_peek_memory(buffer, 0);
    if (!mem0 || !gst_is_gl_memory(mem0)) {
        return fail();
    }

    GstVideoFrame frame{};
    // GST_MAP_GL (a gst-gl extension flag) OR'd with GST_MAP_READ triggers GstGLMemory upload.
    if (!gst_video_frame_map(&frame, &_videoInfo, buffer,
                             static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL))) {
        qCWarning(GstGlBufLog) << "gst_video_frame_map(GST_MAP_READ | GST_MAP_GL) failed";
        return fail();
    }

    // wait_cpu (CPU-blocking) is the only durably correct fence here. The cheaper GPU-side
    // wait() — even when gst_gl_context_can_share(producer, primed) returns true — produces
    // periodic torn / saturated-green NV12 frames on Linux Mesa: wait() only records ordering
    // in the caller's GL command stream, and Mesa's per-thread queues don't always flush in
    // time for QRhi to sample (UV plane reads partially-written memory). wait_cpu blocks
    // until the producer's fence signals on the CPU side, which is what QRhi actually needs.
    GstGLContext *glCtx = GST_GL_BASE_MEMORY_CAST(mem0)->context;
    if (GstGLSyncMeta *syncMeta = gst_buffer_get_gl_sync_meta(buffer); syncMeta && glCtx) {
        gst_gl_sync_meta_wait_cpu(syncMeta, glCtx);
        s_cpuWaitCount.fetch_add(1, std::memory_order_relaxed);
    }

    const int planeCount = qBound(1, int(GST_VIDEO_FRAME_N_PLANES(&frame)), kMaxPlanes);
    std::array<GLuint, kMaxPlanes> names{};
    for (int i = 0; i < planeCount; ++i) {
        // GST_MAP_GL maps return a *pointer to* the GLuint texture id, not the id itself.
        const GLuint *texIdPtr = static_cast<const GLuint *>(GST_VIDEO_FRAME_PLANE_DATA(&frame, i));
        names[i] = texIdPtr ? *texIdPtr : 0;
    }

    gst_video_frame_unmap(&frame);

    // Reuse Qt's previous textures wrapper if the pool gave us identical GL ids on the same RHI.
    // dynamic_cast (Qt builds with RTTI) safely no-ops when `old` came from a different video buffer
    // type (its anon-namespace FrameTextures is a different translation-unit type).
    if (old) {
        if (auto *prev = dynamic_cast<FrameTextures *>(old.get());
            prev && prev->matches(&rhi, _format.frameSize(), _format.pixelFormat(), names, planeCount)) {
            s_textureReuseHits.fetch_add(1, std::memory_order_relaxed);
            return std::move(old);
        }
    }

    auto textures = std::make_unique<FrameTextures>(&rhi, _format.frameSize(),
                                                    _format.pixelFormat(), names, planeCount);
    // FrameTextures null-resets any plane that QRhiTexture::createFrom rejects; for NV12/I420
    // the chroma plane can fail while luma succeeds, producing a "successful" frame with
    // missing chroma if we only check plane 0. Verify all planes (matches DMABuf path).
    for (int i = 0; i < planeCount; ++i) {
        if (!textures->texture(static_cast<uint>(i))) {
            qCWarning(GstGlBufLog) << "createFrom failed for plane" << i
                                   << "format=" << _format.pixelFormat();
            return fail();
        }
    }
    return textures;
}

quint64 GstGlVideoBuffer::peekTextureReuseHits()
{
    return s_textureReuseHits.load(std::memory_order_relaxed);
}

quint64 GstGlVideoBuffer::takeTextureReuseHits()
{
    return s_textureReuseHits.exchange(0, std::memory_order_relaxed);
}

quint64 GstGlVideoBuffer::takeSyncWaitCounts(quint64 &gpuWaits)
{
    gpuWaits = s_gpuWaitCount.exchange(0, std::memory_order_relaxed);
    return s_cpuWaitCount.exchange(0, std::memory_order_relaxed);
}

quint64 GstGlVideoBuffer::takeMapFailureCount()
{
    return s_mapFailureCount.exchange(0, std::memory_order_relaxed);
}

quint64 GstGlVideoBuffer::peekMapFailureCount()
{
    return s_mapFailureCount.load(std::memory_order_relaxed);
}

#endif // QGC_HAS_GST_GLMEMORY_GPU_PATH
