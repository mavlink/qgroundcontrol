#include "GstQgcAllocation.h"

#include <gst/video/video.h>
#include <memory>

#include "GstScoped.h"
#include "HwBuffers/common/CpuVideoFramePool.h"
#include "HwBuffers/common/GstHwVideoInfo.h"
#include "HwBuffers/common/HwBuffers.h"
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include <gst/gl/gstglsyncmeta.h>
#endif

namespace GstQgc {

namespace {

// Most decoder buffers downstream holds at once: kMaxPinnedFrames, plus the texture bundle Qt moves out of a reused
// in-flight slot and keeps (with its sample) until that render frame ends.
constexpr guint kProposedMinBuffers = CpuVideoFramePool::kMaxPinnedFrames + 1;

// Advertise every meta API qgcqvideosink/GstQgcFrameMap consume so upstream keeps them;
// else gst-vaapi/v4l2 strip crop/orientation metas and mapSampleToFrame does a full copy/rotate.
void addConsumedAllocationMetas(GstQuery* query)
{
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    gst_query_add_allocation_meta(query, GST_VIDEO_CROP_META_API_TYPE, NULL);
#if defined(QGC_HAS_GST_VIDEO_ORIENTATION_META)
    gst_query_add_allocation_meta(query, GST_VIDEO_ORIENTATION_META_API_TYPE, NULL);
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    // glupload reads this to skip its own upload pass when upstream already provides GL texture.
    gst_query_add_allocation_meta(query, GST_VIDEO_GL_TEXTURE_UPLOAD_META_API_TYPE, NULL);
    // Let the producer attach a sync meta and set its fence on the producing context; GstGlVideoBuffer
    // waits on it at import time. Without this it synthesizes a same-context fence after the fact, which
    // doesn't synchronize against the real producer — a cross-context tearing hazard on the Qt render thread.
    gst_query_add_allocation_meta(query, GST_GL_SYNC_META_API_TYPE, NULL);
#endif
}

// A GstVideoBufferPool, unlike a plain GstBufferPool, supports VIDEO_META and attaches GstVideoMeta per buffer, so
// decoders that check the option (avdec direct rendering) decode straight into it instead of copying.
bool tryProposeMetaPool(GstQuery* query, GstCaps* caps, gsize size)
{
    const std::unique_ptr<GstBufferPool, GStreamer::GstObjectDeleter> pool{gst_video_buffer_pool_new()};
    if (!pool) {
        return false;
    }

    GstStructure* config = gst_buffer_pool_get_config(pool.get());
    gst_buffer_pool_config_set_params(config, caps, size, kProposedMinBuffers, 0);
    gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    if (!gst_buffer_pool_set_config(pool.get(), config)) {
        return false;
    }
    gst_query_add_allocation_pool(query, pool.get(), size, kProposedMinBuffers, 0);
    return true;
}

}  // namespace

void populateAllocationQuery(GstQuery* query)
{
    GstCaps* caps = nullptr;
    gboolean need_pool = FALSE;
    gst_query_parse_allocation(query, &caps, &need_pool);
    if (!caps) {
        return;
    }

    GstVideoInfo vinfo;
    if (!GstHw::dmaDrmAwareVideoInfo(caps, &vinfo)) {
        return;
    }
    const gsize size = GST_VIDEO_INFO_SIZE(&vinfo);

    GstCapsFeatures* features = gst_caps_get_features(caps, 0);
    const bool is_system_memory =
        !features || gst_caps_features_is_equal(features, GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY);

    if (!is_system_memory) {
        if (!HwBuffers::proposeDeviceBoundPool(query, caps, &vinfo, size, kProposedMinBuffers)) {
            gst_query_add_allocation_pool(query, NULL, size, kProposedMinBuffers, 0);
        }
        addConsumedAllocationMetas(query);
        return;
    }

    if (!need_pool) {
        addConsumedAllocationMetas(query);
        return;
    }

    (void) tryProposeMetaPool(query, caps, size);
    addConsumedAllocationMetas(query);
}

}  // namespace GstQgc
