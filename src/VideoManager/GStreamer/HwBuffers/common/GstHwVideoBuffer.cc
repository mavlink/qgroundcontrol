#include "GstHwVideoBuffer.h"

#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>
#include <utility>

#include "GstHwPathTelemetry.h"

namespace {

// What a QGC_HW_ONCE() latch holds once its use has run; never 0, which a latch starts with.
std::atomic<quint32> s_onceGeneration{1};

// GstHwVideoBuffer::wrapTextures()'s once-per-path notices.
std::array<std::atomic<quint32>, GstHw::kPathCount> s_wrapFailureLatches{};
std::array<std::atomic<quint32>, GstHw::kPathCount> s_wrapSuccessLatches{};

}  // namespace

bool GstHw::firstSinceRearm(std::atomic<quint32>& latch) noexcept
{
    const quint32 generation = s_onceGeneration.load(std::memory_order_relaxed);
    return latch.exchange(generation, std::memory_order_relaxed) != generation;
}

void GstHw::rearmOnceLatches() noexcept
{
    s_onceGeneration.fetch_add(1, std::memory_order_relaxed);
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

GstHwTextureHandles::~GstHwTextureHandles()
{
    if (_sample) {
        gst_sample_unref(_sample);
    }
}

quint64 GstHwTextureHandles::textureHandle(QRhi& /*rhi*/, int plane)
{
    return ((plane >= 0) && (plane < GstHw::kMaxPlanes)) ? _handles[std::size_t(plane)] : 0;
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

QVideoFrameTexturesUPtr GstHwVideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& oldTextures)
{
    QVideoFrameTexturesUPtr textures;
    {
        const GstHwPathTelemetry::ScopedMapTimer mapTimer(path());
        textures = importTextures(rhi, oldTextures);
    }
    if (textures) {
        return textures;
    }
    GstHwPathTelemetry::record(path(), GstHwPathTelemetry::Counter::MapFailures);
    // A mapping of its own: map()'s belongs to the QVideoFrame, which serialises its calls.
    GstHw::FrameMapping probe;
    if (hasCpuLayout() && probe.mapFrame(_sample ? gst_sample_get_buffer(_sample) : nullptr, _videoInfo)) {
        return {};
    }
    return std::move(oldTextures);
}

QVideoFrameTexturesUPtr GstHwVideoBuffer::wrapTextures(QRhi& rhi, std::unique_ptr<GstHwTextureHandles> handles,
                                                       const QLoggingCategory& log, const char* successTag) const
{
    const HwVideoBufferPath importPath = path();
    const QVideoFrameFormat::PixelFormat pixelFormat = _format.pixelFormat();
    const QSize frameSize = _format.frameSize();
    QVideoFrameTexturesUPtr textures =
        QVideoTextureHelper::createTexturesFromHandles(std::move(handles), rhi, pixelFormat, frameSize);
    if (!textures) {
        // A plane without a handle, or one QRhi refused: a partial set would render with missing planes.
        GstHwPathTelemetry::recordFallbackReason(importPath, GstHwPathTelemetry::HwFallbackReason::MapFailed);
        if (GstHw::firstSinceRearm(s_wrapFailureLatches[std::size_t(importPath)])) {
            qCWarning(log) << "QRhiTexture::createFrom failed (format=" << int(pixelFormat) << "size=" << frameSize
                           << ")";
        }
        return {};
    }
    if (successTag && GstHw::firstSinceRearm(s_wrapSuccessLatches[std::size_t(importPath)])) {
        const auto* const description = QVideoTextureHelper::textureDescription(pixelFormat);
        qCInfo(log) << "First" << successTag << "zero-copy mapTextures success: size=" << frameSize
                    << "format=" << int(pixelFormat) << "planes=" << (description ? description->nplanes : 0);
    }
    return textures;
}

bool GstHwVideoBuffer::hasCpuLayout() const
{
    const QVideoFrameFormat::PixelFormat advertised = _format.pixelFormat();
    return (advertised != QVideoFrameFormat::Format_SamplerExternalOES) && (advertised == _memoryPixelFormat);
}

QAbstractVideoBuffer::MapData GstHwVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if ((mode != QVideoFrame::ReadOnly) || !hasCpuLayout()) {
        return {};
    }
    return _cpuView.map(_sample ? gst_sample_get_buffer(_sample) : nullptr, _videoInfo);
}

void GstHwVideoBuffer::unmap()
{
    _cpuView.unmap();
}

bool GstHwVideoBuffer::checkMapPreconditions(const QRhi& rhi, int expectedBackend, const QLoggingCategory& cat,
                                             GstBuffer*& outBuffer) const
{
    if (!_sample) {
        QGC_HW_WARN_ONCE(cat, "mapTextures: GstSample is null");
        return false;
    }
    if (!rhi.thread()->isCurrentThread()) {
        QGC_HW_WARN_ONCE(cat, "mapTextures: called outside QRhi render thread");
        return false;
    }
    if (static_cast<int>(rhi.backend()) != expectedBackend) {
        QGC_HW_WARN_ONCE(cat, "mapTextures: QRhi backend is" << rhi.backendName() << "(backend id" << expectedBackend
                                                             << "required)");
        return false;
    }
    outBuffer = gst_sample_get_buffer(_sample);
    if (!outBuffer) {
        QGC_HW_WARN_ONCE(cat, "mapTextures: GstSample has no buffer");
        return false;
    }
    return true;
}
