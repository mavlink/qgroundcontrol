#include "GstDmaFourcc.h"

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)

#include <drm_fourcc.h>

namespace GstHw {

int drmFourccForPlane(const GstVideoInfo& info, int plane)
{
    const GstVideoFormat fmt = GST_VIDEO_INFO_FORMAT(&info);
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    constexpr int argbFourcc = DRM_FORMAT_ARGB8888;
    constexpr int rgbaFourcc = DRM_FORMAT_ABGR8888;
    constexpr int rgbFourcc = DRM_FORMAT_BGR888;
    constexpr int rgFourcc = DRM_FORMAT_GR88;
#else
    constexpr int argbFourcc = DRM_FORMAT_BGRA8888;
    constexpr int rgbaFourcc = DRM_FORMAT_RGBA8888;
    constexpr int rgbFourcc = DRM_FORMAT_RGB888;
    constexpr int rgFourcc = DRM_FORMAT_RG88;
#endif

    switch (fmt) {
        case GST_VIDEO_FORMAT_RGB16:
        case GST_VIDEO_FORMAT_BGR16:
            return DRM_FORMAT_RGB565;
        case GST_VIDEO_FORMAT_RGB:
        case GST_VIDEO_FORMAT_BGR:
            return rgbFourcc;
        // BGRx/BGRA map to argb_fourcc (LE: ARGB8888 vs ABGR8888), mirrors Qt's fourccFromVideoInfo().
        case GST_VIDEO_FORMAT_BGRA:
        case GST_VIDEO_FORMAT_BGRx:
            return argbFourcc;
        case GST_VIDEO_FORMAT_RGBA:
        case GST_VIDEO_FORMAT_RGBx:
        case GST_VIDEO_FORMAT_ARGB:
        case GST_VIDEO_FORMAT_xRGB:
        case GST_VIDEO_FORMAT_ABGR:
        case GST_VIDEO_FORMAT_xBGR:
        case GST_VIDEO_FORMAT_AYUV:
            return rgbaFourcc;
#if defined(DRM_FORMAT_BGRA1010102)
        case GST_VIDEO_FORMAT_BGR10A2_LE:
            return DRM_FORMAT_BGRA1010102;
#endif
        case GST_VIDEO_FORMAT_GRAY8:
            return DRM_FORMAT_R8;
        case GST_VIDEO_FORMAT_YUY2:
        case GST_VIDEO_FORMAT_UYVY:
        case GST_VIDEO_FORMAT_GRAY16_LE:
        case GST_VIDEO_FORMAT_GRAY16_BE:
            return rgFourcc;
        case GST_VIDEO_FORMAT_NV12:
        case GST_VIDEO_FORMAT_NV21:
            return plane == 0 ? DRM_FORMAT_R8 : rgFourcc;
        case GST_VIDEO_FORMAT_I420:
        case GST_VIDEO_FORMAT_YV12:
        case GST_VIDEO_FORMAT_Y42B:
        case GST_VIDEO_FORMAT_Y444:
            return DRM_FORMAT_R8;
        case GST_VIDEO_FORMAT_P010_10LE:
        case GST_VIDEO_FORMAT_P010_10BE:
            return plane == 0 ? DRM_FORMAT_R16 : DRM_FORMAT_RG1616;
        default:
            return -1;
    }
}

int drmFourccForSingleFd(const GstVideoInfo& info)
{
    switch (GST_VIDEO_INFO_FORMAT(&info)) {
        case GST_VIDEO_FORMAT_NV12:
            return DRM_FORMAT_NV12;
        case GST_VIDEO_FORMAT_NV21:
            return DRM_FORMAT_NV21;
        case GST_VIDEO_FORMAT_P010_10LE:
            return DRM_FORMAT_P010;
        case GST_VIDEO_FORMAT_I420:
            return DRM_FORMAT_YUV420;
        case GST_VIDEO_FORMAT_YV12:
            return DRM_FORMAT_YVU420;
        // packed single-memory formats: planeCount==1, memCount==1
        case GST_VIDEO_FORMAT_YUY2:
            return DRM_FORMAT_YUYV;
        case GST_VIDEO_FORMAT_UYVY:
            return DRM_FORMAT_UYVY;
#ifdef DRM_FORMAT_YVYU
        case GST_VIDEO_FORMAT_YVYU:
            return DRM_FORMAT_YVYU;
#endif
#ifdef DRM_FORMAT_VYUY
        case GST_VIDEO_FORMAT_VYUY:
            return DRM_FORMAT_VYUY;
#endif
            // AYUV excluded: EGL importers don't support DRM_FORMAT_AYUV; per-plane RGBA path handles it.
#ifdef DRM_FORMAT_Y210
        case GST_VIDEO_FORMAT_Y210:
            return DRM_FORMAT_Y210;
#endif
#ifdef DRM_FORMAT_Y410
        case GST_VIDEO_FORMAT_Y410:
            return DRM_FORMAT_Y410;
#endif
        default:
            return -1;
    }
}

const char* gstFormatNameForImportableFourcc(uint32_t fourcc) noexcept
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    constexpr uint32_t bgraFourcc = DRM_FORMAT_ARGB8888;
    constexpr uint32_t rgbaFourcc = DRM_FORMAT_ABGR8888;
#else
    constexpr uint32_t bgraFourcc = DRM_FORMAT_BGRA8888;
    constexpr uint32_t rgbaFourcc = DRM_FORMAT_RGBA8888;
#endif
    // Restricted to single-image formats the EGLImage/Vulkan import path consumes; mirrors the env-LINEAR offer set.
    if (fourcc == static_cast<uint32_t>(DRM_FORMAT_NV12)) return "NV12";
    if (fourcc == static_cast<uint32_t>(DRM_FORMAT_NV21)) return "NV21";
    if (fourcc == static_cast<uint32_t>(DRM_FORMAT_YUV420)) return "I420";
    if (fourcc == static_cast<uint32_t>(DRM_FORMAT_YVU420)) return "YV12";
    if (fourcc == static_cast<uint32_t>(DRM_FORMAT_P010)) return "P010_10LE";
    if (fourcc == bgraFourcc) return "BGRA";
    if (fourcc == rgbaFourcc) return "RGBA";
    return nullptr;
}

}  // namespace GstHw

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH
