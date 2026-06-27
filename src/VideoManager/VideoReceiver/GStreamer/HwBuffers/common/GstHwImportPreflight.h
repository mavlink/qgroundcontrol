#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>
#include <rhi/qrhi.h>

#include "GstHwVideoBufferFactory.h"  // HwVideoBufferPath

/// Read-only RHI capability pre-flight: rejects a GPU import before createFrom() so a frame that the active QRhi
/// cannot back demotes to CPU on a cheap query instead of after a driver error. All queries are light (backends
/// cache resource limits); safe on the streaming/bus-sync thread as long as @p rhi is the live render QRhi.
namespace GstHwImportPreflight {

/// True iff @p rhi can create a texture of @p fmt + @p flags at @p size (format supported AND within TextureSizeMax).
/// Null @p rhi returns true (no snapshot to gate on — let the createFrom attempt decide).
bool canImportTexture(QRhi* rhi, QRhiTexture::Format fmt, const QSize& size, QRhiTexture::Flags flags = {}) noexcept;

/// Multiplane variant: derives each plane's QRhiTexture::Format + plane size from @p pixelFormat via
/// QVideoTextureHelper and pre-flights every plane. Used by NV12 (R8/RG8) / P010 (R16/RG16) imports.
bool canImportPlanes(QRhi* rhi, QVideoFrameFormat::PixelFormat pixelFormat, const QSize& size,
                     QRhiTexture::Flags flags = {}) noexcept;

/// Pre-flight + telemetry: runs canImportPlanes and, on rejection, records an ImportUnsupported fallback for
/// @p path so the demotion shows up in the per-(path,reason) breakdown like other CPU fallbacks. Returns the
/// canImportPlanes result.
bool preflightOrRecord(QRhi* rhi, HwVideoBufferPath path, QVideoFrameFormat::PixelFormat pixelFormat,
                       const QSize& size, QRhiTexture::Flags flags = {}) noexcept;

}  // namespace GstHwImportPreflight

#endif  // QGC_HAS_ANY_GPU_PATH
