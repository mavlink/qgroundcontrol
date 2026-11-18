#include "GstHwVideoBuffer.h"

#include <rhi/qrhi.h>

#include "GstHwPathTelemetry.h"

QVideoFrameTexturesUPtr GstHwPathTelemetry::fail(HwVideoBufferPath path) noexcept
{
    record(path, Counter::MapFailures);
    return {};
}

bool GstHw::texturePadsPicture(QSize textureSize, QSize picture, bool evenSizes)
{
    const auto rounded = [evenSizes](int extent) { return evenSizes ? ((extent + 1) & ~1) : extent; };
    return (textureSize.width() > rounded(picture.width())) || (textureSize.height() > rounded(picture.height()));
}

QVideoFrameFormat GstHw::withPixelFormat(const QVideoFrameFormat& format, QVideoFrameFormat::PixelFormat pixelFormat)
{
    QVideoFrameFormat out(format.frameSize(), pixelFormat);
    out.setViewport(format.viewport());
    out.setScanLineDirection(format.scanLineDirection());
    out.setStreamFrameRate(format.streamFrameRate());
    out.setColorSpace(format.colorSpace());
    out.setColorTransfer(format.colorTransfer());
    out.setColorRange(format.colorRange());
    out.setMirrored(format.isMirrored());
    out.setRotation(format.rotation());
    out.setMaxLuminance(format.maxLuminance());
    return out;
}

GstHwVideoBuffer::GstHwVideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, QVideoFrameFormat format)
    : QHwVideoBuffer(QVideoFrame::RhiTextureHandle)
    , _sample(sample ? gst_sample_ref(sample) : nullptr)
    , _videoInfo(videoInfo)
    , _format(std::move(format))
    , _memoryPixelFormat(_format.pixelFormat())
{
    // Crop is applied by the renderer via QVideoFrameFormat::viewport(); see frameGeometry (GstQgcFrameMap).
}

GstHwVideoBuffer::~GstHwVideoBuffer()
{
    GstHwVideoBuffer::unmap();
    if (_sample) {
        gst_sample_unref(_sample);
    }
}

QAbstractVideoBuffer::MapData GstHwVideoBuffer::map(QVideoFrame::MapMode mode)
{
    // An external-OES frame has no CPU layout Qt could upload (it would read it as RGBA8), nor does a frame advertising
    // another format than its memory holds (Qt would read past the end of a narrower layout).
    const QVideoFrameFormat::PixelFormat advertised = _format.pixelFormat();
    if ((mode != QVideoFrame::ReadOnly) || (advertised == QVideoFrameFormat::Format_SamplerExternalOES) ||
        (advertised != _memoryPixelFormat)) {
        return {};
    }
    return _cpuView.map(_sample ? gst_sample_get_buffer(_sample) : nullptr, _videoInfo);
}

void GstHwVideoBuffer::unmap()
{
    _cpuView.unmap();
}

bool GstHwVideoBuffer::checkMapPreconditions(const QRhi& rhi, int expectedBackend, const QLoggingCategory& cat,
                                             GstHw::MapDiagnostics& diag, GstBuffer*& outBuffer) const
{
    if (!_sample) {
        QGC_HW_WARN_ONCE(cat, diag.loggedNullSample, "mapTextures: GstSample is null");
        return false;
    }
    if (!rhi.thread()->isCurrentThread()) {
        QGC_HW_WARN_ONCE(cat, diag.loggedWrongThread, "mapTextures: called outside QRhi render thread");
        return false;
    }
    if (static_cast<int>(rhi.backend()) != expectedBackend) {
        QGC_HW_WARN_ONCE(
            cat, diag.loggedBadBackend,
            "mapTextures: QRhi backend is" << rhi.backendName() << "(backend id" << expectedBackend << "required)");
        return false;
    }
    outBuffer = gst_sample_get_buffer(_sample);
    if (!outBuffer) {
        QGC_HW_WARN_ONCE(cat, diag.loggedNullBuffer, "mapTextures: GstSample has no buffer");
        return false;
    }
    return true;
}
