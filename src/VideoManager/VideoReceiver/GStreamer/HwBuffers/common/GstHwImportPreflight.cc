#include "GstHwImportPreflight.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <array>

#include <QtCore/QMutex>

#include <private/qvideotexturehelper_p.h>

#include "GstHwPathTelemetry.h"

namespace {

/// Preflight inputs are stable within a caps epoch — memoize the last verdict per path so steady-state
/// frames skip the per-plane RHI queries. A new caps/QRhi changes the key and recomputes naturally.
struct PreflightMemo {
    QRhi* rhi = nullptr;
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
    QSize size;
    QRhiTexture::Flags flags;
    bool result = false;
    bool valid = false;
};

constexpr std::size_t kPathCount = static_cast<std::size_t>(HwVideoBufferPath::Vulkan) + 1;
QMutex s_memoMutex;
std::array<PreflightMemo, kPathCount> s_memo{};

}  // namespace

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
    bool ok = false;
    const std::size_t idx = static_cast<std::size_t>(path);
    if (idx < s_memo.size()) {
        QMutexLocker lock(&s_memoMutex);
        PreflightMemo& memo = s_memo[idx];
        if (memo.valid && (memo.rhi == rhi) && (memo.pixelFormat == pixelFormat) && (memo.size == size) &&
            (memo.flags == flags)) {
            ok = memo.result;
        } else {
            ok = canImportPlanes(rhi, pixelFormat, size, flags);
            memo = {rhi, pixelFormat, size, flags, ok, true};
        }
    } else {
        ok = canImportPlanes(rhi, pixelFormat, size, flags);
    }
    if (ok) {
        return true;
    }
    // Recorded per call (not per epoch) to keep fallback counts frame-accurate.
    GstHwPathTelemetry::recordFallbackReason(path, GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
    return false;
}

}  // namespace GstHwImportPreflight

#endif  // QGC_HAS_ANY_GPU_PATH
