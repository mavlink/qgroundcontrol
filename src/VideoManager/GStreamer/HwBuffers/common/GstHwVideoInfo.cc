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

bool hasCpuLayout([[maybe_unused]] GstCaps* caps)
{
#if GST_CHECK_VERSION(1, 24, 0)
    if (caps && gst_video_is_dma_drm_caps(caps)) {
        // DRM_FORMAT_MOD_LINEAR; <drm_fourcc.h> is Linux-only.
        constexpr guint64 kLinearModifier = 0;
        GstVideoInfoDmaDrm drmInfo;
        gst_video_info_dma_drm_init(&drmInfo);
        return gst_video_info_dma_drm_from_caps(&drmInfo, caps) && (drmInfo.drm_modifier == kLinearModifier);
    }
#endif
    return true;
}

int planeHeight(const GstVideoInfo& info, int plane)
{
    const int component = firstComponentOf(info, plane);
    if (component < 0) {
        return 0;
    }
    return GST_VIDEO_INFO_COMP_HEIGHT(&info, component);
}

}  // namespace GstHw
