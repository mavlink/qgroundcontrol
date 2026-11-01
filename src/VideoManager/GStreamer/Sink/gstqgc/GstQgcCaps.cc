#include "GstQgcCaps.h"

#include <gst/gst.h>
#include <vector>
#ifdef QGC_UNITTEST_BUILD
#include <utility>

#include <QtCore/QMutex>
#endif

#include "GstQgcVideoFormats.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"

namespace GstQgc {

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
    caps += GstHw::buildSupportedDmaDrmCaps(formats, display);
#endif
    return caps;
}
#endif

// texture-target=2D is load-bearing: the GL path samples GL_TEXTURE_2D only, and it makes glcolorconvert convert
// amcvideodec's external-OES Surface textures; with the default ANY target external-OES passes through and is sampled
// black.
constexpr const char* kGlMemory2D = "video/x-raw(memory:GLMemory), texture-target=2D";

// What the D3D12Convert bin's d3d12convert outputs: single-plane formats, the only ones Qt's D3D12 QRhi can sample.
constexpr const char* kD3D12MemoryRgb = "video/x-raw(memory:D3D12Memory)";
constexpr const char* kD3D12ConvertFormats = "(string){ RGBA, BGRA }";

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
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
            if (policy.target.egl && policy.dmaBufDisplay) {
                caps += sinkDmaDrmCaps(policy.dmaBufDisplay);
                // Legacy memory:DMABuf covers v4l2h264dec/Mali/V3D LINEAR. Unrestricted DMA_DRM is never offered (only
                // the importable modifiers above): gst-va iHD would negotiate tiled+CCS, which no per-plane import can
                // read.
                offer("video/x-raw(memory:DMABuf)");
            }
#endif
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
            offer("video/x-raw(memory:D3D11Memory)");
#endif
            break;
        default:
            // Direct3D12: D3D12Memory only through the D3D12Convert bin (buildSinkCaps). Metal: vtdec's
            // IOSurface-backed CoreVideo buffers arrive as system memory. Vulkan: no import path.
            break;
    }
    return caps;
}

}  // namespace

std::string buildSinkCaps(const SinkCapsPolicy& policy)
{
    if (policy.gpuZeroCopy && (policy.gpuBin == GpuBinMode::GlUpload)) {
        return std::string(kGlMemory2D) + ", format=" + advertisedFormatList(kOfferGlUpload);
    }
    if (policy.gpuZeroCopy && (policy.gpuBin == GpuBinMode::D3D12Convert)) {
        // d3d12convert scales as well, so it gives square pixels on the GPU.
        std::string caps = std::string(kD3D12MemoryRgb) + ", format=" + kD3D12ConvertFormats;
        if (policy.squarePixels) {
            caps += ", pixel-aspect-ratio=(fraction)1/1";
        }
        return caps;
    }

    const std::string formats = advertisedFormatList();
    std::string caps = policy.gpuZeroCopy ? gpuMemoryCaps(policy, formats) : std::string();
    caps += "video/x-raw, format=" + formats;
    if (policy.squarePixels) {
        caps += ", pixel-aspect-ratio=(fraction)1/1";
    }
    return caps;
}

#ifdef QGC_UNITTEST_BUILD
namespace {

Q_CONSTINIT QBasicMutex s_testEnvironmentMutex;
std::optional<SinkEnvironment> s_testEnvironment;

}  // namespace

ScopedSinkEnvironmentForTest::ScopedSinkEnvironmentForTest(const SinkEnvironment& environment)
{
    const QMutexLocker lock(&s_testEnvironmentMutex);
    _previous = std::exchange(s_testEnvironment, environment);
}

ScopedSinkEnvironmentForTest::~ScopedSinkEnvironmentForTest()
{
    const QMutexLocker lock(&s_testEnvironmentMutex);
    s_testEnvironment = _previous;
}

std::optional<SinkEnvironment> sinkEnvironmentForTest()
{
    const QMutexLocker lock(&s_testEnvironmentMutex);
    return s_testEnvironment;
}
#endif

}  // namespace GstQgc
