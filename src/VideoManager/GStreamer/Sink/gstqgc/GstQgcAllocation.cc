#include "GstQgcAllocation.h"

#include <gst/video/video.h>
#include <memory>

#include <QtCore/QtEnvironmentVariables>

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

// QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS overrides kProposedMinBuffers, like Qt's
// QT_GSTREAMER_PROPOSE_ALLOCATION_MIN_BUFFERS, e.g. to save CMA memory with 4K decoders on i.MX or Raspberry Pi.
guint proposedMinBuffers()
{
    bool ok = false;
    const int minBuffers = qEnvironmentVariableIntValue("QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS", &ok);
    return (ok && (minBuffers > 0)) ? static_cast<guint>(minBuffers) : kProposedMinBuffers;
}

// Advertise every meta API qgcqvideosink/GstQgcFrameMap consume so upstream keeps them;
// else gst-vaapi/v4l2 strip crop/orientation metas and mapSampleToFrame does a full copy/rotate.
void addConsumedAllocationMetas(GstQuery* query, bool cropMeta = true)
{
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    if (cropMeta) {
        gst_query_add_allocation_meta(query, GST_VIDEO_CROP_META_API_TYPE, NULL);
    }
#if defined(QGC_HAS_GST_VIDEO_ORIENTATION_META)
    gst_query_add_allocation_meta(query, GST_VIDEO_ORIENTATION_META_API_TYPE, NULL);
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    // As upstream's Qt sink asks: glcolorconvert then ends each frame it renders with a sync point, which flushes its
    // context. GstGlVideoBuffer doesn't wait on that fence: it sets one of its own once the frame is mapped.
    gst_query_add_allocation_meta(query, GST_GL_SYNC_META_API_TYPE, NULL);
#endif
}

// A GstVideoBufferPool, unlike a plain GstBufferPool, supports VIDEO_META and attaches GstVideoMeta per buffer, so
// decoders that check the option (avdec direct rendering) decode straight into it instead of copying.
bool tryProposeMetaPool(GstQuery* query, GstCaps* caps, gsize size, guint minBuffers)
{
    const std::unique_ptr<GstBufferPool, GStreamer::GstObjectDeleter> pool{gst_video_buffer_pool_new()};
    if (!pool) {
        return false;
    }

    GstStructure* config = gst_buffer_pool_get_config(pool.get());
    gst_buffer_pool_config_set_params(config, caps, size, minBuffers, 0);
    gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    if (!gst_buffer_pool_set_config(pool.get(), config)) {
        return false;
    }
    gst_query_add_allocation_pool(query, pool.get(), size, minBuffers, 0);
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
    const guint minBuffers = proposedMinBuffers();

    GstCapsFeatures* features = gst_caps_get_features(caps, 0);
    const bool is_system_memory =
        !features || gst_caps_features_is_equal(features, GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY);

    if (!is_system_memory) {
        if (!HwBuffers::proposeDeviceBoundPool(query, caps, &vinfo, size, minBuffers)) {
            gst_query_add_allocation_pool(query, NULL, size, minBuffers, 0);
        }
        // A d3d11 decoder offered crop meta hands over its whole texture when the picture starts off the texture's
        // origin, with the meta saying where; the D3D11 path copies the picture from the origin. Without the offer the
        // decoder crops such a picture itself.
        addConsumedAllocationMetas(query, !gst_caps_features_contains(features, "memory:D3D11Memory"));
        return;
    }

    if (!need_pool) {
        addConsumedAllocationMetas(query);
        return;
    }

    (void) tryProposeMetaPool(query, caps, size, minBuffers);
    addConsumedAllocationMetas(query);
}

}  // namespace GstQgc
