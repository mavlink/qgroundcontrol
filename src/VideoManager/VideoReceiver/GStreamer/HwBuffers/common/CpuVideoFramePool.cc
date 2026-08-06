#include "CpuVideoFramePool.h"

#include <QtCore/QLoggingCategory>
#include <QtMultimedia/QAbstractVideoBuffer>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <gst/video/video.h>
#include <memory>
#include <utility>

#include "QGCLoggingCategory.h"
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "QGCRhiCapture.h"
#endif

QGC_LOGGING_CATEGORY(CpuVideoFramePoolLog, "Video.GStreamer.HwBuffers.CpuPool")

namespace {

class PooledCpuVideoBuffer final : public QAbstractVideoBuffer
{
public:
    PooledCpuVideoBuffer(QVideoFrameFormat format, CpuVideoFramePool::PlaneLayout layout, QByteArray backing)
        : _format(std::move(format)), _layout(layout), _backing(std::move(backing))
    {}

    ~PooledCpuVideoBuffer() override
    {
        if (!_backing.isEmpty()) {
            CpuVideoFramePool::instance().releaseBacking(_format.frameSize(), _format.pixelFormat(),
                                                         std::move(_backing));
        }
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        if (mode == QVideoFrame::NotMapped) {
            return {};
        }
        MapData data;
        data.planeCount = _layout.planeCount;
        uchar* base = reinterpret_cast<uchar*>(_backing.data());
        for (int i = 0; i < _layout.planeCount; ++i) {
            data.data[i] = base + _layout.planeOffset[i];
            data.bytesPerLine[i] = _layout.bytesPerLine[i];
            data.dataSize[i] = static_cast<qsizetype>(_layout.bytesPerLine[i]) * _layout.height[i];
        }
        return data;
    }

    QVideoFrameFormat format() const override { return _format; }

private:
    QVideoFrameFormat _format;
    CpuVideoFramePool::PlaneLayout _layout;
    QByteArray _backing;
};

// Refs the source GstBuffer and maps its system memory through to Qt with no copy. The decoder buffer stays pinned for
// the frame's lifetime — the same tradeoff the GPU paths already make by anchoring the sample.
class MapThroughGstVideoBuffer final : public QAbstractVideoBuffer
{
public:
    // Outstanding zero-copy buffers each pin a decoder buffer; cap how many can be in flight before
    // wrapZeroCopy falls back to a copy so a compositor stall can't drain the decoder's HW pool. The cap is
    // derived from the RHI's frames-in-flight.
    static int maxLive() noexcept
    {
        static const int s_max = []() -> int {
            constexpr int kFloor = 6;
#if defined(QGC_HAS_ANY_GPU_PATH)
            // Snapshot populated render-side; never dereference QRhi from this (streaming) thread.
            int fif = QGCRhiCapture::deviceSnapshot().framesInFlight.load(std::memory_order_acquire);
            if (fif <= 0) {
                fif = 2;  // QRhi's typical default before the snapshot exists
            }
            return std::clamp(fif * 2, kFloor, 16);
#else
            return kFloor;
#endif
        }();
        return s_max;
    }
    static std::atomic<int>& liveCount()
    {
        static std::atomic<int> s_live{0};
        return s_live;
    }

    MapThroughGstVideoBuffer(GstBuffer* buffer, const GstVideoInfo& info, QVideoFrameFormat format)
        : _buffer(gst_buffer_ref(buffer)), _info(info), _format(std::move(format))
    {
        liveCount().fetch_add(1, std::memory_order_relaxed);
    }

    ~MapThroughGstVideoBuffer() override
    {
        if (_mapped) {
            gst_video_frame_unmap(&_frame);
        }
        gst_buffer_unref(_buffer);
        liveCount().fetch_sub(1, std::memory_order_relaxed);
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        if (mode == QVideoFrame::NotMapped) {
            return {};
        }
        if (mode != QVideoFrame::ReadOnly) {
            return {};
        }
        if (!_mapped) {
            if (!gst_video_frame_map(&_frame, &_info, _buffer, GST_MAP_READ)) {
                return {};
            }
            _mapped = true;
        }
        MapData data;
        data.planeCount = GST_VIDEO_FRAME_N_PLANES(&_frame);
        for (int i = 0; i < data.planeCount; ++i) {
            data.data[i] = static_cast<uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&_frame, i));
            data.bytesPerLine[i] = GST_VIDEO_FRAME_PLANE_STRIDE(&_frame, i);
            data.dataSize[i] = static_cast<qsizetype>(GST_VIDEO_FRAME_PLANE_STRIDE(&_frame, i)) *
                               GST_VIDEO_FRAME_COMP_HEIGHT(&_frame, i);
        }
        return data;
    }

    void unmap() override
    {
        if (_mapped) {
            gst_video_frame_unmap(&_frame);
            _mapped = false;
        }
    }

    QVideoFrameFormat format() const override { return _format; }

private:
    GstBuffer* _buffer;
    GstVideoInfo _info;
    QVideoFrameFormat _format;
    GstVideoFrame _frame = {};
    bool _mapped = false;
};

}  // namespace

CpuVideoFramePool& CpuVideoFramePool::instance()
{
    static CpuVideoFramePool s_pool;
    return s_pool;
}

CpuVideoFramePool::PlaneLayout CpuVideoFramePool::computeLayout(const GstVideoInfo& info)
{
    PlaneLayout L = {};
    const int n = GST_VIDEO_INFO_N_PLANES(&info);
    if (n <= 0 || n > 4) {
        return L;
    }
    L.planeCount = n;
    for (int i = 0; i < n; ++i) {
        L.bytesPerLine[i] = GST_VIDEO_INFO_PLANE_STRIDE(&info, i);
        L.planeOffset[i] = static_cast<qsizetype>(GST_VIDEO_INFO_PLANE_OFFSET(&info, i));
        L.height[i] = static_cast<int>(GST_VIDEO_INFO_COMP_HEIGHT(&info, i));
    }
    L.byteSize = static_cast<qsizetype>(GST_VIDEO_INFO_SIZE(&info));
    return L;
}

QByteArray CpuVideoFramePool::acquireBacking(const PlaneLayout& layout, QSize size,
                                             QVideoFrameFormat::PixelFormat format)
{
    QMutexLocker lock(&_mutex);
    for (std::size_t i = 0; i < _slots.size(); ++i) {
        auto& slot = _slots[i];
        if (slot.size == size && slot.format == format) {
            // (size,format) implies a fixed byteSize; discard any stale mis-sized backing and keep scanning rather
            // than stranding it, which would force a permanent miss loop.
            while (slot.availableCount > 0) {
                QByteArray b = std::move(slot.available[--slot.availableCount]);
                if (b.size() == layout.byteSize) {
                    _hits.fetch_add(1, std::memory_order_relaxed);
                    return b;
                }
            }
            break;
        }
    }
    _misses.fetch_add(1, std::memory_order_relaxed);
    // Qt::Uninitialized skips the value-init memset; caller overwrites every plane.
    return QByteArray(layout.byteSize, Qt::Uninitialized);
}

void CpuVideoFramePool::releaseBacking(QSize size, QVideoFrameFormat::PixelFormat format, QByteArray&& backing)
{
    QMutexLocker lock(&_mutex);
    for (std::size_t i = 0; i < _slots.size(); ++i) {
        auto& slot = _slots[i];
        if (slot.size == size && slot.format == format) {
            if (slot.availableCount < kMaxPerSlot) {
                slot.available[slot.availableCount++] = std::move(backing);
            }
            return;
        }
    }
    // No slot for this (size,format): claim an empty slot or round-robin evict to stay bounded.
    Slot* target = nullptr;
    for (std::size_t i = 0; i < _slots.size(); ++i) {
        auto& slot = _slots[i];
        if (slot.format == QVideoFrameFormat::Format_Invalid) {
            target = &slot;
            break;
        }
    }
    if (!target) {
        target = &_slots[_evictCursor];
        _evictCursor = (_evictCursor + 1) % kMaxSlots;
        *target = Slot{};
    }
    target->size = size;
    target->format = format;
    target->available[0] = std::move(backing);
    target->availableCount = 1;
}

QVideoFrame CpuVideoFramePool::acquireFrame(const QVideoFrameFormat& format, const GstVideoInfo& info)
{
    const PlaneLayout layout = computeLayout(info);
    if (layout.planeCount == 0 || layout.byteSize <= 0) {
        return {};
    }
    QByteArray backing = acquireBacking(layout, format.frameSize(), format.pixelFormat());
    auto buffer = std::make_unique<PooledCpuVideoBuffer>(format, layout, std::move(backing));
    return QVideoFrame(std::move(buffer));
}

std::unique_ptr<QAbstractVideoBuffer> CpuVideoFramePool::wrapZeroCopy(GstBuffer* buffer, const GstVideoInfo& info,
                                                                      const QVideoFrameFormat& format)
{
    if (!buffer || !format.isValid()) {
        return nullptr;
    }
    if (MapThroughGstVideoBuffer::liveCount().load(std::memory_order_relaxed) >= MapThroughGstVideoBuffer::maxLive()) {
        return nullptr;
    }
    auto buf = std::make_unique<MapThroughGstVideoBuffer>(buffer, info, format);
    // Probe mappability: system memory always maps, GPU memory that slipped past the factory does not (caller copies).
    // Keep the mapping rather than unmapping — the sink's subsequent map() reuses it instead of re-running the map.
    const auto probe = buf->map(QVideoFrame::ReadOnly);
    if (probe.planeCount <= 0) {
        return nullptr;
    }
    return buf;
}

QVideoFrame CpuVideoFramePool::copyFromBuffer(GstBuffer* buffer, const GstVideoInfo& videoInfo,
                                              const QVideoFrameFormat& format)
{
    if (!buffer || !format.isValid()) {
        return {};
    }

    GstVideoFrame gstFrame;
    if (!gst_video_frame_map(&gstFrame, &videoInfo, buffer, GST_MAP_READ)) {
        static std::atomic<quint64> s_failCount{0};
        const quint64 count = s_failCount.fetch_add(1, std::memory_order_relaxed) + 1;
        if ((count & 0x3F) == 1) {
            qCWarning(CpuVideoFramePoolLog) << "copyFromBuffer: gst_video_frame_map failed (count=" << count << ")";
        }
        return {};
    }

    QVideoFrame videoFrame = instance().acquireFrame(format, videoInfo);
    if (!videoFrame.isValid() || !videoFrame.map(QVideoFrame::WriteOnly)) {
        gst_video_frame_unmap(&gstFrame);
        return {};
    }

    const int srcPlanes = GST_VIDEO_INFO_N_PLANES(&videoInfo);
    if (srcPlanes != videoFrame.planeCount()) {
        static std::atomic<bool> s_warnedPlaneMismatch{false};
        if (!s_warnedPlaneMismatch.exchange(true, std::memory_order_relaxed)) {
            qCWarning(CpuVideoFramePoolLog) << "copyFromBuffer: plane-count mismatch src" << srcPlanes << "dst"
                                            << videoFrame.planeCount() << "— dropping frame";
        }
        videoFrame.unmap();
        gst_video_frame_unmap(&gstFrame);
        return {};
    }

    for (int p = 0; p < srcPlanes; ++p) {
        const int dstStride = videoFrame.bytesPerLine(p);
        const int srcStride = GST_VIDEO_FRAME_PLANE_STRIDE(&gstFrame, p);
        const int planeHeight = GST_VIDEO_FRAME_COMP_HEIGHT(&gstFrame, p);
        const int activeRowBytes =
            GST_VIDEO_FRAME_COMP_WIDTH(&gstFrame, p) * GST_VIDEO_FRAME_COMP_PSTRIDE(&gstFrame, p);
        // Reject before copy: a dst stride too narrow for a source row would tear the plane.
        if (activeRowBytes > dstStride) {
            static std::atomic<bool> s_warnedStrideOverflow{false};
            if (!s_warnedStrideOverflow.exchange(true, std::memory_order_relaxed)) {
                qCWarning(CpuVideoFramePoolLog) << "copyFromBuffer: plane" << p << "activeRowBytes" << activeRowBytes
                                                << "> dstStride" << dstStride << "— dropping frame";
            }
            videoFrame.unmap();
            gst_video_frame_unmap(&gstFrame);
            return {};
        }
        const uchar* src = static_cast<const uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&gstFrame, p));
        uchar* dst = videoFrame.bits(p);
        if (!dst) {
            continue;
        }
        if (srcStride == dstStride && activeRowBytes == srcStride) {
            std::memcpy(dst, src, static_cast<size_t>(planeHeight) * srcStride);
        } else {
            for (int y = 0; y < planeHeight; ++y) {
                std::memcpy(dst + y * dstStride, src + y * srcStride, static_cast<size_t>(activeRowBytes));
            }
        }
    }

    videoFrame.unmap();
    gst_video_frame_unmap(&gstFrame);
    return videoFrame;
}
