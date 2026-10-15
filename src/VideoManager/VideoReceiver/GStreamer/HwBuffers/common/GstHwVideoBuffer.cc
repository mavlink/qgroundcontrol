#include "GstHwVideoBuffer.h"

#include <rhi/qrhi.h>

#include "GstHwPathTelemetry.h"

QVideoFrameTexturesUPtr GstHwPathTelemetry::fail(HwVideoBufferPath path) noexcept
{
    record(path, Counter::MapFailures);
    return {};
}

GstHwVideoBuffer::GstHwVideoBuffer(QVideoFrame::HandleType handleType, GstSample* sample, const GstVideoInfo& videoInfo,
                                   QVideoFrameFormat format)
    : QHwVideoBuffer(handleType)
    , _sample(sample ? gst_sample_ref(sample) : nullptr)
    , _videoInfo(videoInfo)
    , _format(std::move(format))
    , _memoryPixelFormat(_format.pixelFormat())
{
    // Crop is applied by the renderer via QVideoFrameFormat::viewport(); see applyCropMeta (GstQgcFrameMap).
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
        if (!diag.loggedNullSample.exchange(true, std::memory_order_relaxed))
            qCWarning(cat) << "mapTextures: GstSample is null";
        return false;
    }
    if (!rhi.thread()->isCurrentThread()) {
        if (!diag.loggedWrongThread.exchange(true, std::memory_order_relaxed))
            qCWarning(cat) << "mapTextures: called outside QRhi render thread";
        return false;
    }
    if (static_cast<int>(rhi.backend()) != expectedBackend) {
        if (!diag.loggedBadBackend.exchange(true, std::memory_order_relaxed))
            qCWarning(cat) << "mapTextures: QRhi backend is" << rhi.backendName() << "(backend id" << expectedBackend
                           << "required)";
        return false;
    }
    outBuffer = gst_sample_get_buffer(_sample);
    if (!outBuffer) {
        if (!diag.loggedNullBuffer.exchange(true, std::memory_order_relaxed))
            qCWarning(cat) << "mapTextures: GstSample has no buffer";
        return false;
    }
    return true;
}
