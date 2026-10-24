#pragma once

#include <gst/gst.h>
#include <gst/video/video-format.h>
#include <span>
#include <string>

namespace GstHw {

/// Best-effort DMA_DRM caps string built from the GPU's EGL-reported (format, modifier) pairs intersected with
/// @p formats: a pair is offered when Qt's DMABuf importer can sample every plane of the format with that modifier
/// (no compression or auxiliary planes). Returns a single ready-to-prepend "video/x-raw(memory:DMABuf),
/// format=DMA_DRM, drm-format={ FMT:0xMOD, ... }; " fragment, or "" on ANY failure (no Qt EGL display, no modifier
/// query, import disabled). Additive only: callers must keep the existing system catch-all.
std::string buildSupportedDmaDrmCaps(std::span<const GstVideoFormat> formats);

#ifdef QGC_UNITTEST_BUILD
/// True when @p modifier carries no compression or auxiliary plane, so its planes can be imported one by one.
bool dmaDrmModifierImportableForTest(guint64 modifier) noexcept;
/// DRM fourcc plane @p plane of @p format is imported through; 0 when there is none.
guint32 dmaDrmPlaneImportFourccForTest(GstVideoFormat format, guint plane) noexcept;
#endif

}  // namespace GstHw
