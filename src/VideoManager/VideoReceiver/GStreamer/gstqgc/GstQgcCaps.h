#pragma once

#include <string>
#ifdef QGC_UNITTEST_BUILD
#include <optional>
#endif

#include "GstQgcGpuBin.h"
#include "GstQgcRenderTarget.h"

namespace GstQgc {

/// Everything qgcqvideosink's caps depend on.
struct SinkCapsPolicy
{
    bool gpuZeroCopy = false;
    /// qgcvideosinkbin's GPU variant in front of the sink; value-initialized, Passthrough (a standalone sink).
    GpuBinMode gpuBin{};
    RenderTarget target;
    /// The EGLDisplay the DMABuf path imports with now (HwBuffers::dmaBufImportDisplay()); null while it can't import.
    void* dmaBufDisplay = nullptr;
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

#ifdef QGC_UNITTEST_BUILD
/// What qgcqvideosink and qgcvideosinkbin read from the process.
struct SinkEnvironment
{
    RenderTarget target{};      ///< For currentRenderTarget().
    GpuBinPreference gpuBin{};  ///< For gpuBinPreference().
    /// For Qt's EGLDisplay in HwBuffers::dmaBufImportDisplay(); the import-failure latch still applies.
    void* dmaBufDisplay = nullptr;
};

/// The sink environment readers return @p environment while alive, then what they returned before.
class ScopedSinkEnvironmentForTest
{
public:
    explicit ScopedSinkEnvironmentForTest(const SinkEnvironment& environment);
    ~ScopedSinkEnvironmentForTest();
    ScopedSinkEnvironmentForTest(const ScopedSinkEnvironmentForTest&) = delete;
    ScopedSinkEnvironmentForTest& operator=(const ScopedSinkEnvironmentForTest&) = delete;

private:
    std::optional<SinkEnvironment> _previous;
};

/// The environment a live ScopedSinkEnvironmentForTest substitutes; nullopt without one.
std::optional<SinkEnvironment> sinkEnvironmentForTest();
#endif

}  // namespace GstQgc
