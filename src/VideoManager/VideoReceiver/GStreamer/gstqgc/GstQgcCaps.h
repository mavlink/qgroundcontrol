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
    /// The DMABuf path imports now (HwBuffers::dmaBufImportDisplay()): Qt's EGLDisplay is known and no import failure
    /// disabled the path.
    bool dmaBufImport = false;
    /// Ask for square pixels in system memory, which the bin's converter scales to.
    bool squarePixels = true;

    friend bool operator==(const SinkCapsPolicy&, const SinkCapsPolicy&) = default;
};

/// The caps qgcqvideosink accepts under @p policy, most preferred first, all in the Qt-renderable formats of
/// kVideoFormatTable. The glupload variant takes GLMemory only, as glcolorconvert outputs it. Otherwise, with
/// gpu-zerocopy, the GPU memory @p policy's render target imports comes first: EGL-importable DMA_DRM and DMABuf on
/// OpenGL over EGL while the DMABuf import works; GLMemory, unless the direct DMABuf variant, wherever gst-gl shares
/// Qt's OpenGL context (EGL or GLX); D3D11Memory on Direct3D11; none elsewhere. GLMemory always carries
/// texture-target=2D, the only target the GL path samples. System memory always closes the list. DMABuf builds query
/// EGL for importable DMA_DRM modifiers.
std::string buildSinkCaps(const SinkCapsPolicy& policy);

}  // namespace GstQgc
