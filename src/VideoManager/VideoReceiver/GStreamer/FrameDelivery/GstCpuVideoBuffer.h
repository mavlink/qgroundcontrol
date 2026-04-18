#pragma once

#include <QtMultimedia/QAbstractVideoBuffer>
#include <QtMultimedia/QVideoFrameFormat>
#include <gst/video/video-frame.h>
#include <gst/video/video-info.h>

/// Zero-copy QAbstractVideoBuffer wrapping a GstBuffer for CPU-mapped frames.
///
/// Holds a GstBuffer ref and maps it on demand via gst_video_frame_map().
/// The GstBuffer's memory is returned directly to Qt's texture upload
/// without an intermediate copy.
///
/// Thread-safety: QAbstractVideoBuffer::map() is documented as non-concurrent
/// per instance — Qt serializes map/unmap on the owning QVideoFrame's internal
/// refcount. We only assert nested-map defensively; no lock is held on the hot
/// path (one assert per frame, zero lock acquisition).
///
/// Usage:
///   auto buf = std::make_unique<GstCpuVideoBuffer>(buffer, videoInfo, qtFormat);
///   QVideoFrame frame(std::move(buf));  // buf->format() becomes frame format
///   sink->setVideoFrame(frame);
class GstCpuVideoBuffer : public QAbstractVideoBuffer
{
public:
    /// Takes a ref on @p buffer (released in destructor).
    GstCpuVideoBuffer(GstBuffer* buffer, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format);
    ~GstCpuVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;
    QVideoFrameFormat format() const override;

private:
    GstBuffer* _buffer = nullptr;
    GstVideoInfo _videoInfo;
    GstVideoFrame _mappedFrame{};
    QVideoFrameFormat _format;
    bool _mapped = false;
};
