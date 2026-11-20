#pragma once

#include <QtCore/QByteArrayView>

namespace GstQgc {

struct RenderTarget;

/// Head of qgcvideosinkbin's GPU branch. Linux builds with both gst-gl and DMABuf compile GlUpload, DirectDmaBuf and
/// DmaBufOrGlUpload; every other build compiles exactly one.
enum class GpuBinMode
{
    Passthrough,   ///< Every compiled native memory straight into qgcqvideosink (Windows, Apple, ...).
    GlUpload,      ///< glupload → glcolorconvert → GLMemory 2D; gst-gl imports LINEAR and vendor-tiled DMABuf.
    DirectDmaBuf,  ///< memory:DMABuf and DMA_DRM into QGC's own importer (LINEAR only), skipping gst-gl's copy.
    /// A switchbin routes each stream by the memory its decoder negotiates: DMABuf and DMA_DRM that QGC's importer
    /// takes go straight into it, as in DirectDmaBuf; GLMemory and system memory go through glupload →
    /// glcolorconvert, as in GlUpload. Needs switchbin of GStreamer 1.24 or later, else the bin is GlUpload.
    DmaBufOrGlUpload,
    /// d3d12upload → d3d12convert → single-plane D3D12Memory RGBA/BGRA: Qt's D3D12 QRhi has no plane views, so it
    /// can't sample NV12/P010 resources. The only variant for a Direct3D12 render target.
    D3D12Convert,
};

/// The GPU bin asked for, before the render target is known.
struct GpuBinPreference
{
    GpuBinMode mode = GpuBinMode::Passthrough;
    /// Chosen through QGC_GST_GPU_BIN: used even under a render target it doesn't suit.
    bool explicitChoice = false;
};

/// @p value of QGC_GST_GPU_BIN ("glupload" or "dmabuf", case-insensitive) against the compiled bins. Empty keeps the
/// default, DmaBufOrGlUpload; an unknown value warns and keeps it too. With one compiled bin the value is ignored.
GpuBinPreference parseGpuBinPreference(QByteArrayView value);

/// parseGpuBinPreference() of QGC_GST_GPU_BIN, read and logged on the first call.
GpuBinPreference gpuBinPreference();

/// The bin for @p preference under @p target. glupload hands GLMemory to Qt through the GL share group, so it needs Qt
/// on OpenGL that gst-gl shares (EGL or GLX), as does DMABuf-or-glupload, whose sink offers DMABuf only on EGL; direct
/// DMABuf hands EGLImages to Qt, so it needs OpenGL on EGL. A preference the target doesn't suit falls back to
/// passthrough unless it is explicit.
GpuBinMode selectGpuBin(GpuBinPreference preference, const RenderTarget& target) noexcept;

const char* gpuBinName(GpuBinMode mode) noexcept;

/// Whether the element @p factoryName of GStreamer's plugin @p pluginName may be the bin's converter under @p target.
/// Under Direct3D 11 a pipeline's d3d11 elements share the device Qt renders with (GstD3D11ContextBridge), and with
/// it the immediate context, whose state Qt caches. Decoders, the uploader and the downloader only copy there. The
/// plugin's other filters (d3d11convert, d3d11scale, d3d11deinterlace, d3d11overlay) draw, which sets that state
/// behind Qt's back.
bool converterSuitsRenderTarget(const char* pluginName, const char* factoryName, const RenderTarget& target) noexcept;

}  // namespace GstQgc
