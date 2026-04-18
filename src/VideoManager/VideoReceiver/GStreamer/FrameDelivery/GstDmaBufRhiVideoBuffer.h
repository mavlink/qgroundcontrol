#pragma once

#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include <QtMultimedia/QVideoFrameFormat>
#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include "GstDmaBufVideoBuffer.h"

#ifdef QGC_GST_DMABUF

/// Hardware-facing DMA-BUF wrapper for Qt's video renderer.
///
/// This class is the private-Qt boundary for true hardware video frames. It
/// advertises QVideoFrame::RhiTextureHandle through QHwVideoBuffer and tries to
/// import DMA-BUF planes as QRhi textures when Qt renders the frame. If the
/// active QRhi/backend cannot import the buffer, map() delegates to the stable
/// mmap-based GstDmaBufVideoBuffer fallback.
class GstDmaBufRhiVideoBuffer : public QHwVideoBuffer
{
public:
    GstDmaBufRhiVideoBuffer(GstBuffer* buffer, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format);
    ~GstDmaBufRhiVideoBuffer() override;

    MapData map(QVideoFrame::MapMode mode) override;
    void unmap() override;
    [[nodiscard]] QVideoFrameFormat format() const override;

    [[nodiscard]] bool isDmaBuf() const override;
    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures) override;

private:
    GstBuffer* _buffer = nullptr;
    GstVideoInfo _videoInfo;
    QVideoFrameFormat _format;
    GstDmaBufVideoBuffer _cpuFallback;
};

#endif  // QGC_GST_DMABUF
