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

std::string buildGpuCapsString()
{
    const std::string kFormats = advertisedFormatList();
    std::string capsStr;
#if defined(QGC_GST_BIN_USE_GLUPLOAD)
    // texture-target=2D is load-bearing: it forces glcolorconvert to convert amcvideodec's external-OES
    // Surface textures to GL_TEXTURE_2D; the default ANY target passes external-OES through and the Qt
    // RHI 2D sink samples it black.
    // Keep this path single-plane for Qt's GL import. VA H.265 can negotiate NV12 GLMemory here, which
    // reaches QVideoFrame delivery cleanly but renders as intermittent green frames through Qt's sampler.
    capsStr = "video/x-raw(memory:GLMemory), texture-target=2D, format={ BGRA, RGBA }";
#else
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#if GST_CHECK_VERSION(1, 24, 0)
    // Best-effort: advertise the GPU's actually-supported (format, modifier) pairs (EGL query) so modifier-aware
    // drivers negotiate a tiling QGC's importer can consume. Additive — empty on any query failure (see below).
    capsStr += GstHw::buildSupportedDmaDrmCaps(kFormats.c_str());
    // QGC_GST_OFFER_DMA_DRM_LINEAR=1 force-offers LINEAR-modifier (0x0) DMA_DRM as a fallback/override so a driver
    // that mis-reports modifiers still gets a guaranteed-LINEAR option and iHD can't pick tiled+CCS.
    if (HwBuffers::hwBufferEnvConfig().offerDmaDrmLinear) {
        capsStr += GstHw::buildLinearDmaDrmCaps("{ NV12, NV21, I420, P010_10LE, BGRA, RGBA }");
    }
#endif
    // Legacy memory:DMABuf covers v4l2h264dec/Mali/V3D LINEAR. DMA_DRM omitted: gst-va iHD
    // negotiates tiled+CCS that crashes both paths (system catch-all routes va to GstVaMemory).
    capsStr += "video/x-raw(memory:DMABuf), format=";
    capsStr += kFormats;
    capsStr += "; ";
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    capsStr += "video/x-raw(memory:D3D11Memory), format=";
    capsStr += kFormats;
    capsStr += "; ";
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    capsStr += "video/x-raw(memory:D3D12Memory), format=";
    capsStr += kFormats;
    capsStr += "; ";
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && !defined(QGC_GST_BIN_USE_DMABUF)
    capsStr += "video/x-raw(memory:GLMemory), format=";
    capsStr += kFormats;
    capsStr += "; ";
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    capsStr += "video/x-raw(memory:AHardwareBuffer), format=";
    capsStr += kFormats;
    capsStr += "; ";
#endif
    capsStr += "video/x-raw, format=";
    capsStr += kFormats;
#endif
    return capsStr;
}

}  // namespace GstQgc
