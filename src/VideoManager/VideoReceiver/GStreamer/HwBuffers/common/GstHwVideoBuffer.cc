#include "GstHwVideoBuffer.h"

#include <rhi/qrhi.h>

GstHwVideoBuffer::GstHwVideoBuffer(QVideoFrame::HandleType handleType, GstSample* sample, const GstVideoInfo& videoInfo,
                                   QVideoFrameFormat format)
    : QHwVideoBuffer(handleType),
      _sample(sample ? gst_sample_ref(sample) : nullptr),
      _videoInfo(videoInfo),
      _format(std::move(format))
{
    // Crop is applied by the renderer via QVideoFrameFormat::viewport(); see GStreamerFrameMap::applyCropMeta.
}

GstHwVideoBuffer::~GstHwVideoBuffer()
{
    if (_sample) {
        gst_sample_unref(_sample);
    }
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

void GstHwVideoBuffer::logFirstSuccess(std::atomic<bool>& flag, const QLoggingCategory& cat, const char* tag,
                                       QSize frameSize, QVideoFrameFormat::PixelFormat pixelFormat, int planes)
{
    if (!flag.exchange(true, std::memory_order_relaxed)) {
        qCInfo(cat) << "First" << tag << "zero-copy mapTextures success: size=" << frameSize
                    << "format=" << int(pixelFormat) << "planes=" << planes;
    }
}
