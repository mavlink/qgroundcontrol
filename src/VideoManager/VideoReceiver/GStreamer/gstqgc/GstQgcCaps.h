#pragma once

#include <string>

#include "GstQgcRenderTarget.h"

namespace HwBuffers {
enum class GpuBinMode;
}

namespace GstQgc {

/// Everything qgcqvideosink's caps depend on.
struct SinkCapsPolicy
{
    bool gpuZeroCopy = false;
    /// qgcvideosinkbin's GPU variant in front of the sink; value-initialized, Passthrough (a standalone sink).
    HwBuffers::GpuBinMode gpuBin{};
    RenderTarget target;
    /// Ask for square pixels in system memory, which the bin's converter scales to.
    bool squarePixels = true;

    friend bool operator==(const SinkCapsPolicy&, const SinkCapsPolicy&) = default;
};

/// The caps qgcqvideosink accepts under @p policy, most preferred first, all in the Qt-renderable formats of
/// kVideoFormatTable. The glupload variant takes GLMemory only, as glcolorconvert outputs it. Otherwise, with
/// gpu-zerocopy, the GPU memory @p policy's render target imports comes first: EGL-importable DMA_DRM and DMABuf, and
/// GLMemory unless the direct DMABuf variant, on OpenGL over EGL; D3D11Memory on Direct3D11; none elsewhere. System
/// memory always closes the list. DMABuf builds query EGL for importable DMA_DRM modifiers and read
/// QGC_GST_OFFER_DMA_DRM_LINEAR.
std::string buildSinkCaps(const SinkCapsPolicy& policy);

}  // namespace GstQgc
