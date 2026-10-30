#pragma once

#include <QtCore/QByteArrayView>

namespace GstQgc {

struct RenderTarget;

/// Head of qgcvideosinkbin's GPU branch. Linux builds with both gst-gl and DMABuf compile GlUpload and DirectDmaBuf;
/// every other build compiles exactly one.
enum class GpuBinMode
{
    Passthrough,   ///< Every compiled native memory straight into qgcqvideosink (Windows, Apple, ...).
    GlUpload,      ///< glupload → glcolorconvert → GLMemory 2D; gst-gl imports LINEAR and vendor-tiled DMABuf.
    DirectDmaBuf,  ///< memory:DMABuf and DMA_DRM into QGC's own importer (LINEAR only), skipping gst-gl's copy.
};

/// The GPU bin asked for, before the render target is known.
struct GpuBinPreference
{
    GpuBinMode mode = GpuBinMode::Passthrough;
    /// Chosen through QGC_GST_GPU_BIN: used even under a render target it doesn't suit.
    bool explicitChoice = false;
};

/// @p value of QGC_GST_GPU_BIN ("glupload" or "dmabuf", case-insensitive) against the compiled bins. Empty keeps the
/// default, GlUpload; an unknown value warns and keeps it too. With one compiled bin the value is ignored.
GpuBinPreference parseGpuBinPreference(QByteArrayView value);

/// parseGpuBinPreference() of QGC_GST_GPU_BIN, read and logged on the first call.
GpuBinPreference gpuBinPreference() noexcept;

/// The bin for @p preference under @p target. glupload hands GLMemory to Qt through the GL share group, so it needs Qt
/// on OpenGL that gst-gl shares (EGL or GLX); direct DMABuf hands EGLImages to Qt, so it needs OpenGL on EGL. A
/// preference the target doesn't suit falls back to passthrough unless it is explicit.
GpuBinMode selectGpuBin(GpuBinPreference preference, const RenderTarget& target) noexcept;

const char* gpuBinName(GpuBinMode mode) noexcept;

#ifdef QGC_UNITTEST_BUILD
/// gpuBinPreference() returns @p mode, not explicitly chosen, while alive, then what it returned before.
class ScopedGpuBinModeForTest
{
public:
    explicit ScopedGpuBinModeForTest(GpuBinMode mode);
    ~ScopedGpuBinModeForTest();
    ScopedGpuBinModeForTest(const ScopedGpuBinModeForTest&) = delete;
    ScopedGpuBinModeForTest& operator=(const ScopedGpuBinModeForTest&) = delete;

private:
    int _previous;
};
#endif

}  // namespace GstQgc
