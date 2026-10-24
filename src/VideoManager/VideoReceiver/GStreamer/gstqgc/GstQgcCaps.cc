#include "GstQgcCaps.h"

#include <gst/gst.h>
#include <vector>

#include "GstQgcVideoFormats.h"
#include "HwBuffers/common/HwBuffers.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"

namespace GstQgc {

static_assert(HwBuffers::GpuBinMode{} == HwBuffers::GpuBinMode::Passthrough,
              "SinkCapsPolicy::gpuBin's value-initialized default must be Passthrough");

namespace {

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
/// The DMA_DRM fragments the DMABuf importer takes; "" before GStreamer 1.24 or when EGL reports none.
std::string sinkDmaDrmCaps()
{
    std::string caps;
#if GST_CHECK_VERSION(1, 24, 0)
    const std::vector<GstVideoFormat> formats = advertisedFormats(kOfferDmaDrm);
    // Best-effort: advertise the GPU's actually-supported (format, modifier) pairs (EGL query) so modifier-aware
    // drivers negotiate a tiling QGC's importer can consume. Additive; empty on any query failure.
    caps += GstHw::buildSupportedDmaDrmCaps(formats);
#endif
    return caps;
}
#endif

// texture-target=2D is load-bearing: the GL path samples GL_TEXTURE_2D only, and it makes glcolorconvert convert
// amcvideodec's external-OES Surface textures; with the default ANY target external-OES passes through and is sampled
// black.
constexpr const char* kGlMemory2D = "video/x-raw(memory:GLMemory), texture-target=2D";

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
            if (policy.target.egl && policy.dmaBufImport) {
                caps += sinkDmaDrmCaps();
                // Legacy memory:DMABuf covers v4l2h264dec/Mali/V3D LINEAR. Unrestricted DMA_DRM is never offered (only
                // the importable modifiers above): gst-va iHD would negotiate tiled+CCS, which no per-plane import can
                // read.
                offer("video/x-raw(memory:DMABuf)");
            }
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
            if (policy.target.sharesGlContext() && (policy.gpuBin != HwBuffers::GpuBinMode::DirectDmaBuf)) {
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
            // Direct3D12: deliberately no D3D12Memory. GPU zero-copy has never been enabled under this RHI (GStreamer
            // can't wrap Qt's ID3D12Device, and QGC's D3D12 import isn't validated), so frames take system memory.
            // Metal: vtdec's IOSurface-backed CoreVideo buffers arrive as system memory. Vulkan: no import path.
            break;
    }
    return caps;
}

}  // namespace

std::string buildSinkCaps(const SinkCapsPolicy& policy)
{
    if (policy.gpuZeroCopy && (policy.gpuBin == HwBuffers::GpuBinMode::GlUpload)) {
        return std::string(kGlMemory2D) + ", format=" + advertisedFormatList(kOfferGlUpload);
    }

    const std::string formats = advertisedFormatList();
    std::string caps = policy.gpuZeroCopy ? gpuMemoryCaps(policy, formats) : std::string();
    caps += "video/x-raw, format=" + formats;
    if (policy.squarePixels) {
        caps += ", pixel-aspect-ratio=(fraction)1/1";
    }
    return caps;
}

}  // namespace GstQgc
