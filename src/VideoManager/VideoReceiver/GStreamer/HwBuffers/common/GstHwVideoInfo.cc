#include "GstHwVideoInfo.h"

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
    const int component = firstComponentOf(info, plane);
    if (component < 0) {
        return 0;
    }
    return GST_VIDEO_INFO_COMP_WIDTH(&info, component) * GST_VIDEO_INFO_COMP_PSTRIDE(&info, component);
}

}  // namespace GstHw
