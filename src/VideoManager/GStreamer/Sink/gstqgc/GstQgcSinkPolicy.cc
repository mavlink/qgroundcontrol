#include "GstQgcSinkPolicy.h"

#include <atomic>
#include <gst/gst.h>
#include <string_view>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QThread>
#include <QtCore/QtEnvironmentVariables>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

#if defined(QGC_HAS_ANY_GPU_PATH)
#include <rhi/qrhi.h>
#endif
#include "GstQgcVideoFormats.h"
#include "GstScoped.h"
#include "HwBuffers/HwBuffers.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstQgcGpuBinLog, "Video.GStreamer.GstQgcGpuBin")

namespace GstQgc {

namespace {

constinit std::atomic<int> s_requestedBackend{static_cast<int>(RenderBackend::Unknown)};

RenderBackend fromGraphicsApi(QSGRendererInterface::GraphicsApi api) noexcept
{
    switch (api) {
        case QSGRendererInterface::Unknown:
            return RenderBackend::Unknown;
        case QSGRendererInterface::OpenGL:
            return RenderBackend::OpenGL;
        case QSGRendererInterface::Direct3D11:
            return RenderBackend::Direct3D11;
        case QSGRendererInterface::Direct3D12:
            return RenderBackend::Direct3D12;
        case QSGRendererInterface::Metal:
            return RenderBackend::Metal;
        case QSGRendererInterface::Vulkan:
            return RenderBackend::Vulkan;
        default:
            return RenderBackend::Null;
    }
}

#if defined(QGC_HAS_ANY_GPU_PATH)
RenderBackend fromRhiImplementation(int implementation) noexcept
{
    switch (static_cast<QRhi::Implementation>(implementation)) {
        case QRhi::Null:
            return RenderBackend::Null;
        case QRhi::OpenGLES2:
            return RenderBackend::OpenGL;
        case QRhi::D3D11:
            return RenderBackend::Direct3D11;
        case QRhi::D3D12:
            return RenderBackend::Direct3D12;
        case QRhi::Metal:
            return RenderBackend::Metal;
        case QRhi::Vulkan:
            return RenderBackend::Vulkan;
    }
    return RenderBackend::Unknown;
}
#endif

}  // namespace

RenderTarget currentRenderTarget()
{
    // QQuickWindow::graphicsApi() may apply Qt Quick's settings on first use, so only the GUI thread asks it.
    if (QThread::isMainThread()) {
        s_requestedBackend.store(static_cast<int>(fromGraphicsApi(QQuickWindow::graphicsApi())),
                                 std::memory_order_relaxed);
    }
    RenderTarget target{.backend = static_cast<RenderBackend>(s_requestedBackend.load(std::memory_order_relaxed))};
#if defined(QGC_HAS_ANY_GPU_PATH)
    if (const int live = HwBuffers::renderBackend(); live >= 0) {
        target.backend = fromRhiImplementation(live);
    }
    target.egl = (target.backend == RenderBackend::OpenGL) && HwBuffers::qtEglDisplay();
#endif
    return target;
}

bool RenderTarget::sharesGlContext() const noexcept
{
#if defined(Q_OS_LINUX)
    return backend == RenderBackend::OpenGL;
#else
    // WGL and CGL contexts aren't wrapped.
    return isOpenGlOnEgl();
#endif
}

const char* toString(RenderBackend backend) noexcept
{
    switch (backend) {
        case RenderBackend::Unknown:
            return "unknown";
        case RenderBackend::Null:
            return "null";
        case RenderBackend::OpenGL:
            return "OpenGL";
        case RenderBackend::Direct3D11:
            return "Direct3D11";
        case RenderBackend::Direct3D12:
            return "Direct3D12";
        case RenderBackend::Metal:
            return "Metal";
        case RenderBackend::Vulkan:
            return "Vulkan";
    }
    return "unknown";
}

GpuBinPreference parseGpuBinPreference(QByteArrayView value)
{
#if defined(QGC_GST_BIN_HAS_GLUPLOAD) && defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    const QByteArray choice = value.trimmed().toByteArray().toLower();
    if (choice == "dmabuf") {
        return {.mode = GpuBinMode::DirectDmaBuf, .explicitChoice = true};
    }
    if (choice == "glupload") {
        return {.mode = GpuBinMode::GlUpload, .explicitChoice = true};
    }
    if (!choice.isEmpty()) {
        qCWarning(GstQgcGpuBinLog) << "Unknown QGC_GST_GPU_BIN value" << choice
                                   << "(expected glupload or dmabuf); using the default bin";
    }
    return {.mode = GpuBinMode::DmaBufOrGlUpload};
#else
    Q_UNUSED(value);
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    return {.mode = GpuBinMode::GlUpload};
#elif defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    return {.mode = GpuBinMode::DirectDmaBuf};
#else
    return {};
#endif
#endif
}

GpuBinPreference gpuBinPreference()
{
    static const GpuBinPreference preference = [] {
        const GpuBinPreference parsed = parseGpuBinPreference(qgetenv("QGC_GST_GPU_BIN"));
        qCDebug(GstQgcGpuBinLog).nospace() << "GPU bin preference: " << gpuBinName(parsed.mode) << " ("
                                           << (parsed.explicitChoice ? "QGC_GST_GPU_BIN" : "default") << ")";
        return parsed;
    }();
    return preference;
}

SinkEnvironment currentSinkEnvironment()
{
    return {.target = currentRenderTarget(),
            .gpuBin = gpuBinPreference(),
            .dmaBufDisplay = HwBuffers::dmaBufImportDisplay(),
            .d3d11OpensD3D12 = HwBuffers::d3d11OpensD3D12()};
}

GpuBinMode selectGpuBin(GpuBinPreference preference, const RenderTarget& target) noexcept
{
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (target.backend == RenderBackend::Direct3D12) {
        return GpuBinMode::D3D12Convert;
    }
#endif
    bool suited = true;
    switch (preference.mode) {
        case GpuBinMode::GlUpload:
        case GpuBinMode::DmaBufOrGlUpload:
            suited = target.sharesGlContext();
            break;
        case GpuBinMode::DirectDmaBuf:
            suited = target.isOpenGlOnEgl();
            break;
        case GpuBinMode::D3D12Convert:
            suited = (target.backend == RenderBackend::Direct3D12);
            break;
        case GpuBinMode::Passthrough:
            break;
    }
    return (suited || preference.explicitChoice) ? preference.mode : GpuBinMode::Passthrough;
}

const char* gpuBinName(GpuBinMode mode) noexcept
{
    switch (mode) {
        case GpuBinMode::GlUpload:
            return "glupload";
        case GpuBinMode::DirectDmaBuf:
            return "direct DMABuf";
        case GpuBinMode::DmaBufOrGlUpload:
            return "DMABuf-or-glupload";
        case GpuBinMode::D3D12Convert:
            return "d3d12convert";
        case GpuBinMode::Passthrough:
            break;
    }
    return "passthrough";
}

bool converterSuitsRenderTarget(const char* pluginName, const char* factoryName, const RenderTarget& target) noexcept
{
    const auto is = [](const char* text, std::string_view expected) { return text && (expected == text); };
    if ((target.backend != RenderBackend::Direct3D11) || !is(pluginName, "d3d11")) {
        return true;
    }
    return is(factoryName, "d3d11upload") || is(factoryName, "d3d11download");
}

static_assert(GpuBinMode{} == GpuBinMode::Passthrough,
              "SinkCapsPolicy::gpuBin's value-initialized default must be Passthrough");

namespace {

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
/// The DMA_DRM fragments the DMABuf importer takes on @p display; "" before GStreamer 1.24 or when EGL reports none.
std::string sinkDmaDrmCaps([[maybe_unused]] void* display)
{
    std::string caps;
#if GST_CHECK_VERSION(1, 24, 0)
    const std::vector<GstVideoFormat> formats = advertisedFormats(kOfferDmaDrm);
    // Best-effort: advertise the GPU's actually-supported (format, modifier) pairs (EGL query) so modifier-aware
    // drivers negotiate a tiling QGC's importer can consume. Additive; empty on any query failure.
    caps += GstHw::buildSupportedDmaDrmCaps(formats, &toQtPixelFormat, display);
#endif
    return caps;
}
#endif

// texture-target=2D is load-bearing: the GL path samples GL_TEXTURE_2D only, and it makes glcolorconvert convert
// amcvideodec's external-OES Surface textures; with the default ANY target external-OES passes through and is sampled
// black.
constexpr const char* kGlMemory2D = "video/x-raw(memory:GLMemory), texture-target=2D";

constexpr const char* kD3D12Memory = "video/x-raw(memory:D3D12Memory)";
// What the D3D12Convert bin's d3d12convert outputs: single-plane formats, the only ones Qt's D3D12 QRhi can sample.
constexpr const char* kD3D12ConvertFormats = "(string){ RGBA, BGRA }";
#if defined(QGC_HAS_GST_D3D11_GPU_PATH) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
// What decoders output as D3D12Memory, each one resource a Direct3D 11 device opens as a texture with both planes.
constexpr const char* kD3D12DecoderFormats = "(string){ NV12, P010_10LE }";
#endif

/// The DMA_DRM and DMABuf caps of @p formats that QGC's importer takes on Qt's EGL display; "" while it can't import.
std::string dmaBufImportCaps([[maybe_unused]] const SinkCapsPolicy& policy, [[maybe_unused]] const std::string& formats)
{
    std::string caps;
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    // DMABuf reaches Qt as EGLImages on its EGLDisplay.
    if (policy.target.isOpenGlOnEgl() && policy.dmaBufDisplay) {
        caps += sinkDmaDrmCaps(policy.dmaBufDisplay);
        // Legacy memory:DMABuf covers v4l2h264dec/Mali/V3D LINEAR. Unrestricted DMA_DRM is never offered (only the
        // importable modifiers above): gst-va iHD would negotiate tiled+CCS, which no per-plane import can read.
        caps += "video/x-raw(memory:DMABuf), format=" + formats + "; ";
    }
#endif
    return caps;
}

/// What glcolorconvert hands the sink at the end of a glupload chain.
std::string glUploadCaps()
{
    return std::string(kGlMemory2D) + ", format=" + advertisedFormatList(kOfferGlUpload);
}

/// "<media type>, format=<formats>; " for each memory the render target of @p policy imports.
std::string gpuMemoryCaps(const SinkCapsPolicy& policy, const std::string& formats)
{
    std::string caps;
    [[maybe_unused]] const auto offer = [&caps, &formats](const char* mediaType) {
        caps += mediaType;
        caps += ", format=";
        caps += formats;
        caps += "; ";
    };
    switch (policy.target.backend) {
        case RenderBackend::OpenGL:
            // DMABuf and AHardwareBuffer reach Qt as EGLImages on its EGLDisplay; GLMemory through the GL share group,
            // which gst-gl joins on EGL and GLX.
            caps += dmaBufImportCaps(policy, formats);
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
            if (policy.target.sharesGlContext() && (policy.gpuBin != GpuBinMode::DirectDmaBuf)) {
                offer(kGlMemory2D);
            }
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
            if (policy.target.egl) {
                offer("video/x-raw(memory:AHardwareBuffer)");
            }
#endif
            break;
        case RenderBackend::Direct3D11:
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
            // First: its decoder works on a device of its own, a D3D11Memory one on the device Qt renders with.
            if (policy.d3d11OpensD3D12) {
                caps += std::string(kD3D12Memory) + ", format=" + kD3D12DecoderFormats + "; ";
            }
#endif
            offer("video/x-raw(memory:D3D11Memory)");
#endif
            break;
        case RenderBackend::Vulkan:
            // DMA_DRM only, in the layouts Qt's Vulkan device imports: Vulkan needs the modifier, which legacy
            // memory:DMABuf caps don't carry.
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT) && GST_CHECK_VERSION(1, 24, 0)
            if (policy.dmaBufDisplay) {
                caps += GstHw::buildVulkanDmaDrmCaps(advertisedFormats(kOfferDmaDrm), &toQtPixelFormat);
            }
#endif
            break;
        case RenderBackend::Direct3D12:
        case RenderBackend::Metal:
        case RenderBackend::Null:
        case RenderBackend::Unknown:
            // Direct3D12: D3D12Memory only through the D3D12Convert bin (buildSinkCaps). Metal: vtdec's
            // IOSurface-backed CoreVideo buffers arrive as system memory.
            break;
    }
    return caps;
}

/// The memory and formats the sink takes under @p policy, whatever their pixels' shape.
std::string sinkMemoryCaps(const SinkCapsPolicy& policy)
{
    if (policy.gpuZeroCopy && (policy.gpuBin == GpuBinMode::GlUpload)) {
        return glUploadCaps();
    }
    if (policy.gpuZeroCopy && (policy.gpuBin == GpuBinMode::DmaBufOrGlUpload)) {
        // No system memory: the bin's switchbin sends it through glupload.
        return dmaBufImportCaps(policy, advertisedFormatList()) + glUploadCaps();
    }
    if (policy.gpuZeroCopy && (policy.gpuBin == GpuBinMode::D3D12Convert)) {
        return std::string(kD3D12Memory) + ", format=" + kD3D12ConvertFormats;
    }
    const std::string formats = advertisedFormatList();
    return (policy.gpuZeroCopy ? gpuMemoryCaps(policy, formats) : std::string()) + "video/x-raw, format=" + formats;
}

/// @p caps asking for pixel-aspect-ratio=1/1 in every structure.
std::string sinkCapsWithSquarePixels(const std::string& caps)
{
    const GStreamer::GstCapsPtr parsed = GStreamer::adoptCaps(gst_caps_from_string(caps.c_str()));
    if (!parsed) {
        return caps;
    }
    gst_caps_set_simple(parsed.get(), "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, nullptr);
    const GStreamer::GCharPtr text = GStreamer::adoptGChar(gst_caps_to_string(parsed.get()));
    return text ? std::string(text.get()) : caps;
}

}  // namespace

std::string buildSinkCaps(const SinkCapsPolicy& policy)
{
    // Square pixels in GPU memory as well as in system memory: the frame's size is all the VideoOutput has to go by.
    // Only system memory is scaled to them, by the bin's converter (and D3D12Memory by d3d12convert), so a decoder
    // with other pixels negotiates system memory.
    const std::string caps = sinkMemoryCaps(policy);
    return policy.squarePixels ? sinkCapsWithSquarePixels(caps) : caps;
}

}  // namespace GstQgc
