#include "GstD3D12StagingPool.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <utility>

#include <QtCore/QMutex>

#include "GstD3DVideoBufferCommon.h"
#include "GstHwImportCache.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"

namespace {

using GstD3DVideoBufferCommon::StagingKey;

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

    explicit StagingEntryRef(const StagingEntry& e)
        : _entry(e)
    {}

    ~StagingEntryRef() { releaseEntryResources(_entry); }

    StagingEntryRef(const StagingEntryRef&) = delete;
    StagingEntryRef& operator=(const StagingEntryRef&) = delete;

    StagingEntryRef(StagingEntryRef&& other) noexcept
        : _entry(other._entry)
    {
        other._entry = {};
    }

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

}  // namespace

namespace GstD3D12StagingPool {

/// Holds one ring slot, plus refs on its COM objects, from acquire() until the last holder (the D3D12 video buffer and
/// the frame bundle QRhi samples) drops it; another stream or a later frame can't reset its allocator/list or overwrite
/// its resource meanwhile. Its copy has completed by the time the first holder exists (the resolve fence-waits), so the
/// next acquirer may Reset() the allocator immediately.
class SlotLease
{
public:
    SlotLease(std::shared_ptr<StagingLeaseFlags> flags, std::size_t slot, StagingEntryRef entry)
        : _flags(std::move(flags))
        , _slot(slot)
        , _entry(std::move(entry))
    {}

    ~SlotLease() { (*_flags)[_slot].store(false, std::memory_order_release); }

    SlotLease(const SlotLease&) = delete;
    SlotLease& operator=(const SlotLease&) = delete;

    const StagingEntryRef& entry() const noexcept { return _entry; }

private:
    std::shared_ptr<StagingLeaseFlags> _flags;
    std::size_t _slot;
    StagingEntryRef _entry;
};

}  // namespace GstD3D12StagingPool

namespace {

using GstD3D12StagingPool::SlotLease;

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
    std::shared_ptr<SlotLease> acquire(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE queueType, const StagingKey& key,
                                       const D3D12_RESOURCE_DESC& dstDesc, const D3D12_HEAP_PROPERTIES& heapProps,
                                       int planeIdx, guint subIdx, GstHw::MapDiagnostics& diag)
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
            } else if (createEntry(dev, queueType, dstDesc, heapProps, planeIdx, subIdx, diag, e)) {
                GstHwPathTelemetry::record(HwVideoBufferPath::D3D12, GstHwPathTelemetry::Counter::ImageCacheMisses);
            } else {
                (*ring->leased)[slot].store(false, std::memory_order_release);
                return nullptr;
            }
            ring->next = (slot + 1) % kStagingRingSize;
            return std::make_shared<SlotLease>(ring->leased, slot, addRefEntry(e));
        }
        QGC_HW_WARN_ONCE(GstD3D12Log, diag.loggedTextureCreateFail,
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
                            const D3D12_HEAP_PROPERTIES& heapProps, int planeIdx, guint subIdx,
                            GstHw::MapDiagnostics& diag, StagingEntry& e)
    {
        if (FAILED(dev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &dstDesc, D3D12_RESOURCE_STATE_COMMON,
                                                nullptr, IID_PPV_ARGS(&e.resource)))) {
            QGC_HW_WARN_ONCE(GstD3D12Log, diag.loggedTextureCreateFail,
                             "mapTextures: CreateCommittedResource for slice copy failed (plane="
                                 << planeIdx << " subresource=" << subIdx << ")");
            e = {};
            return false;
        }
        if (FAILED(dev->CreateCommandAllocator(queueType, IID_PPV_ARGS(&e.allocator))) ||
            FAILED(dev->CreateCommandList(0, queueType, e.allocator, nullptr, IID_PPV_ARGS(&e.list)))) {
            QGC_HW_WARN_ONCE(GstD3D12Log, diag.loggedTextureCreateFail,
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

}  // namespace

namespace GstD3D12StagingPool {

ID3D12Resource* copySlice(ID3D12Resource* resource, guint subIdx, int planeIdx, const D3D12_RESOURCE_DESC& srcDesc,
                          ID3D12Device* device, GstD3D12CmdQueue* queue, D3D12_COMMAND_LIST_TYPE queueType,
                          guint64& maxFenceValue, std::shared_ptr<SlotLease>& lease, GstHw::MapDiagnostics& diag)
{
    D3D12_RESOURCE_DESC dstDesc = srcDesc;
    dstDesc.DepthOrArraySize = 1;
    dstDesc.MipLevels = 1;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    const StagingKey key{srcDesc.Width, srcDesc.Height, UINT(srcDesc.Format), planeIdx};
    std::shared_ptr<SlotLease> slotLease =
        StagingResourcePool::instance().acquire(device, queueType, key, dstDesc, heapProps, planeIdx, subIdx, diag);
    if (!slotLease || !slotLease->entry().valid()) {
        return nullptr;
    }
    const StagingEntryRef& entry = slotLease->entry();

    if (FAILED(entry.allocator()->Reset()) || FAILED(entry.list()->Reset(entry.allocator(), nullptr))) {
        QGC_HW_WARN_ONCE(GstD3D12Log, diag.loggedTextureCreateFail,
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
    if (FAILED(gst_d3d12_cmd_queue_execute_command_lists(queue, 1, lists, &fenceValue))) {
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

void clear()
{
    StagingResourcePool::instance().clear();
}

}  // namespace GstD3D12StagingPool
