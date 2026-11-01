#include "GstD3DVideoBufferCommon.h"

#if defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <d3d12.h>
#include <gst/d3d12/gstd3d12.h>
#include <utility>

#include "GstD3D12ContextBridge.h"
#include "GstD3D12StagingPool.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstD3D12Log, "Video.GStreamer.HwBuffers.GstD3D12Buf")

namespace {

using GstD3DVideoBufferCommon::kMaxPlanes;
using GstD3DVideoBufferCommon::MapDiagnostics;

MapDiagnostics s_diag;
std::atomic<bool> s_loggedMultiPlane{false};

using PlaneLeases = std::array<std::shared_ptr<GstD3D12StagingPool::SlotLease>, kMaxPlanes>;

/// D3D12 pieces of GstD3DVideoBufferCommon's resolvePlanes() and proposePool(). Staged planes share one copy queue,
/// resolved on the first staged plane (constant per device); finish() does a single fence wait on the highest submitted
/// value. An early return still waits for submitted copies (destructor) before the slot leases are released.
class D3D12PlaneResolver
{
public:
    using Handle = ID3D12Resource;
    using Memory = GstD3D12Memory;
    using Device = GstD3D12Device;

    static constexpr const char* kMemoryName = "GstD3D12Memory";
    static constexpr const char* kCapsFeature = GST_CAPS_FEATURE_MEMORY_D3D12_MEMORY;

    D3D12PlaneResolver() = default;
    D3D12PlaneResolver(const D3D12PlaneResolver&) = delete;
    D3D12PlaneResolver& operator=(const D3D12PlaneResolver&) = delete;

    ~D3D12PlaneResolver()
    {
        (void) waitForCopies();
        if (_queue) {
            gst_object_unref(_queue);
        }
    }

    static const QLoggingCategory& log() { return GstD3D12Log(); }

    static bool isMemory(GstMemory* mem) { return gst_is_d3d12_memory(mem); }

    static Device* bridgeDevice() { return GstD3D12ContextBridge::currentDevice(); }

    // Compare the underlying device, not the wrapper: each bridge prime creates a new GstD3D12Device.
    static bool sameDevice(Device* a, Device* b) { return gst_d3d12_device_is_equal(a, b); }

    static Handle* resource(Memory* mem) { return gst_d3d12_memory_get_resource_handle(mem); }

    static guint subresource(Memory* mem, int plane)
    {
        guint subIdx = 0;
        gst_d3d12_memory_get_subresource_index(mem, guint(plane), &subIdx);
        return subIdx;
    }

    static bool isArraySlice(Handle* resource, guint subIdx)
    {
        return subIdx > 0 || resource->GetDesc().DepthOrArraySize > 1;
    }

    // Block on the decoder's fence before reading, else QRhi samples mid-write while the decoder is still writing this
    // resource.
    static bool prepare(Memory* mem, int plane)
    {
        if (!gst_d3d12_memory_sync(mem)) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedNullResource,
                             "resolve: gst_d3d12_memory_sync failed for plane" << plane);
            return false;
        }
        return true;
    }

    Handle* copySlice(Handle* resource, guint subIdx, int plane, Memory* mem)
    {
        ID3D12Device* d3dDev = gst_d3d12_device_get_device_handle(mem->device);
        if (!_queue) {
            _queueType = D3D12_COMMAND_LIST_TYPE_COPY;
            GstD3D12CmdQueue* q = gst_d3d12_device_get_cmd_queue(mem->device, _queueType);
            if (!q) {
                _queueType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                q = gst_d3d12_device_get_cmd_queue(mem->device, _queueType);
            }
            if (q) {
                // Hold a ref for the whole resolve so it serializes against gst-d3d12's queue lock like the
                // decoder-thread submits do.
                _queue = static_cast<GstD3D12CmdQueue*>(gst_object_ref(q));
            }
        }
        if (!d3dDev || !_queue) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                             "mapTextures: could not obtain D3D12 device/queue for slice copy (plane=" << plane << ")");
            return nullptr;
        }
        Handle* staging = GstD3D12StagingPool::copySlice(resource, subIdx, plane, resource->GetDesc(), d3dDev, _queue,
                                                         _queueType, _maxFenceValue, _leases[plane], s_diag);
        if (staging) {
            _anyCopy = true;
        }
        return staging;
    }

    // One fence wait on the highest value covers every plane (same queue, in order) and guarantees the pooled
    // allocators are idle before the next frame resets them.
    bool finish() { return waitForCopies(); }

    /// Slot leases of the staged planes; null when nothing was staged.
    std::shared_ptr<void> takeKeepAlive()
    {
        return _anyCopy ? std::make_shared<PlaneLeases>(std::move(_leases)) : nullptr;
    }

    static GstBufferPool* newPool(Device* device) { return gst_d3d12_buffer_pool_new(device); }

    static void setAllocationParams(GstStructure* config, Device* device, const GstVideoInfo* vinfo)
    {
        if (GstD3D12AllocationParams* params = gst_d3d12_allocation_params_new(
                device, vinfo, GST_D3D12_ALLOCATION_FLAG_DEFAULT, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_FLAG_NONE)) {
            gst_buffer_pool_config_set_d3d12_allocation_params(config, params);
            gst_d3d12_allocation_params_free(params);
        }
    }

private:
    bool waitForCopies()
    {
        if (!_anyCopy || !_queue || _waited) {
            return true;
        }
        _waited = true;
        GstHwPathTelemetry::record(HwVideoBufferPath::D3D12, GstHwPathTelemetry::Counter::CpuWaits);
        if (FAILED(gst_d3d12_cmd_queue_fence_wait(_queue, _maxFenceValue))) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                             "resolve: gst_d3d12_cmd_queue_fence_wait failed");
            return false;
        }
        return true;
    }

    PlaneLeases _leases;
    GstD3D12CmdQueue* _queue = nullptr;
    D3D12_COMMAND_LIST_TYPE _queueType = D3D12_COMMAND_LIST_TYPE_COPY;
    guint64 _maxFenceValue = 0;
    bool _anyCopy = false;
    bool _waited = false;
};

/// Wraps a D3D12Memory-backed GstSample as a QHwVideoBuffer. The device guard, decoder sync and slice copies run in the
/// constructor, on the streaming thread (blocking there is harmless: the decoder is async), so mapTextures() on the
/// render thread only imports resolved resources. D3D11 has its own buffer: it shares QRhi's immediate context, so it
/// copies on the render thread instead.
class D3D12VideoBuffer final : public GstD3DVideoBufferCommon::VideoBuffer<D3D12PlaneResolver>
{
public:
    D3D12VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format)
        : VideoBuffer(sample, videoInfo, format, HwVideoBufferPath::D3D12, s_diag)
    {
        // Qt's D3D12 QRhi creates no per-plane views, so it can't sample an NV12/P010 resource's chroma. The
        // D3D12Convert bin negotiates single-plane formats only; anything else takes the CPU path.
        if (const auto* description = QVideoTextureHelper::textureDescription(format.pixelFormat());
            !description || (description->nplanes != 1)) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_loggedMultiPlane,
                             "resolve: multi-plane D3D12Memory" << format.pixelFormat() << "can't be sampled by QRhi");
            return;
        }
        D3D12PlaneResolver resolver;
        if (!resolve(resolver)) {
            return;
        }
        _keepAlive = resolver.takeKeepAlive();
    }

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old) override
    {
        // The device bridge must be primed: resources on another device import fine but render corrupt.
        if (!beginMap(rhi, QRhi::D3D12)) {
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
        }
        return finishMap(rhi, old, _handles, _keepAlive);
    }

private:
    /// Staging slot leases the frame bundles share while QRhi samples them.
    std::shared_ptr<void> _keepAlive;
};

void resetCachedState() noexcept
{
    GstD3D12StagingPool::clear();
    s_diag.reset();
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kD3D12 = {
    .path = HwVideoBufferPath::D3D12,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool { return gst_is_d3d12_memory(mem); },
    .construct = &GstHwPaths::construct<D3D12VideoBuffer>,
    .resetCaches = &resetCachedState,
    .proposePool = &GstD3DVideoBufferCommon::proposePool<D3D12PlaneResolver>,
    .bridge = &GstD3D12ContextBridge::instance,
};

#endif  // QGC_HAS_GST_D3D12_GPU_PATH
