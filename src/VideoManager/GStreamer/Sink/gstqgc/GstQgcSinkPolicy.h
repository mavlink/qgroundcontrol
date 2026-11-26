#pragma once

#include <string>

#include <QtCore/QByteArrayView>

/// What decides how qgcqvideosink and its bin take frames: the backend Qt Quick renders with, the GPU variant of the
/// bin, and the caps the sink offers for the two.
namespace GstQgc {

/// Graphics API Qt Quick renders the video with, which decides the GPU memory qgcqvideosink can import.
enum class RenderBackend
{
    Unknown,  ///< No scene graph yet, and the GUI thread hasn't recorded the requested API either.
    Null,     ///< QRhi's Null backend or the software renderer: no GPU import.
    OpenGL,
    Direct3D11,
    Direct3D12,
    Metal,
    Vulkan,
};

struct RenderTarget
{
    RenderBackend backend = RenderBackend::Unknown;
    /// Qt's OpenGL is known to run on EGL (QGCRhiCapture::qtEglDisplay()), so GStreamer's EGLImages (DMABuf,
    /// AHardwareBuffer) can use its display. False for GLX, WGL and CGL, and while that isn't known yet (Android before
    /// the scene graph runs).
    bool egl = false;

    bool isOpenGlOnEgl() const { return (backend == RenderBackend::OpenGL) && egl; }

    /// gst-gl can share Qt's OpenGL context (GstGlContextBridge wraps an EGL or a GLX one), so GLMemory textures reach
    /// Qt directly. Qt's OpenGL on Linux and Android is always one of the two, even before it is known which.
    bool sharesGlContext() const noexcept;

    friend bool operator==(const RenderTarget&, const RenderTarget&) = default;
};

/// What qgcqvideosink renders into now; callable from any thread. The backend of QGC's scene graph once it runs
/// (QGCRhiCapture); before that, the API Qt Quick was asked to use, as last read on the GUI thread.
RenderTarget currentRenderTarget();

const char* toString(RenderBackend backend) noexcept;

/// Head of the sink bin's GPU branch. Linux builds with both gst-gl and DMABuf compile GlUpload, DirectDmaBuf and
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

/// What qgcqvideosink and its sink bin read from the process: how Qt Quick renders, and the GPU bin asked for.
struct SinkEnvironment
{
    RenderTarget target{};
    GpuBinPreference gpuBin{};
    /// What the DMABuf path imports with (HwBuffers::dmaBufImportDisplay()): Qt's EGLDisplay, or its Vulkan device.
    void* dmaBufDisplay = nullptr;
    /// Qt's Direct3D 11 device opens a Direct3D 12 device's resources (HwBuffers::d3d11OpensD3D12()).
    bool d3d11OpensD3D12 = false;
};

/// The process's environment now: currentRenderTarget(), gpuBinPreference() and what HwBuffers knows of Qt's device.
/// Callable from any thread.
SinkEnvironment currentSinkEnvironment();

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

/// Everything qgcqvideosink's caps depend on.
struct SinkCapsPolicy
{
    bool gpuZeroCopy = false;
    /// The sink bin's GPU variant in front of the sink; value-initialized, Passthrough (a standalone sink).
    GpuBinMode gpuBin{};
    RenderTarget target;
    /// What the DMABuf path imports with now (SinkEnvironment::dmaBufDisplay): Qt's EGLDisplay, or its Vulkan
    /// device. Null while it can't import.
    void* dmaBufDisplay = nullptr;
    /// Qt's Direct3D 11 device opens a Direct3D 12 device's resources (SinkEnvironment::d3d11OpensD3D12).
    bool d3d11OpensD3D12 = false;
    /// Ask for square pixels, in every kind of memory. The bin's converter scales system memory to them.
    bool squarePixels = true;

    friend bool operator==(const SinkCapsPolicy&, const SinkCapsPolicy&) = default;
};

/// The caps qgcqvideosink accepts under @p policy, most preferred first, all in the Qt-renderable formats of
/// kVideoFormatTable. The glupload variant takes GLMemory only, as glcolorconvert outputs it; the DMABuf-or-glupload
/// variant takes that and, ahead of it, the EGL-importable DMA_DRM and DMABuf of the direct DMABuf variant. Otherwise,
/// with gpuZeroCopy, the GPU memory @p policy's render target imports comes first: EGL-importable DMA_DRM and DMABuf
/// on OpenGL over EGL while the DMABuf import works; GLMemory, unless the direct DMABuf variant, wherever gst-gl shares
/// Qt's OpenGL context (EGL or GLX); AHardwareBuffer on OpenGL over EGL (Android); on Direct3D11 NV12 and P010
/// D3D12Memory, where Qt's device opens it, then D3D11Memory; the DMA_DRM layouts Qt's device imports on Vulkan; none
/// elsewhere.
/// GLMemory always carries texture-target=2D, the only target the GL path samples. System memory closes the list,
/// except in the variants whose bin uploads it. Every entry asks for square pixels when @p policy does. DMABuf builds
/// query EGL for importable DMA_DRM modifiers.
std::string buildSinkCaps(const SinkCapsPolicy& policy);

}  // namespace GstQgc
