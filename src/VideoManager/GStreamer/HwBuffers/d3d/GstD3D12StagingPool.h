#pragma once

#include <d3d12.h>
#include <gst/d3d12/gstd3d12.h>
#include <memory>

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GstD3D12Log)

namespace GstHw {
struct MapDiagnostics;
}

/// Staging copies of the D3D12 subresource slices QRhi can't sample in place: a ring of staging slots per (size,
/// format, plane), each with its own allocator/list so concurrent streams never record into the same command objects.
namespace GstD3D12StagingPool {

/// Holds one ring slot from copySlice() until the last holder drops it.
class SlotLease;

/// Copy one subresource slice into a leased COPY-queue (DIRECT fallback) staging slot and submit the copy WITHOUT
/// waiting; the caller does a single fence wait after all planes. On success returns the staging resource AddRef'd
/// (caller owns the ref), hands the slot lease to @p lease and advances @p maxFenceValue to the highest submitted fence
/// value. nullptr on failure, which @p diag logs once.
ID3D12Resource* copySlice(ID3D12Resource* resource, guint subIdx, int planeIdx, const D3D12_RESOURCE_DESC& srcDesc,
                          ID3D12Device* device, GstD3D12CmdQueue* queue, D3D12_COMMAND_LIST_TYPE queueType,
                          guint64& maxFenceValue, std::shared_ptr<SlotLease>& lease, GstHw::MapDiagnostics& diag);

/// Drops the pool's refs on every slot; leased slots live on until their leases go.
void clear();

}  // namespace GstD3D12StagingPool
