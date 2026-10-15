#include "GstD3DVideoBufferCommon.h"

#if defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <algorithm>
#include <array>
#include <cstddef>
#include <d3d12.h>
#include <gst/d3d12/gstd3d12.h>
#include <utility>

#include <QtCore/QMutex>

#include "GstD3D12ContextBridge.h"
#include "GstHwImportCache.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstD3D12Log, "Video.GStreamer.HwBuffers.GstD3D12Buf")

namespace {

using GstD3DVideoBufferCommon::kMaxPlanes;
using GstD3DVideoBufferCommon::MapDiagnostics;
using GstD3DVideoBufferCommon::StagingKey;

MapDiagnostics s_diag;

/// One ring slot's COPY/DIRECT-queue objects: staging resource plus the allocator/list that record its slice copy.
struct StagingEntry
{
    ID3D12Resource* resource = nullptr;
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12GraphicsCommandList* list = nullptr;
};

void releaseEntryResources(const StagingEntry& e)
{
    if (e.list)
        e.list->Release();
    if (e.allocator)
        e.allocator->Release();
    if (e.resource)
        e.resource->Release();
}

class StagingEntryRef
{
public:
    StagingEntryRef() = default;

    explicit StagingEntryRef(const StagingEntry& e) : _entry(e) {}

    ~StagingEntryRef() { releaseEntryResources(_entry); }

    StagingEntryRef(const StagingEntryRef&) = delete;
    StagingEntryRef& operator=(const StagingEntryRef&) = delete;

    StagingEntryRef(StagingEntryRef&& other) noexcept : _entry(other._entry) { other._entry = {}; }

    StagingEntryRef& operator=(StagingEntryRef&& other) noexcept
    {
        if (this != &other) {
            releaseEntryResources(_entry);
            _entry = other._entry;
            other._entry = {};
        }
        return *this;
    }

    bool valid() const noexcept { return _entry.resource && _entry.allocator && _entry.list; }

    ID3D12Resource* resource() const noexcept { return _entry.resource; }

    ID3D12CommandAllocator* allocator() const noexcept { return _entry.allocator; }

    ID3D12GraphicsCommandList* list() const noexcept { return _entry.list; }

private:
    StagingEntry _entry;
};

StagingEntryRef addRefEntry(const StagingEntry& e)
{
    if (e.resource)
        e.resource->AddRef();
    if (e.allocator)
        e.allocator->AddRef();
    if (e.list)
        e.list->AddRef();
    return StagingEntryRef(e);
}

// Covers the slots a stream keeps leased (Qt's FramesInFlight bundles, the frames queued towards them and the one being
// resolved) plus spares, so round-robin reuses a slot released at least one acquisition earlier, after QRhi's last
// sample of it. Exhaustion (e.g. several same-key streams) fails the resolve and demotes the frame to CPU.
constexpr std::size_t kStagingRingSize = 8;

using StagingLeaseFlags = std::array<std::atomic<bool>, kStagingRingSize>;

/// Holds one ring slot, plus refs on its COM objects, from acquire() until the last holder (the D3D12 video buffer and
/// the frame bundle QRhi samples) drops it; another stream or a later frame can't reset its allocator/list or overwrite
/// its resource meanwhile. Its copy has completed by the time the first holder exists (the resolve fence-waits), so the
/// next acquirer may Reset() the allocator immediately.
class StagingSlotLease
{
public:
    StagingSlotLease(std::shared_ptr<StagingLeaseFlags> flags, std::size_t slot, StagingEntryRef entry)
        : _flags(std::move(flags))
        , _slot(slot)
        , _entry(std::move(entry))
    {}

    ~StagingSlotLease() { (*_flags)[_slot].store(false, std::memory_order_release); }

    StagingSlotLease(const StagingSlotLease&) = delete;
    StagingSlotLease& operator=(const StagingSlotLease&) = delete;

    const StagingEntryRef& entry() const noexcept { return _entry; }

private:
    std::shared_ptr<StagingLeaseFlags> _flags;
    std::size_t _slot;
    StagingEntryRef _entry;
};

/// Per (size, format, plane) ring of staging slots, each with its own allocator/list so concurrent streams never record
/// into the same command objects. The pool owns one ref on each created COM object; leases add their own.
class StagingResourcePool
{
public:
    static StagingResourcePool& instance()
    {
        static StagingResourcePool pool;
        return pool;
    }

    ~StagingResourcePool() { clear(); }

    /// Leases the next free slot for @p key round-robin, creating its objects on first use. nullptr when creation fails
    /// or every slot is leased.
    std::shared_ptr<StagingSlotLease> acquire(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE queueType,
                                              const StagingKey& key, const D3D12_RESOURCE_DESC& dstDesc,
                                              const D3D12_HEAP_PROPERTIES& heapProps, int planeIdx, guint subIdx)
    {
        QMutexLocker lock(&_mutex);
        StagingRing* ring = _entries.find(key).resource;
        if (!ring) {
            ring = _entries.insert(key, StagingRing{}).resource;
        }
        for (std::size_t probe = 0; probe < kStagingRingSize; ++probe) {
            const std::size_t slot = (ring->next + probe) % kStagingRingSize;
            if ((*ring->leased)[slot].exchange(true, std::memory_order_acq_rel)) {
                continue;
            }
            StagingEntry& e = ring->entries[slot];
            if (e.resource) {
                GstHwPathTelemetry::record(HwVideoBufferPath::D3D12, GstHwPathTelemetry::Counter::ImageCacheHits);
            } else if (createEntry(dev, queueType, dstDesc, heapProps, planeIdx, subIdx, e)) {
                GstHwPathTelemetry::record(HwVideoBufferPath::D3D12, GstHwPathTelemetry::Counter::ImageCacheMisses);
            } else {
                (*ring->leased)[slot].store(false, std::memory_order_release);
                return nullptr;
            }
            ring->next = (slot + 1) % kStagingRingSize;
            return std::make_shared<StagingSlotLease>(ring->leased, slot, addRefEntry(e));
        }
        QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                         "resolve: every staging slot is in use (plane=" << planeIdx << ") — CPU fallback");
        return nullptr;
    }

    void clear()
    {
        QMutexLocker lock(&_mutex);
        _entries.clear();
    }

private:
    struct StagingRing
    {
        std::array<StagingEntry, kStagingRingSize> entries{};
        std::shared_ptr<StagingLeaseFlags> leased = std::make_shared<StagingLeaseFlags>();
        std::size_t next = 0;
    };

    static bool createEntry(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE queueType, const D3D12_RESOURCE_DESC& dstDesc,
                            const D3D12_HEAP_PROPERTIES& heapProps, int planeIdx, guint subIdx, StagingEntry& e)
    {
        if (FAILED(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &dstDesc, D3D12_RESOURCE_STATE_COMMON,
                                                nullptr, IID_PPV_ARGS(&e.resource)))) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                             "mapTextures: CreateCommittedResource for slice copy failed (plane="
                                 << planeIdx << " subresource=" << subIdx << ")");
            e = {};
            return false;
        }
        if (FAILED(dev->CreateCommandAllocator(queueType, IID_PPV_ARGS(&e.allocator))) ||
            FAILED(dev->CreateCommandList(0, queueType, e.allocator, nullptr, IID_PPV_ARGS(&e.list)))) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                             "mapTextures: command allocator/list create failed (plane=" << planeIdx << ")");
            releaseEntryResources(e);
            e = {};
            return false;
        }
        // Freshly created lists are open; close so the per-frame Reset() path is uniform.
        e.list->Close();
        return true;
    }

    static void releaseRing(StagingRing& ring)
    {
        for (StagingEntry& e : ring.entries) {
            releaseEntryResources(e);
            e = {};
        }
    }

    static constexpr std::size_t kMaxEntries = 8;
    QMutex _mutex;
    GstHw::GstHwImportCache<StagingKey, StagingRing, kMaxEntries> _entries{&releaseRing};
};

/// Copy one subresource slice into a leased COPY-queue (DIRECT fallback) staging slot and submit the copy WITHOUT
/// waiting; the caller does a single fence wait after all planes. On success returns the staging resource AddRef'd
/// (caller owns the ref), hands the slot lease to @p lease and advances @p maxFenceValue to the highest submitted fence
/// value. nullptr on failure.
ID3D12Resource* copySliceToStaging(ID3D12Resource* resource, guint subIdx, int planeIdx,
                                   const D3D12_RESOURCE_DESC& srcDesc, ID3D12Device* d3dDev, GstD3D12CmdQueue* cmdQueue,
                                   D3D12_COMMAND_LIST_TYPE queueType, guint64& maxFenceValue,
                                   std::shared_ptr<StagingSlotLease>& lease)
{
    D3D12_RESOURCE_DESC dstDesc = srcDesc;
    dstDesc.DepthOrArraySize = 1;
    dstDesc.MipLevels = 1;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    const StagingKey key{srcDesc.Width, srcDesc.Height, UINT(srcDesc.Format), planeIdx};
    std::shared_ptr<StagingSlotLease> slotLease =
        StagingResourcePool::instance().acquire(d3dDev, queueType, key, dstDesc, heapProps, planeIdx, subIdx);
    if (!slotLease || !slotLease->entry().valid()) {
        return nullptr;
    }
    const StagingEntryRef& entry = slotLease->entry();

    if (FAILED(entry.allocator()->Reset()) || FAILED(entry.list()->Reset(entry.allocator(), nullptr))) {
        QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                         "mapTextures: command allocator/list reset failed (plane=" << planeIdx << ")");
        return nullptr;
    }
    ID3D12GraphicsCommandList* cmdList = entry.list();

    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = resource;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = subIdx;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = entry.resource();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    // Decoder output is parked in COMMON by gst_d3d12_memory_sync; gst-d3d12 exposes no per-resource state getter, so
    // COMMON is the only valid before-state.
    D3D12_RESOURCE_BARRIER toCopySrc = {};
    toCopySrc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toCopySrc.Transition.pResource = resource;
    toCopySrc.Transition.Subresource = subIdx;
    toCopySrc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    toCopySrc.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    cmdList->ResourceBarrier(1, &toCopySrc);

    cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

    D3D12_RESOURCE_BARRIER toCommon = toCopySrc;
    toCommon.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    toCommon.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
    cmdList->ResourceBarrier(1, &toCommon);

    cmdList->Close();

    ID3D12CommandList* lists[] = {cmdList};
    guint64 fenceValue = 0;
    // Wrapper execute takes gst-d3d12's queue lock, serializing the copy against decoder-thread submits.
    if (FAILED(gst_d3d12_cmd_queue_execute_command_lists(cmdQueue, 1, lists, &fenceValue))) {
        // The caller skips the fence wait on failure, so the pooled allocator's GPU work may still be in flight; drop
        // the pool so the next frame rebuilds clean command objects instead of resetting a busy allocator.
        StagingResourcePool::instance().clear();
        return nullptr;
    }
    if (fenceValue > maxFenceValue) {
        maxFenceValue = fenceValue;
    }
    ID3D12Resource* stagingResource = entry.resource();
    stagingResource->AddRef();
    lease = std::move(slotLease);
    return stagingResource;
}

using PlaneLeases = std::array<std::shared_ptr<StagingSlotLease>, kMaxPlanes>;

/// D3D12 pieces of GstD3DVideoBufferCommon's resolvePlanes(), VideoBuffer and proposePool(). Staged planes share one
/// copy queue, resolved on the first staged plane (constant per device); finish() does a single fence wait on the
/// highest submitted value. An early return still waits for submitted copies (destructor) before the slot leases are
/// released.
class D3D12PlaneResolver
{
public:
    using Handle = ID3D12Resource;
    using Memory = GstD3D12Memory;
    using Device = GstD3D12Device;

    static constexpr const char* kMemoryName = "GstD3D12Memory";
    static constexpr HwVideoBufferPath kPath = HwVideoBufferPath::D3D12;
    static constexpr QRhi::Implementation kBackend = QRhi::D3D12;
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

    static MapDiagnostics& diag() { return s_diag; }

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
        Handle* staging = copySliceToStaging(resource, subIdx, plane, resource->GetDesc(), d3dDev, _queue, _queueType,
                                             _maxFenceValue, _leases[plane]);
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

void resetCachedState() noexcept
{
    StagingResourcePool::instance().clear();
    s_diag.reset();
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kD3D12 = {
    .path = HwVideoBufferPath::D3D12,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool { return gst_is_d3d12_memory(mem); },
    .construct = &GstHwPaths::construct<GstD3DVideoBufferCommon::VideoBuffer<D3D12PlaneResolver>>,
    .resetCaches = &resetCachedState,
    .proposePool = &GstD3DVideoBufferCommon::proposePool<D3D12PlaneResolver>,
    .bridge = &GstD3D12ContextBridge::instance,
};

#endif  // QGC_HAS_GST_D3D12_GPU_PATH
