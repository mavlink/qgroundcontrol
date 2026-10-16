#pragma once

#include <gst/gst.h>
#include <gst/video/video-info.h>

namespace GstHw {

/// Parse @p caps into @p info, handling the GStreamer 1.24+ DMA_DRM format that plain gst_video_info_from_caps cannot
/// decode. False on parse failure or null args.
bool dmaDrmAwareVideoInfo(GstCaps* caps, GstVideoInfo* info);

/// Size of @p plane, taken from the first component stored in it: plane and component indices differ in general (AV12's
/// third plane holds component 3, the full-height alpha, not the half-height V component 2). 0 for a plane without
/// components.
int planeWidth(const GstVideoInfo& info, int plane);
int planeHeight(const GstVideoInfo& info, int plane);

/// Bytes of image data in one row of @p plane, i.e. the stride without its alignment padding: up to the last byte of
/// any component stored in the plane.
int planeRowBytes(const GstVideoInfo& info, int plane);

}  // namespace GstHw
