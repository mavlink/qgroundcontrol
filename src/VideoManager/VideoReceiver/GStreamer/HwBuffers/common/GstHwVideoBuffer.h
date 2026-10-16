#pragma once

#include <algorithm>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <private/qhwvideobuffer_p.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "CpuVideoFramePool.h"
#include "GstHwTypes.h"

namespace GstHw {

/// Matches GST_VIDEO_MAX_PLANES (gst-video pins it at 4); single source of truth for every per-platform buffer.
constexpr int kMaxPlanes = 4;

/// One-shot warning flags per failure cause; paths with extra causes (D3D, IOSurface) derive and add members.
struct MapDiagnostics
{
    std::atomic<bool> loggedFirstSuccess{false};
    std::atomic<bool> loggedNullSample{false};
    std::atomic<bool> loggedWrongThread{false};
    std::atomic<bool> loggedBadBackend{false};
    std::atomic<bool> loggedNullBuffer{false};
    std::atomic<bool> loggedTextureCreateFail{false};
};

}  // namespace GstHw

namespace GstHwPathTelemetry {

/// Records a map failure for @p path and returns the empty bundle mapTextures() reports it with.
QVideoFrameTexturesUPtr fail(HwVideoBufferPath path) noexcept;

}  // namespace GstHwPathTelemetry

/// Logs once via qCWarning(LOGCAT) the first time @p FLAG flips true; subsequent trips are silent.
#define QGC_HW_WARN_ONCE(LOGCAT, FLAG, ...)                      \
    do {                                                         \
        if (!(FLAG).exchange(true, std::memory_order_relaxed)) { \
            qCWarning(LOGCAT) << __VA_ARGS__;                    \
        }                                                        \
    } while (0)

/// \brief Common base for GStreamer-backed QHwVideoBuffer subclasses.
class GstHwVideoBuffer : public QHwVideoBuffer
{
public:
    GstHwVideoBuffer(QVideoFrame::HandleType handleType, GstSample* sample, const GstVideoInfo& videoInfo,
                     QVideoFrameFormat format);
    ~GstHwVideoBuffer() override;

    QVideoFrameFormat format() const override { return _format; }

    /// CPU view of the sample through gst_video_frame_map (e.g. GL memory downloads, DMABuf mmaps). Qt's
    /// createTextures() uploads this whenever mapTextures() fails, so a failed GPU import still shows the frame.
    /// ReadOnly only. Refused when format() isn't the layout of the memory: Qt would upload it in the wrong format.
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
        const int memCount = (std::min)(int(gst_buffer_n_memory(buffer)), GstHw::kMaxPlanes);
        if (memCount <= 0)
            return false;
        for (int i = 0; i < memCount; ++i) {
            GstMemory* mem = gst_buffer_peek_memory(buffer, i);
            if (!planePred(mem))
                return false;
        }
        return true;
    }

    /// Shared mapTextures preamble; warns once per cause via @p diag flags, returns false on first failure.
    bool checkMapPreconditions(const QRhi& rhi, int expectedBackend, const QLoggingCategory& cat,
                               GstHw::MapDiagnostics& diag, GstBuffer*& outBuffer) const;

    GstSample* _sample = nullptr;
    GstVideoInfo _videoInfo = {};
    QVideoFrameFormat _format;

private:
    /// Pixel format of the sample's memory, as constructed. A subclass may advertise another one in _format for its GPU
    /// import (the DMABuf eglfs YUYV/UYVY-as-RGBA quirk); such frames have no CPU layout matching format().
    QVideoFrameFormat::PixelFormat _memoryPixelFormat;
    CpuVideoFramePool::FrameMapping _cpuView;
};
