#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMutex>
#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <array>
#include <atomic>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

class QAbstractVideoBuffer;

/// \brief Recycles CPU-backed QVideoFrame storage keyed by (size, pixelFormat) to avoid per-frame malloc churn.
class CpuVideoFramePool
{
public:
    static CpuVideoFramePool& instance();

    /// Returns a pool-backed frame sized to @p info, or freshly allocates on a pool miss; invalid if unsupported.
    QVideoFrame acquireFrame(const QVideoFrameFormat& format, const GstVideoInfo& info);

    /// Zero-copy CPU frame: wraps @p buffer and maps its system memory through to Qt. nullptr if not CPU-mappable
    /// (caller copies instead). Pins the decoder buffer for the frame's lifetime.
    static std::unique_ptr<QAbstractVideoBuffer> wrapZeroCopy(GstBuffer* buffer, const GstVideoInfo& info,
                                                              const QVideoFrameFormat& format);

    /// Telemetry: total acquireFrame() calls satisfied from the pool vs. fresh alloc.
    quint64 hitCount() const noexcept { return _hits.load(std::memory_order_relaxed); }

    quint64 missCount() const noexcept { return _misses.load(std::memory_order_relaxed); }

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
    std::atomic<quint64> _hits{0};
    std::atomic<quint64> _misses{0};
};
