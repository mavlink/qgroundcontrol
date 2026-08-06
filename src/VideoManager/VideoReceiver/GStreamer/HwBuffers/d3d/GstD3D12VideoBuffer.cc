#include "GstD3D12VideoBuffer.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <QtCore/QMutex>
#include <algorithm>
#include <array>
#include <cstddef>
#include <d3d12.h>
#include <gst/d3d12/gstd3d12.h>
#include <utility>

#include "GstContextBridgeRegistry.h"
#include "GstD3D12ContextBridge.h"
#include "GstD3DContextBridgeCommon.h"
#include "GstD3DVideoBufferCommon.h"
#include "GstHwImportCache.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstD3D12Log, "Video.GStreamer.HwBuffers.GstD3D12Buf")

namespace {

using GstD3DVideoBufferCommon::kMaxPlanes;
using GstD3DVideoBufferCommon::MapDiagnostics;
using D3D12FrameTextures = GstD3DVideoBufferCommon::FrameTextures<ID3D12Resource>;
using GstD3DVideoBufferCommon::StagingKey;
using GstD3DVideoBufferCommon::StagingKeyHash;

MapDiagnostics s_diag;

/// Per-key pooled COPY/DIRECT-queue objects: staging resource plus the allocator/list that record the slice copy.
/// Per-frame allocator Reset() is safe because resolvePlaneResources() fence-waits before returning, so the prior
/// submit has completed.
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

    ID3D12Resource* takeResource() noexcept
    {
        ID3D12Resource* resource = _entry.resource;
        _entry.resource = nullptr;
        return resource;
    }

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

/// Caches StagingEntry per (size, format, plane); pool owns one ref on each held COM object, acquire() returns the
/// COM objects AddRef'd.
class StagingResourcePool
{
public:
    static StagingResourcePool& instance()
    {
        static StagingResourcePool pool;
        return pool;
    }

    ~StagingResourcePool() { clear(); }

    /// Returns AddRef'd pooled COM objects for @p key (creating them on a miss). Returns an invalid ref on creation
    /// failure.
    StagingEntryRef acquire(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE queueType, const StagingKey& key,
                            const D3D12_RESOURCE_DESC& dstDesc, const D3D12_HEAP_PROPERTIES& heapProps, int planeIdx,
                            guint subIdx)
    {
        QMutexLocker lock(&_mutex);
        StagingEntry* cachedEntry = _entries.find(key);
        if (cachedEntry && cachedEntry->resource && cachedEntry->allocator && cachedEntry->list) {
            GstHwPathTelemetry::recordImageCacheHit(HwVideoBufferPath::D3D12);
            return addRefEntry(*cachedEntry);
        }

        StagingEntry e;
        if (FAILED(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &dstDesc, D3D12_RESOURCE_STATE_COMMON,
                                                nullptr, IID_PPV_ARGS(&e.resource)))) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                             "mapTextures: CreateCommittedResource for slice copy failed (plane="
                                 << planeIdx << " subresource=" << subIdx << ")");
            return {};
        }
        if (FAILED(dev->CreateCommandAllocator(queueType, IID_PPV_ARGS(&e.allocator))) ||
            FAILED(dev->CreateCommandList(0, queueType, e.allocator, nullptr, IID_PPV_ARGS(&e.list)))) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                             "mapTextures: command allocator/list create failed (plane=" << planeIdx << ")");
            if (e.list)
                e.list->Release();
            if (e.allocator)
                e.allocator->Release();
            e.resource->Release();
            return {};
        }
        // Freshly created lists are open; close so the per-frame Reset() path is uniform.
        e.list->Close();

        StagingEntryRef ref = addRefEntry(e);
        _entries.insert(key, e);
        GstHwPathTelemetry::recordImageCacheMiss(HwVideoBufferPath::D3D12);
        return ref;
    }

    void clear()
    {
        QMutexLocker lock(&_mutex);
        _entries.clear();
    }

private:
    static constexpr std::size_t kMaxEntries = 8;
    QMutex _mutex;
    GstHw::GstHwImportCache<StagingKey, StagingEntry, StagingKeyHash> _entries{
        kMaxEntries,
        [](const StagingKey&, StagingEntry& e) {
            releaseEntryResources(e);
            e = {};
        }};
};

/// Copy one subresource slice into a pooled COPY-queue (DIRECT fallback) resource and submit the copy WITHOUT waiting;
/// the caller does a single fence wait after all planes. On success returns the staging resource AddRef'd (caller owns
/// the ref) and advances @p maxFenceValue to the highest submitted fence value. nullptr on failure.
ID3D12Resource* copySliceToStaging(ID3D12Resource* resource, guint subIdx, int planeIdx,
                                   const D3D12_RESOURCE_DESC& srcDesc, ID3D12Device* d3dDev, GstD3D12CmdQueue* cmdQueue,
                                   D3D12_COMMAND_LIST_TYPE queueType, guint64& maxFenceValue,
                                   StagingEntryRef& retainedEntry)
{
    D3D12_RESOURCE_DESC dstDesc = srcDesc;
    dstDesc.DepthOrArraySize = 1;
    dstDesc.MipLevels = 1;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    const StagingKey key{srcDesc.Width, srcDesc.Height, UINT(srcDesc.Format), planeIdx};
    StagingEntryRef entry =
        StagingResourcePool::instance().acquire(d3dDev, queueType, key, dstDesc, heapProps, planeIdx, subIdx);
    if (!entry.valid()) {
        return nullptr;
    }

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
    ID3D12Resource* stagingResource = entry.takeResource();
    retainedEntry = std::move(entry);
    return stagingResource;
}

}  // namespace

void GstD3D12VideoBuffer::resetCachedState() noexcept
{
    StagingResourcePool::instance().clear();
    s_diag.reset();
}

namespace {
struct D3D12CacheResetRegistrar
{
    D3D12CacheResetRegistrar() { GstContextBridgeRegistry::registerCacheReset(&GstD3D12VideoBuffer::resetCachedState); }
};

const D3D12CacheResetRegistrar s_d3d12CacheResetRegistrar;
}  // namespace

GstD3D12VideoBuffer::GstD3D12VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                         const QVideoFrameFormat& format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{
    // Resolve on the streaming thread, never in mapTextures (render thread); blocking here is harmless since the
    // decoder is async.
    resolvePlaneResources();
}

GstD3D12VideoBuffer::~GstD3D12VideoBuffer()
{
    for (int i = 0; i < _resolvedCount; ++i) {
        if (_resources[i])
            _resources[i]->Release();
    }
}

bool GstD3D12VideoBuffer::validatePlaneHandles() const
{
    // Constructor-time resolve failed (device mismatch, sync/copy failure) — let the factory demote to CPU.
    if (!_resolved) {
        return false;
    }
    return validatePlanes([](GstMemory* mem) {
        if (!mem || !gst_is_d3d12_memory(mem))
            return false;
        return gst_d3d12_memory_get_resource_handle(GST_D3D12_MEMORY_CAST(mem)) != nullptr;
    });
}

void GstD3D12VideoBuffer::resolvePlaneResources()
{
    GstBuffer* buffer = _sample ? gst_sample_get_buffer(_sample) : nullptr;
    if (!buffer)
        return;

    const int memCount = (std::min)(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    GstD3DVideoBufferCommon::PlaneResourceGuard<ID3D12Resource> guard;
    std::array<StagingEntryRef, kMaxPlanes> stagingLeases;

    // Shared copy state: the queue is resolved lazily on the first staged plane (constant per device) and all planes
    // submit to it, then a single fence wait covers every submit. The guard unrefs the held queue ref on any early
    // return path.
    struct CmdQueueRef
    {
        GstD3D12CmdQueue* q = nullptr;

        ~CmdQueueRef()
        {
            if (q)
                gst_object_unref(q);
        }
    } queueRef;

    D3D12_COMMAND_LIST_TYPE queueType = D3D12_COMMAND_LIST_TYPE_COPY;
    guint64 maxFenceValue = 0;
    bool anyCopy = false;
    struct SubmittedCopyWait
    {
        GstD3D12CmdQueue*& queue;
        guint64& maxFenceValue;
        bool& anyCopy;
        bool armed = true;

        ~SubmittedCopyWait()
        {
            if (armed)
                wait();
        }

        bool waitAndDisarm()
        {
            armed = false;
            return wait();
        }

        bool wait()
        {
            if (!anyCopy || !queue) {
                return true;
            }
            GstHwPathTelemetry::recordSyncWait(HwVideoBufferPath::D3D12, /*gpuSide=*/false);
            if (FAILED(gst_d3d12_cmd_queue_fence_wait(queue, maxFenceValue))) {
                QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                                 "resolve: gst_d3d12_cmd_queue_fence_wait failed");
                return false;
            }
            return true;
        }
    } submittedCopyWait{queueRef.q, maxFenceValue, anyCopy};

    for (int i = 0; i < memCount; ++i) {
        GstMemory* mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_d3d12_memory(mem)) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedNonD3DMemory,
                             "resolve: plane" << i << "memory is not GstD3D12Memory (allocator="
                                              << (mem && mem->allocator ? mem->allocator->mem_type : "null") << ")");
            return;
        }
        GstD3D12Memory* d3dmem = GST_D3D12_MEMORY_CAST(mem);
        // Device guard (plane 0): gst-d3d12 may land on an isolated device when NEED_CONTEXT was preempted, and
        // importing a foreign-device resource into QRhi corrupts.
        if (i == 0) {
            // currentDevice() is transfer-full — unref both branches to avoid UAF after reset().
            GstD3D12Device* bridgeDev = GstD3D12ContextBridge::currentDevice();
            if (bridgeDev && d3dmem->device != bridgeDev) {
                const gint64 bridgeLuid = GstD3DContextBridgeCommon::readAdapterLuid(bridgeDev);
                const gint64 bufLuid = GstD3DContextBridgeCommon::readAdapterLuid(d3dmem->device);
                gst_object_unref(bridgeDev);
                QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedDeviceMismatch,
                                 "resolve: GstD3D12Memory on foreign device (bridge LUID="
                                     << bridgeLuid << "buffer LUID=" << bufLuid
                                     << "); bridge missed NEED_CONTEXT race — rejecting frame");
                return;
            }
            if (bridgeDev)
                gst_object_unref(bridgeDev);
        }
        // Block on the decoder's fence before reading, else QRhi samples mid-write while the decoder is still writing
        // this resource.
        if (!gst_d3d12_memory_sync(d3dmem)) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedNullResource,
                             "resolve: gst_d3d12_memory_sync failed for plane" << i);
            return;
        }
        ID3D12Resource* resource = gst_d3d12_memory_get_resource_handle(d3dmem);
        if (!resource) {
            QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedNullResource,
                             "resolve: gst_d3d12_memory_get_resource_handle returned null for plane" << i);
            return;
        }
        // QRhi::createFrom has no subresource parameter — copy the slice (with its blocking fence wait) here, not in
        // mapTextures.
        guint subIdx = 0;
        gst_d3d12_memory_get_subresource_index(d3dmem, guint(i), &subIdx);
        D3D12_RESOURCE_DESC srcDesc = resource->GetDesc();
        if (subIdx > 0 || srcDesc.DepthOrArraySize > 1) {
            ID3D12Device* d3dDev = gst_d3d12_device_get_device_handle(d3dmem->device);
            if (!queueRef.q) {
                queueType = D3D12_COMMAND_LIST_TYPE_COPY;
                GstD3D12CmdQueue* q = gst_d3d12_device_get_cmd_queue(d3dmem->device, queueType);
                if (!q) {
                    queueType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    q = gst_d3d12_device_get_cmd_queue(d3dmem->device, queueType);
                }
                if (q) {
                    // Hold a ref for the whole loop so it serializes against gst-d3d12's queue lock like the
                    // decoder-thread submits do.
                    gst_object_ref(q);
                    queueRef.q = q;
                }
            }
            if (!d3dDev || !queueRef.q) {
                QGC_HW_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                                 "mapTextures: could not obtain D3D12 device/queue for slice copy (plane=" << i << ")");
                return;
            }
            ID3D12Resource* stagingResource = copySliceToStaging(resource, subIdx, i, srcDesc, d3dDev, queueRef.q,
                                                                 queueType, maxFenceValue, stagingLeases[i]);
            if (!stagingResource) {
                return;
            }
            anyCopy = true;
            guard.handles[i] = stagingResource;
        } else {
            resource->AddRef();
            guard.handles[i] = resource;
        }
        guard.owned = i + 1;
    }

    // One fence wait on the highest value covers every plane (same queue, in order) and guarantees the pooled
    // allocators are idle before the next frame resets them.
    if (!submittedCopyWait.waitAndDisarm()) {
        return;
    }

    _resources = guard.handles;
    _resolvedCount = memCount;
    _resolved = true;
    guard.commit();
}

QVideoFrameTexturesUPtr GstD3D12VideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    // GstD3D12ContextBridge wires a shared adapter; without it createFrom() succeeds but resources sit on an isolated
    // device and rendering corrupts.
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::D3D12), GstD3D12Log(), s_diag, buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
    }
    if (!_resolved) {
        // Failure cause was warned in resolvePlaneResources() on the streaming thread.
        return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
    }

    return GstD3DVideoBufferCommon::mapResolvedTextures(
        *this, rhi, old, HwVideoBufferPath::D3D12, _resources, _resolvedCount, _format.frameSize(),
        _format.pixelFormat(), s_diag.loggedFirstSuccess, s_diag.loggedTextureCreateFail, GstD3D12Log, "D3D12");
}

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D12_GPU_PATH
