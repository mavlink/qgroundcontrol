#include "GstHwVideoBuffer.h"

#include <rhi/qrhi.h>

#include "GstHwPathTelemetry.h"

QVideoFrameTexturesUPtr GstHwPathTelemetry::fail(HwVideoBufferPath path) noexcept
{
    recordMapFailure(path);
    return {};
}

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
    GstHwVideoBuffer::unmap();
    if (_sample) {
        gst_sample_unref(_sample);
    }
}

QAbstractVideoBuffer::MapData GstHwVideoBuffer::map(QVideoFrame::MapMode mode)
{
    // An external-OES frame has no CPU layout Qt could upload (it would read it as RGBA8).
    if ((mode != QVideoFrame::ReadOnly) || (_format.pixelFormat() == QVideoFrameFormat::Format_SamplerExternalOES)) {
        return {};
    }
    if (!_cpuMapped) {
        GstBuffer* const buffer = _sample ? gst_sample_get_buffer(_sample) : nullptr;
        if (!buffer || !gst_video_frame_map(&_cpuFrame, &_videoInfo, buffer, GST_MAP_READ)) {
            return {};
        }
        _cpuMapped = true;
    }
    MapData data;
    data.planeCount = static_cast<int>(GST_VIDEO_FRAME_N_PLANES(&_cpuFrame));
    for (int i = 0; i < data.planeCount; ++i) {
        data.data[i] = static_cast<uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&_cpuFrame, i));
        data.bytesPerLine[i] = GST_VIDEO_FRAME_PLANE_STRIDE(&_cpuFrame, i);
        data.dataSize[i] = static_cast<qsizetype>(GST_VIDEO_FRAME_PLANE_STRIDE(&_cpuFrame, i)) *
                           GST_VIDEO_FRAME_COMP_HEIGHT(&_cpuFrame, i);
    }
    return data;
}

void GstHwVideoBuffer::unmap()
{
    if (_cpuMapped) {
        gst_video_frame_unmap(&_cpuFrame);
        _cpuMapped = false;
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
