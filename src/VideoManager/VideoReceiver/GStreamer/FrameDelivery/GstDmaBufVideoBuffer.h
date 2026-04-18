#pragma once

#include <QtMultimedia/QAbstractVideoBuffer>
#include <QtMultimedia/QVideoFrameFormat>
#include <gst/gst.h>
#include <gst/video/video-info.h>

#ifdef QGC_GST_DMABUF

/// QAbstractVideoBuffer wrapping a DMA-BUF-backed GstBuffer.
///
/// When a hardware decoder (vah264dec, v4l2h264dec, nvmm fallback → dmabuf)
/// outputs DMA-BUF memory, this buffer mmaps the dmabuf file descriptor and
/// hands the userspace pointer + stride to Qt. The win over the CPU path is
/// that decodebin does NOT insert a software `videoconvert` to copy DMA-BUF
/// → system memory — we accept DMA-BUF caps natively.
///
/// This is not true GPU zero-copy. It is the Linux DMA-BUF mmap fallback used
/// by GstDmaBufRhiVideoBuffer when the active QRhi cannot import the buffer.
///
/// Thread-safety: same contract as GstCpuVideoBuffer — Qt serializes map/unmap
/// per instance; we assert against nested map without taking a lock.
class GstDmaBufVideoBuffer : public QAbstractVideoBuffer
{
public:
    /// Takes a ref on @p buffer (released in destructor). Caller has already
    /// verified buffer memory is DMA-BUF via gst_is_dmabuf_memory().
    GstDmaBufVideoBuffer(GstBuffer* buffer, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format);
    ~GstDmaBufVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;
    [[nodiscard]] QVideoFrameFormat format() const override;

private:
    /// Actual munmap logic — called from destructor and map() failure path.
    void _doUnmap();

    GstBuffer* _buffer = nullptr;
    GstVideoInfo _videoInfo;
    QVideoFrameFormat _format;
    // Per-plane mmap'd pointers and lengths. At most GST_VIDEO_MAX_PLANES (4).
    void* _planeData[4] = {nullptr, nullptr, nullptr, nullptr};
    size_t _planeSize[4] = {0, 0, 0, 0};
    int _planeStride[4] = {0, 0, 0, 0};
    int _planeCount = 0;
    bool _mapped = false;
};

#endif  // QGC_GST_DMABUF
