#pragma once

#include <string>

namespace GstQgc {

/// Builds the format capsfilter caps for the gpu-zerocopy bin, branching on the compiled GPU paths and the bin
/// variant. DMABuf builds also query EGL for importable DMA_DRM modifiers and read QGC_GST_OFFER_DMA_DRM_LINEAR.
std::string buildGpuCapsString();

/// System-memory caps for the CPU branch's format capsfilter; shares the Qt-renderable format set
/// with buildGpuCapsString() so the two can't drift.
std::string buildCpuCapsString();

}  // namespace GstQgc
