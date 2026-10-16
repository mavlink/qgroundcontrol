#include "GstHwVideoInfo.h"

#include <algorithm>

#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif

namespace GstHw {

namespace {

int firstComponentOf(const GstVideoInfo& info, int plane)
{
    gint components[GST_VIDEO_MAX_COMPONENTS];
    gst_video_format_info_component(info.finfo, static_cast<guint>(plane), components);
    return components[0];
}

}  // namespace

bool dmaDrmAwareVideoInfo(GstCaps* caps, GstVideoInfo* info)
{
    if (!caps || !info) {
        return false;
    }
#if GST_CHECK_VERSION(1, 24, 0)
    if (gst_video_is_dma_drm_caps(caps)) {
        GstVideoInfoDmaDrm drmInfo;
        gst_video_info_dma_drm_init(&drmInfo);
        return gst_video_info_dma_drm_from_caps(&drmInfo, caps) && gst_video_info_dma_drm_to_video_info(&drmInfo, info);
    }
#endif
    return gst_video_info_from_caps(info, caps);
}

int planeWidth(const GstVideoInfo& info, int plane)
{
    const int component = firstComponentOf(info, plane);
    if (component < 0) {
        return 0;
    }
    return GST_VIDEO_INFO_COMP_WIDTH(&info, component);
}

int planeHeight(const GstVideoInfo& info, int plane)
{
    const int component = firstComponentOf(info, plane);
    if (component < 0) {
        return 0;
    }
    return GST_VIDEO_INFO_COMP_HEIGHT(&info, component);
}

int planeRowBytes(const GstVideoInfo& info, int plane)
{
    // The furthest byte any component of the plane reaches: packed 4:2:2 (YUY2, UYVY) interleaves half-width chroma
    // with luma, so an odd-width row ends in a chroma sample past the last luma byte.
    gint components[GST_VIDEO_MAX_COMPONENTS];
    gst_video_format_info_component(info.finfo, static_cast<guint>(plane), components);
    int rowBytes = 0;
    for (const gint component : components) {
        if (component < 0) {
            break;
        }
        const int pixelStride = GST_VIDEO_INFO_COMP_PSTRIDE(&info, component);
        if (pixelStride <= 0) {
            // Complex packings (e.g. v210) have no per-pixel stride; the whole stride is the safe bound.
            return GST_VIDEO_INFO_PLANE_STRIDE(&info, plane);
        }
        const int sampleBytes = static_cast<int>((GST_VIDEO_FORMAT_INFO_DEPTH(info.finfo, component) +
                                                  GST_VIDEO_FORMAT_INFO_SHIFT(info.finfo, component) + 7U) /
                                                 8U);
        const int end = GST_VIDEO_INFO_COMP_POFFSET(&info, component) +
                        ((GST_VIDEO_INFO_COMP_WIDTH(&info, component) - 1) * pixelStride) + sampleBytes;
        rowBytes = std::max(rowBytes, end);
    }
    return rowBytes;
}

}  // namespace GstHw
