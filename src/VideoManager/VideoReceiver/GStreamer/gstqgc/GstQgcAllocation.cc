#include "GstQgcAllocation.h"

#include <QtCore/qglobal.h>

#include <gst/video/gstvideometa.h>
#include <gst/video/video-info.h>

#include "HwBuffers/dmabuf/GstDmaDrmCaps.h"
#include "HwBuffers/common/HwBuffers.h"
#if GST_CHECK_VERSION(1, 24, 0)
#include <gst/video/video-info-dma.h>
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include <gst/gl/gstglsyncmeta.h>
#endif
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include <gst/d3d11/gstd3d11.h>
#include "HwBuffers/d3d/GstD3D11ContextBridge.h"
#endif
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include <gst/d3d12/gstd3d12.h>
#include "HwBuffers/d3d/GstD3D12ContextBridge.h"
#endif

namespace GstQgc {

namespace {

// VAAPI/H.264 ref-frame queue typically 4–8; min=2 forced fallback allocations.
constexpr guint kProposedMinBuffers = 4;

// Advertise every meta API qgcqvideosink/GStreamerFrameMap consume so upstream keeps them;
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

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
// Bind allocation to QRhi's shared device so import stays same-device instead of hitting
// GstD3D11VideoBuffer's foreign-device copy path; SHADER_RESOURCE matches the importer's sampled texture.
bool tryProposeD3D11Pool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size)
{
    GstD3D11Device* device = GstD3D11ContextBridge::currentDevice();
    if (!device) {
        return false;
    }
    bool proposed = false;
    if (GstBufferPool* pool = gst_d3d11_buffer_pool_new(device)) {
        GstStructure* config = gst_buffer_pool_get_config(pool);
        gst_buffer_pool_config_set_params(config, caps, size, kProposedMinBuffers, 0);
        gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
        if (GstD3D11AllocationParams* params = gst_d3d11_allocation_params_new(
                device, vinfo, GST_D3D11_ALLOCATION_FLAG_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0)) {
            gst_buffer_pool_config_set_d3d11_allocation_params(config, params);
            gst_d3d11_allocation_params_free(params);
        }
        if (gst_buffer_pool_set_config(pool, config)) {
            // The d3d11 pool may grow the buffer size to the allocated texture's pitch; re-read it.
            GstStructure* updated = gst_buffer_pool_get_config(pool);
            guint poolSize = static_cast<guint>(size);
            gst_buffer_pool_config_get_params(updated, nullptr, &poolSize, nullptr, nullptr);
            gst_structure_free(updated);
            gst_query_add_allocation_pool(query, pool, poolSize, kProposedMinBuffers, 0);
            proposed = true;
        }
        gst_object_unref(pool);
    }
    gst_object_unref(device);  // currentDevice() returns a transfer-full ref.
    return proposed;
}
#endif

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
// D3D12 counterpart of tryProposeD3D11Pool — steers allocation onto QRhi's shared (LUID-matched) adapter.
bool tryProposeD3D12Pool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size)
{
    GstD3D12Device* device = GstD3D12ContextBridge::currentDevice();
    if (!device) {
        return false;
    }
    bool proposed = false;
    if (GstBufferPool* pool = gst_d3d12_buffer_pool_new(device)) {
        GstStructure* config = gst_buffer_pool_get_config(pool);
        gst_buffer_pool_config_set_params(config, caps, size, kProposedMinBuffers, 0);
        gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
        if (GstD3D12AllocationParams* params = gst_d3d12_allocation_params_new(
                device, vinfo, GST_D3D12_ALLOCATION_FLAG_DEFAULT, D3D12_RESOURCE_FLAG_NONE,
                D3D12_HEAP_FLAG_NONE)) {
            gst_buffer_pool_config_set_d3d12_allocation_params(config, params);
            gst_d3d12_allocation_params_free(params);
        }
        if (gst_buffer_pool_set_config(pool, config)) {
            GstStructure* updated = gst_buffer_pool_get_config(pool);
            guint poolSize = static_cast<guint>(size);
            gst_buffer_pool_config_get_params(updated, nullptr, &poolSize, nullptr, nullptr);
            gst_structure_free(updated);
            gst_query_add_allocation_pool(query, pool, poolSize, kProposedMinBuffers, 0);
            proposed = true;
        }
        gst_object_unref(pool);
    }
    gst_object_unref(device);
    return proposed;
}
#endif

// Propose a shared-device pool for D3D11/D3D12 caps; false for other HW memory or before the bridge is primed.
bool tryProposeDeviceBoundPool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size)
{
    GstCapsFeatures* features = gst_caps_get_features(caps, 0);
    if (!features) {
        return false;
    }
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    if (gst_caps_features_contains(features, GST_CAPS_FEATURE_MEMORY_D3D11_MEMORY)) {
        return tryProposeD3D11Pool(query, caps, vinfo, size);
    }
#endif
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (gst_caps_features_contains(features, GST_CAPS_FEATURE_MEMORY_D3D12_MEMORY)) {
        return tryProposeD3D12Pool(query, caps, vinfo, size);
    }
#endif
    (void) query;
    (void) vinfo;
    (void) size;
    return false;
}

bool tryProposeMetaPool(GstQuery* query, GstCaps* caps, gsize size)
{
    GstBufferPool* pool = gst_buffer_pool_new();
    if (!pool) {
        return false;
    }

    bool proposed = false;
    GstStructure* config = gst_buffer_pool_get_config(pool);
    gst_buffer_pool_config_set_params(config, caps, size, kProposedMinBuffers, 0);
    gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    if (gst_buffer_pool_set_config(pool, config)) {
        gst_query_add_allocation_pool(query, pool, size, kProposedMinBuffers, 0);
        proposed = true;
    }
    gst_object_unref(pool);
    return proposed;
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
        if (!tryProposeDeviceBoundPool(query, caps, &vinfo, size)) {
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

GstPadProbeReturn videosinkQueryProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    (void) pad;
    (void) user_data;
    GstQuery* query = GST_PAD_PROBE_INFO_QUERY(info);
    if (!query) {
        return GST_PAD_PROBE_OK;
    }

    switch (GST_QUERY_TYPE(query)) {
        case GST_QUERY_ALLOCATION:
            populateAllocationQuery(query);
            return GST_PAD_PROBE_HANDLED;
        case GST_QUERY_CONTEXT:
            // Synchronous answer for gst.gl.GLDisplay/app_context — bus NEED_CONTEXT fallback races state changes and
            // can isolate glupload from Qt's RHI context.
            if (HwBuffers::answerSinkBinContextQuery(query)) {
                return GST_PAD_PROBE_HANDLED;
            }
            return GST_PAD_PROBE_OK;
        default:
            return GST_PAD_PROBE_OK;
    }
}

}  // namespace GstQgc
