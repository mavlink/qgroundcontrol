#include "GstHwImportPreflight.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <algorithm>
#include <array>
#include <private/qvideotexturehelper_p.h>

#include <QtCore/QMutex>

#include "GstHwPathTelemetry.h"

namespace {

/// Preflight inputs are stable within a caps epoch — memoize the last verdict per path so steady-state
/// frames skip the per-plane RHI queries. A new caps/QRhi changes the key and recomputes naturally; a destroyed QRhi's
/// verdicts are dropped (forgetRhi) so a new QRhi allocated at the same address cannot inherit them.
struct PreflightMemo {
    QRhi* rhi = nullptr;
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
    QSize size;
    QRhiTexture::Flags flags;
    bool result = false;
    bool valid = false;
};

QMutex s_memoMutex;
std::array<PreflightMemo, GstHw::kPathCount> s_memo{};

void forgetRhi(QRhi* rhi)
{
    QMutexLocker lock(&s_memoMutex);
    for (PreflightMemo& memo : s_memo) {
        if (memo.rhi == rhi) {
            memo = {};
        }
    }
}

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
    {
        QMutexLocker lock(&s_memoMutex);
        PreflightMemo& memo = s_memo[static_cast<std::size_t>(path)];
        if (memo.valid && (memo.rhi == rhi) && (memo.pixelFormat == pixelFormat) && (memo.size == size) &&
            (memo.flags == flags)) {
            ok = memo.result;
        } else {
            ok = canImportPlanes(rhi, pixelFormat, size, flags);
            memo = {rhi, pixelFormat, size, flags, ok, true};
            if (rhi) {
                // Keyed, so repeated registration replaces rather than stacks the callback.
                rhi->addCleanupCallback(&s_memo, [](QRhi* dying) { forgetRhi(dying); });
            }
        }
    }
    if (ok) {
        return true;
    }
    // Recorded per call (not per epoch) to keep fallback counts frame-accurate.
    GstHwPathTelemetry::recordFallbackReason(path, GstHwPathTelemetry::HwFallbackReason::ImportUnsupported);
    return false;
}

#ifdef QGC_UNITTEST_BUILD
std::size_t memoizedVerdictCountForTest(const QRhi* rhi) noexcept
{
    QMutexLocker lock(&s_memoMutex);
    return static_cast<std::size_t>(std::count_if(
        s_memo.cbegin(), s_memo.cend(), [rhi](const PreflightMemo& memo) { return memo.valid && (memo.rhi == rhi); }));
}
#endif

}  // namespace GstHwImportPreflight

#endif  // QGC_HAS_ANY_GPU_PATH
