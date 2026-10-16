#include "GstQgcCaps.h"

#include <gst/gst.h>

#include "GstQgcVideoFormats.h"
#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "HwBuffers/common/HwBuffers.h"

namespace GstQgc {

std::string buildCpuCapsString()
{
    return std::string("video/x-raw, format=") + advertisedFormatList();
}

std::string buildGpuCapsString(HwBuffers::GpuBinMode mode)
{
    if (mode == HwBuffers::GpuBinMode::GlUpload) {
        // texture-target=2D is load-bearing: it forces glcolorconvert to convert amcvideodec's external-OES
        // Surface textures to GL_TEXTURE_2D; the default ANY target passes external-OES through and the Qt
        // RHI 2D sink samples it black.
        return "video/x-raw(memory:GLMemory), texture-target=2D, format=" + advertisedFormatList(kOfferGlUpload);
    }

    const std::string kFormats = advertisedFormatList();
    std::string capsStr;
    // "<media type>, format=<advertised formats>; "
    [[maybe_unused]] const auto offer = [&capsStr, &kFormats](const char* mediaType) {
        capsStr += mediaType;
        capsStr += ", format=";
        capsStr += kFormats;
        capsStr += "; ";
    };
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#if GST_CHECK_VERSION(1, 24, 0)
    const std::vector<GstVideoFormat> dmaDrmFormats = advertisedFormats(kOfferLinearDmaDrm);
    // Best-effort: advertise the GPU's actually-supported (format, modifier) pairs (EGL query) so modifier-aware
    // drivers negotiate a tiling QGC's importer can consume. Additive — empty on any query failure (see below).
    capsStr += GstHw::buildSupportedDmaDrmCaps(dmaDrmFormats);
    // QGC_GST_OFFER_DMA_DRM_LINEAR=1 force-offers LINEAR-modifier (0x0) DMA_DRM as a fallback/override so a driver
    // that mis-reports modifiers still gets a guaranteed-LINEAR option and iHD can't pick tiled+CCS.
    if (HwBuffers::hwBufferEnvConfig().offerDmaDrmLinear) {
        capsStr += GstHw::buildLinearDmaDrmCaps(dmaDrmFormats);
    }
#endif
    // Legacy memory:DMABuf covers v4l2h264dec/Mali/V3D LINEAR. Unrestricted DMA_DRM is never offered (only the
    // importable modifiers above): gst-va iHD would negotiate tiled+CCS, which crashes both import paths.
    offer("video/x-raw(memory:DMABuf)");
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    offer("video/x-raw(memory:D3D11Memory)");
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    offer("video/x-raw(memory:D3D12Memory)");
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    if (mode != HwBuffers::GpuBinMode::DirectDmaBuf) {
        offer("video/x-raw(memory:GLMemory)");
    }
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    offer("video/x-raw(memory:AHardwareBuffer)");
#endif
    capsStr += "video/x-raw, format=" + kFormats;
    return capsStr;
}

}  // namespace GstQgc
