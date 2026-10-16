#pragma once

#include <string>

namespace HwBuffers {
enum class GpuBinMode;
}

namespace GstQgc {

/// Builds the format capsfilter caps for the gpu-zerocopy bin of @p mode, branching on the compiled GPU paths. The
/// glupload bin takes GLMemory only; the others take every compiled native memory plus system memory, and the direct
/// DMABuf bin leaves GLMemory out. DMABuf builds also query EGL for importable DMA_DRM modifiers and read
/// QGC_GST_OFFER_DMA_DRM_LINEAR.
std::string buildGpuCapsString(HwBuffers::GpuBinMode mode);

/// System-memory caps for the CPU branch's format capsfilter; shares the Qt-renderable format set
/// with buildGpuCapsString() so the two can't drift.
std::string buildCpuCapsString();

}  // namespace GstQgc
