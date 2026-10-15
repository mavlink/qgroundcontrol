#include "GstHwVideoInfo.h"

#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif

namespace GstHw {

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

}  // namespace GstHw
