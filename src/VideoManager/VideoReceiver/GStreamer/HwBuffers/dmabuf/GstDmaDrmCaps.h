#pragma once

#include <gst/gst.h>
#include <gst/video/video-format.h>
#include <span>
#include <string>

namespace GstHw {

/// Best-effort DMA_DRM caps string built from the GPU's EGL-reported (format, modifier) pairs intersected with
/// @p formats. Returns a single ready-to-prepend "video/x-raw(memory:DMABuf), format=DMA_DRM,
/// drm-format={ FMT:0xMOD, ... }; " fragment, or "" on ANY failure (no display/ext/results/error). Additive only:
/// callers must keep the existing system catch-all.
std::string buildSupportedDmaDrmCaps(std::span<const GstVideoFormat> formats);

/// LINEAR-modifier DMA_DRM caps fragment for @p formats, in their order, as a forced fallback when the driver
/// mis-reports modifiers. "" if none has a DRM fourcc.
std::string buildLinearDmaDrmCaps(std::span<const GstVideoFormat> formats);

#ifdef QGC_UNITTEST_BUILD
bool dmaDrmModifierAdvertisedForTest(guint64 modifier) noexcept;
#endif

}  // namespace GstHw
