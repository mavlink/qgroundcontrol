#pragma once

#include <rhi/qrhi.h>

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwTypes.h"

/// Read-only RHI capability pre-flight, for a path that asks before its own costly import: rejects a frame whose
/// textures @p rhi can't create, so Qt uploads it through GstHwVideoBuffer::map() instead. All queries are light
/// (backends cache resource limits).
namespace GstHwImportPreflight {

/// Pre-flight + telemetry: true iff @p rhi can create every plane texture of @p pixelFormat with @p flags at @p size
/// (format supported AND within TextureSizeMax). Each plane's QRhiTexture::Format + plane size come from
/// QVideoTextureHelper: NV12 (R8/RG8), P010 (R16/RG16). A rejection records an ImportUnsupported fallback for @p path
/// so it shows up in the per-(path,reason) breakdown. Call it on @p rhi's thread (importTextures()).
bool preflightOrRecord(QRhi& rhi, HwVideoBufferPath path, QVideoFrameFormat::PixelFormat pixelFormat, const QSize& size,
                       QRhiTexture::Flags flags = {});

}  // namespace GstHwImportPreflight
