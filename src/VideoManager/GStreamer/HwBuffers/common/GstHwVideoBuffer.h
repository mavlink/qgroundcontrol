#pragma once

#include <algorithm>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <private/qhwvideobuffer_p.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QSize>
#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "CpuVideoFramePool.h"
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

namespace GstHwPathTelemetry {

/// Records a map failure for @p path and returns the empty bundle mapTextures() reports it with.
QVideoFrameTexturesUPtr fail(HwVideoBufferPath path) noexcept;

}  // namespace GstHwPathTelemetry

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

/// \brief Common base for GStreamer-backed QHwVideoBuffer subclasses.
class GstHwVideoBuffer : public QHwVideoBuffer
{
public:
    GstHwVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, QVideoFrameFormat format);
    ~GstHwVideoBuffer() override;

    QVideoFrameFormat format() const override { return _format; }

    /// CPU view of the sample through gst_video_frame_map (e.g. GL memory downloads, DMABuf mmaps). Qt's
    /// createTextures() uploads this whenever mapTextures() fails, so a failed GPU import still shows the frame.
    /// ReadOnly only. Refused when format() advertises another pixel format than the memory holds: Qt would upload it
    /// in the wrong format or read past it.
    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;

    /// Streaming-thread sanity check on per-plane handles; failure routes to CPU memcpy.
    virtual bool validatePlaneHandles() const { return true; }

    /// New sample ref for the texture bundle built in mapTextures(). The buffer keeps its own ref so map() and a
    /// repeated mapTextures() (another window, QRhi re-creation) still find the sample; Qt keeps this frame alive
    /// alongside the bundle anyway (QVideoFrameTextures::setSourceFrame), so no buffer is held longer.
    GstSample* refSample() noexcept { return _sample ? gst_sample_ref(_sample) : nullptr; }

protected:
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
    CpuVideoFramePool::FrameMapping _cpuView;
};
