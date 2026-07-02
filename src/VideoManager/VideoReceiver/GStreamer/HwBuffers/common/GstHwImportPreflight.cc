#include "GstHwImportPreflight.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <private/qvideotexturehelper_p.h>

#include "GstHwPathTelemetry.h"

namespace GstHwImportPreflight {

bool canImportTexture(QRhi* rhi, QRhiTexture::Format fmt, const QSize& size, QRhiTexture::Flags flags) noexcept
{
    if (!rhi) {
        return true;
    }
    if (fmt == QRhiTexture::UnknownFormat) {
        return false;
    }
    if (!rhi->isTextureFormatSupported(fmt, flags)) {
        return false;
    }
    const int maxDim = rhi->resourceLimit(QRhi::TextureSizeMax);
    if (maxDim > 0 && (size.width() > maxDim || size.height() > maxDim)) {
        return false;
    }
    return true;
}

bool canImportPlanes(QRhi* rhi, QVideoFrameFormat::PixelFormat pixelFormat, const QSize& size,
                     QRhiTexture::Flags flags) noexcept
{
    if (!rhi) {
        return true;
    }
    const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
    if (!desc) {
        return false;
    }
    // Disable the helper's own format fallback chain so the pre-flight tests the exact format createFrom() will use.
    using FallbackPolicy = QVideoTextureHelper::TextureDescription::FallbackPolicy;
    for (int plane = 0; plane < desc->nplanes; ++plane) {
        const QRhiTexture::Format fmt = desc->rhiTextureFormat(plane, rhi, FallbackPolicy::Disable);
        const QSize planeSize = desc->rhiPlaneSize(size, plane, rhi);
        if (!canImportTexture(rhi, fmt, planeSize, flags)) {
            return false;
        }
    }
    return true;
}

bool preflightOrRecord(QRhi* rhi, HwVideoBufferPath path, QVideoFrameFormat::PixelFormat pixelFormat,
                       const QSize& size, QRhiTexture::Flags flags) noexcept
{
    if (canImportPlanes(rhi, pixelFormat, size, flags)) {
        return true;
    }
    GstHwPathTelemetry::recordFallbackReason(path, GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
    return false;
}

}  // namespace GstHwImportPreflight

#endif  // QGC_HAS_ANY_GPU_PATH
