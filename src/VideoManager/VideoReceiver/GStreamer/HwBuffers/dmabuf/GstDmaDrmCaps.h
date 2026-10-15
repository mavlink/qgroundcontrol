#pragma once

#include <gst/gst.h>

#include <string>

namespace GstHw {

/// Best-effort DMA_DRM caps string built from the GPU's EGL-reported (format, modifier) pairs intersected with the
/// renderable @p gstFormatList. Returns a single ready-to-prepend "video/x-raw(memory:DMABuf), format=DMA_DRM,
/// drm-format={ FMT:0xMOD, ... }; " fragment, or "" on ANY failure (no display/ext/results/error). Additive only:
/// callers must keep the existing system catch-all. @p gstFormatList is the kFormats brace list (e.g. "{ NV12, ... }").
std::string buildSupportedDmaDrmCaps(const char* gstFormatList);

/// LINEAR-modifier DMA_DRM caps fragment for @p gstFormatList (the kFormats brace list), as a forced fallback when the
/// driver mis-reports modifiers. Maps each GStreamer format to its DRM fourcc (bare = LINEAR). "" if none map.
std::string buildLinearDmaDrmCaps(const char* gstFormatList);

#ifdef QGC_GST_BUILD_TESTING
bool dmaDrmModifierAdvertisedForTest(guint64 modifier) noexcept;
#endif

}  // namespace GstHw
