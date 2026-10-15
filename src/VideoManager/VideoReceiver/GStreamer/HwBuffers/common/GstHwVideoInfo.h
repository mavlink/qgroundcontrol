#pragma once

#include <gst/gst.h>
#include <gst/video/video-info.h>

namespace GstHw {

/// Parse @p caps into @p info, handling the GStreamer 1.24+ DMA_DRM format that plain gst_video_info_from_caps cannot
/// decode. False on parse failure or null args.
bool dmaDrmAwareVideoInfo(GstCaps* caps, GstVideoInfo* info);

}  // namespace GstHw
