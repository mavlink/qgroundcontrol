#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>
#include <private/qhwvideobuffer_p.h>
#include <type_traits>

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwFrameMapping.h"
#include "GstHwTypes.h"

namespace GstHw {

/// Matches GST_VIDEO_MAX_PLANES (gst-video pins it at 4); single source of truth for every per-platform buffer.
constexpr int kMaxPlanes = 4;

/// True the first time it is called with @p latch, and the first time after each rearmOnceLatches(). For QGC_HW_ONCE().
bool firstSinceRearm(std::atomic<quint32>& latch) noexcept;

/// Re-arms every QGC_HW_ONCE(), so a recreated device or scene graph reports its failures again. Thread-safe.
void rearmOnceLatches() noexcept;

/// True when a native texture of @p textureSize holds @p picture in its top-left corner with padding around it:
/// decoders pad their surfaces to their alignment without crop meta. @p evenSizes: the texture format (4:2:0) only has
/// even sizes, so an odd picture dimension rounded up is no padding.
bool texturePadsPicture(QSize textureSize, QSize picture, bool evenSizes);

/// @p format with @p pixelFormat in place of its own and every other property kept. QVideoFrameFormat has no
/// pixel-format setter, so a path that advertises another format than its memory's builds a new one around it.
QVideoFrameFormat withPixelFormat(const QVideoFrameFormat& format, QVideoFrameFormat::PixelFormat pixelFormat);

}  // namespace GstHw

/// True the first time control reaches it, and the first time after each GstHw::rearmOnceLatches(). Each use has a
/// latch of its own; in a template, one for each instantiation.
#define QGC_HW_ONCE()                         \
    ([]() noexcept {                          \
        static std::atomic<quint32> latch{0}; \
        return GstHw::firstSinceRearm(latch); \
    }())

/// Logs via qCWarning(LOGCAT) where QGC_HW_ONCE() is true: once for a failure cause, until the GPU caches are reset.
#define QGC_HW_WARN_ONCE(LOGCAT, ...)         \
    do {                                      \
        if (QGC_HW_ONCE()) {                  \
            qCWarning(LOGCAT) << __VA_ARGS__; \
        }                                     \
    } while (0)

/// What keeps one frame's native textures valid while Qt samples them, and their handles, which Qt wraps
/// (QVideoTextureHelper::createTexturesFromHandles()): the frame's sample, and what a path's subclass adds. The
/// textures own it until Qt deletes them, so nothing it holds may be released before its destructor.
class GstHwTextureHandles : public QVideoFrameTexturesHandles
{
public:
    /// Takes over @p sample's reference.
    explicit GstHwTextureHandles(GstSample* sample) noexcept
        : _sample(sample)
    {}

    ~GstHwTextureHandles() override;

    GstHwTextureHandles(const GstHwTextureHandles&) = delete;
    GstHwTextureHandles& operator=(const GstHwTextureHandles&) = delete;

    /// The native texture of @p plane, 0 for a plane without one, which then fails the whole import.
    quint64 textureHandle(QRhi& rhi, int plane) override;

    /// Sets @p plane's native texture: a GL name, an ID3D11Texture2D*, ID3D12Resource* or MTLTexture.
    template <class Handle>
    void setHandle(int plane, Handle handle) noexcept
    {
        if ((plane < 0) || (plane >= GstHw::kMaxPlanes)) {
            return;
        }
        if constexpr (std::is_pointer_v<Handle>) {
            _handles[std::size_t(plane)] = static_cast<quint64>(reinterpret_cast<quintptr>(handle));
        } else {
            _handles[std::size_t(plane)] = static_cast<quint64>(handle);
        }
    }

private:
    GstSample* _sample;
    std::array<quint64, GstHw::kMaxPlanes> _handles{};
};

/// \brief Common base for GStreamer-backed QHwVideoBuffer subclasses.
class GstHwVideoBuffer : public QHwVideoBuffer
{
public:
    GstHwVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, QVideoFrameFormat format);
    ~GstHwVideoBuffer() override;

    QVideoFrameFormat format() const override { return _format; }

    /// The GPU path this buffer imports through, which its timing and failures are counted for.
    virtual HwVideoBufferPath path() const = 0;

    /// The path's importTextures(), timed for the path's map time; a failure is counted for the path. When the import
    /// fails, Qt uploads map() instead; for a frame map() can't read either, @p oldTextures comes back, so the picture
    /// shown before stays. Qt's texture pool deletes @p oldTextures at the end of the frame, and its video material
    /// goes on sampling the last textures it got when a frame yields none (QSGVideoMaterial::updateTextures()).
    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) final;

    /// CPU view of the sample through gst_video_frame_map (e.g. GL memory downloads, DMABuf mmaps). Qt's
    /// createTextures() uploads this whenever mapTextures() fails, so a failed GPU import still shows the frame.
    /// ReadOnly only, and empty without hasCpuLayout().
    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    /// Streaming-thread sanity check on per-plane handles; failure routes to CPU memcpy.
    virtual bool validatePlaneHandles() const { return true; }

    /// New sample ref for the textures an import hands Qt. The buffer keeps its own ref so map() and a repeated
    /// mapTextures() (another window, QRhi re-creation) still find the sample; Qt keeps this frame alive alongside the
    /// textures anyway (QVideoFrameTextures::setSourceFrame), so no buffer is held longer.
    GstSample* refSample() noexcept { return _sample ? gst_sample_ref(_sample) : nullptr; }

protected:
    /// The frame's GPU import into @p rhi, on its thread; null when it fails, which mapTextures() counts.
    /// @p oldTextures are the textures Qt's texture pool replaces, which mapTextures() hands back when the frame shows
    /// nothing else: an import may read them, and take their handles only once its own textures exist.
    virtual QVideoFrameTexturesUPtr importTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) = 0;

    /// Qt's textures of @p handles in @p rhi, for format(), which then own @p handles. Null when a plane doesn't
    /// wrap, which is warned of on @p log once per path. @p successTag names the path in a line logged at its first
    /// import.
    QVideoFrameTexturesUPtr wrapTextures(QRhi& rhi, std::unique_ptr<GstHwTextureHandles> handles,
                                         const QLoggingCategory& log, const char* successTag = nullptr) const;

    /// False for a frame whose memory can't be read as format() describes it: an external-OES frame (Qt would read it
    /// as RGBA8), or one whose format() advertises another pixel format than its memory holds (Qt would upload it in
    /// the wrong format or read past it).
    virtual bool hasCpuLayout() const;

    /// Validate each plane satisfies @p planePred; predicate runs once per plane on the streaming thread.
    template <class PlanePred>
    bool validatePlanes(PlanePred&& planePred) const
    {
        if (!_sample)
            return false;
        GstBuffer* buffer = gst_sample_get_buffer(_sample);
        if (!buffer)
            return false;
        const int memCount = (std::min) (int(gst_buffer_n_memory(buffer)), GstHw::kMaxPlanes);
        if (memCount <= 0)
            return false;
        for (int i = 0; i < memCount; ++i) {
            GstMemory* mem = gst_buffer_peek_memory(buffer, i);
            if (!planePred(mem))
                return false;
        }
        return true;
    }

    /// Shared mapTextures preamble; warns once per cause, returns false on first failure.
    bool checkMapPreconditions(const QRhi& rhi, int expectedBackend, const QLoggingCategory& cat,
                               GstBuffer*& outBuffer) const;

    GstSample* _sample = nullptr;
    GstVideoInfo _videoInfo = {};
    QVideoFrameFormat _format;

private:
    /// Pixel format of the sample's memory, as constructed. A subclass may advertise another in _format for its GPU
    /// import (the DMABuf eglfs YUYV/UYVY-as-RGBA quirk); such frames have no CPU layout matching format().
    QVideoFrameFormat::PixelFormat _memoryPixelFormat;
    GstHw::FrameMapping _cpuView;
};
