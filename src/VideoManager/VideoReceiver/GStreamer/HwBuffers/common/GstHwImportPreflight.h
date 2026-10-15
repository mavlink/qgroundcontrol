#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <rhi/qrhi.h>

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwTypes.h"

/// Read-only RHI capability pre-flight: rejects a GPU import before createFrom() so mapTextures() fails on a cheap
/// query instead of a driver error, and Qt uploads the frame through GstHwVideoBuffer::map() instead. All queries are
/// light (backends cache resource limits).
namespace GstHwImportPreflight {

/// True iff @p rhi can create a texture of @p fmt + @p flags at @p size (format supported AND within TextureSizeMax).
/// Null @p rhi returns true (no snapshot to gate on — let the createFrom attempt decide).
bool canImportTexture(QRhi* rhi, QRhiTexture::Format fmt, const QSize& size, QRhiTexture::Flags flags = {}) noexcept;

/// Multiplane variant: derives each plane's QRhiTexture::Format + plane size from @p pixelFormat via
/// QVideoTextureHelper and pre-flights every plane. Used by NV12 (R8/RG8) / P010 (R16/RG16) imports.
bool canImportPlanes(QRhi* rhi, QVideoFrameFormat::PixelFormat pixelFormat, const QSize& size,
                     QRhiTexture::Flags flags = {}) noexcept;

/// Pre-flight + telemetry: runs canImportPlanes and, on rejection, records an ImportUnsupported fallback for
/// @p path so the rejection shows up in the per-(path,reason) breakdown. Returns the canImportPlanes result.
/// The verdict is memoized per path until @p rhi is destroyed, so call it on @p rhi's thread (mapTextures()).
bool preflightOrRecord(QRhi* rhi, HwVideoBufferPath path, QVideoFrameFormat::PixelFormat pixelFormat,
                       const QSize& size, QRhiTexture::Flags flags = {}) noexcept;

#ifdef QGC_UNITTEST_BUILD
/// Memoized verdicts currently keyed on @p rhi (compared by address only, never dereferenced).
std::size_t memoizedVerdictCountForTest(const QRhi* rhi) noexcept;
#endif

}  // namespace GstHwImportPreflight

#endif  // QGC_HAS_ANY_GPU_PATH
