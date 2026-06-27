#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <algorithm>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <private/qhwvideobuffer_p.h>

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

    MapData map(QVideoFrame::MapMode /*mode*/) override { return {}; }

    /// Streaming-thread sanity check on per-plane handles; failure routes to CPU memcpy.
    virtual bool validatePlaneHandles() const { return true; }

    /// Human-readable GPU path identifier (e.g. "DMABuf"); string literal, safe from any thread.
    virtual const char* storageTag() const { return "Unknown"; }

    /// Transfers GstSample ownership out for early pool-slot release in mapTextures.
    GstSample* takeSample() noexcept
    {
        GstSample* s = _sample;
        _sample = nullptr;
        return s;
    }

    /// One-shot "first zero-copy success" info line; silent after @p flag first flips.
    static void logFirstSuccess(std::atomic<bool>& flag, const QLoggingCategory& cat, const char* tag, QSize frameSize,
                                QVideoFrameFormat::PixelFormat pixelFormat, int planes);

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
};
