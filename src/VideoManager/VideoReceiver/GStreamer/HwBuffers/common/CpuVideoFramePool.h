#pragma once

#include <array>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/video-frame.h>
#include <gst/video/video-info.h>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QMutex>
#include <QtCore/QSize>
#include <QtMultimedia/QAbstractVideoBuffer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

/// \brief Recycles CPU-backed QVideoFrame storage keyed by (size, pixelFormat) to avoid per-frame malloc churn.
class CpuVideoFramePool
{
public:
    static CpuVideoFramePool& instance();

    /// Returns a pool-backed frame sized to @p info, or freshly allocates on a pool miss; invalid if unsupported.
    QVideoFrame acquireFrame(const QVideoFrameFormat& format, const GstVideoInfo& info);

    /// Upper bound on decoder buffers one sink holds downstream at once, zero-copy CPU and GPU frames alike: the frame
    /// being mapped in show_frame, one pending GUI delivery (latest-frame-only), the frame Qt displays, and the
    /// previous frame, whose texture bundle keeps its sample until Qt reuses that in-flight slot (QRhi FramesInFlight
    /// is 2 on the common backends).
    static constexpr int kMaxPinnedFrames = 4;

    /// Live zero-copy frames of one sink; shared so frames Qt still holds after the sink is gone can release it.
    using PinnedFrameCounter = std::shared_ptr<std::atomic<int>>;

    /// Zero-copy CPU frame: wraps @p buffer and maps its system memory through to Qt. nullptr if not CPU-mappable or
    /// @p pinned already counts kMaxPinnedFrames (caller copies instead). Pins the decoder buffer for the frame's
    /// lifetime.
    static std::unique_ptr<QAbstractVideoBuffer> wrapZeroCopy(GstBuffer* buffer, const GstVideoInfo& info,
                                                              const QVideoFrameFormat& format,
                                                              const PinnedFrameCounter& pinned);

    struct PlaneLayout
    {
        int planeCount = 0;
        int bytesPerLine[4] = {};
        int height[4] = {};
        qsizetype planeOffset[4] = {};
        qsizetype byteSize = 0;
    };

    /// Destination plane layout mirroring @p info (strides/offsets from the decoder). planeCount==0 means unsupported.
    static PlaneLayout computeLayout(const GstVideoInfo& info);

    /// Qt map data for a frame mapped with gst_video_frame_map, each plane sized by its own row count.
    static QAbstractVideoBuffer::MapData mapDataFor(const GstVideoFrame& frame);

    /// A buffer's gst_video_frame_map, held until unmap() or destruction: the CPU view behind a buffer's
    /// QAbstractVideoBuffer::map(), or any other scoped frame map.
    class FrameMapping
    {
    public:
        FrameMapping() = default;

        ~FrameMapping() { unmap(); }

        FrameMapping(const FrameMapping&) = delete;
        FrameMapping& operator=(const FrameMapping&) = delete;

        /// Maps @p buffer with @p flags on the first call; false when that fails.
        bool mapFrame(GstBuffer* buffer, const GstVideoInfo& info, GstMapFlags flags = GST_MAP_READ);

        /// The mapped frame; valid while mapFrame() succeeded and unmap() hasn't run.
        const GstVideoFrame& frame() const { return _frame; }

        /// mapFrame() for reading, as Qt map data; empty when the map fails.
        QAbstractVideoBuffer::MapData map(GstBuffer* buffer, const GstVideoInfo& info);
        void unmap() noexcept;

    private:
        GstVideoFrame _frame = {};
        bool _mapped = false;
    };

    /// Copy @p buffer's planes into a pool-allocated QVideoFrame; invalid on stride overflow or unsupported format.
    static QVideoFrame copyFromBuffer(GstBuffer* buffer, const GstVideoInfo& videoInfo,
                                      const QVideoFrameFormat& format);

    /// Return a backing array to the pool; called by PooledCpuVideoBuffer's destructor only.
    void releaseBacking(QSize size, QVideoFrameFormat::PixelFormat format, QByteArray&& backing);

private:
    CpuVideoFramePool() = default;

    QByteArray acquireBacking(const PlaneLayout& layout, QSize size, QVideoFrameFormat::PixelFormat format);

    static constexpr int kMaxPerSlot = 4;
    // Fixed-size slot array; round-robin eviction caps memory when resolution keeps changing.
    static constexpr int kMaxSlots = 8;

    struct Slot
    {
        QSize size;
        QVideoFrameFormat::PixelFormat format = QVideoFrameFormat::Format_Invalid;
        std::array<QByteArray, kMaxPerSlot> available;
        int availableCount = 0;
    };

    QMutex _mutex;
    std::array<Slot, kMaxSlots> _slots;
    int _evictCursor = 0;
};
