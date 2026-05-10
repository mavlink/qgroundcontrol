#include "GstD3D12VideoBuffer.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include "GstD3D12ContextBridge.h"
#include "GstD3DContextBridgeCommon.h"
#include "GstD3DVideoBufferCommon.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QVarLengthArray>

#include <gst/d3d12/gstd3d12.h>

#include <d3d12.h>

QGC_LOGGING_CATEGORY(GstD3D12Log, "Video.GStreamer.HwBuffers.GstD3D12Buf")

namespace {

using GstD3DVideoBufferCommon::kMaxPlanes;
using GstD3DVideoBufferCommon::MapDiagnostics;
using GstD3DVideoBufferCommon::fail;
using D3D12FrameTextures = GstD3DVideoBufferCommon::FrameTextures<ID3D12Resource>;

MapDiagnostics s_diag;

/// Copies one subresource slice into a fresh resource via the dedicated COPY queue
/// (DIRECT fallback) so the transfer doesn't stall Qt's DIRECT queue. Caller owns the ref.
ID3D12Resource *copySliceToStaging(ID3D12Resource *resource, guint subIdx, int planeIdx,
                                   const D3D12_RESOURCE_DESC &srcDesc,
                                   GstD3D12Memory *d3dmem)
{
    ID3D12Device *d3dDev = gst_d3d12_device_get_device_handle(d3dmem->device);
    D3D12_COMMAND_LIST_TYPE queueType = D3D12_COMMAND_LIST_TYPE_COPY;
    GstD3D12CmdQueue *cmdQueue = gst_d3d12_device_get_cmd_queue(d3dmem->device, queueType);
    if (!cmdQueue) {
        queueType = D3D12_COMMAND_LIST_TYPE_DIRECT;
        cmdQueue = gst_d3d12_device_get_cmd_queue(d3dmem->device, queueType);
    }
    ID3D12CommandQueue *rawQueue = cmdQueue ? gst_d3d12_cmd_queue_get_handle(cmdQueue) : nullptr;
    if (!d3dDev || !rawQueue) {
        QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                          "mapTextures: could not obtain D3D12 device/queue for slice copy (plane="
                          << planeIdx << ")");
        return nullptr;
    }

    D3D12_RESOURCE_DESC dstDesc = srcDesc;
    dstDesc.DepthOrArraySize = 1;
    dstDesc.MipLevels = 1;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    ID3D12Resource *stagingResource = nullptr;
    if (FAILED(d3dDev->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &dstDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&stagingResource)))) {
        QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                          "mapTextures: CreateCommittedResource for slice copy failed (plane=" << planeIdx
                          << " subresource=" << subIdx << ")");
        return nullptr;
    }

    // COPY queue accepts COMMON↔COPY_SOURCE↔COPY_DEST barriers — transitions below stay legal.
    ID3D12CommandAllocator *cmdAlloc = nullptr;
    ID3D12GraphicsCommandList *cmdList = nullptr;
    bool copyOk = false;
    if (SUCCEEDED(d3dDev->CreateCommandAllocator(queueType, IID_PPV_ARGS(&cmdAlloc))) &&
        SUCCEEDED(d3dDev->CreateCommandList(0, queueType,
                                             cmdAlloc, nullptr, IID_PPV_ARGS(&cmdList)))) {
        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = resource;
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = subIdx;

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = stagingResource;
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;

        // Decoder leaves the resource in a non-COPY_SOURCE state (typically COMMON
        // after gst_d3d12_memory_sync, or VIDEO_DECODE_WRITE pre-sync). Issue an
        // explicit transition; without it the debug layer fires and some drivers TDR.
        D3D12_RESOURCE_BARRIER toCopySrc{};
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

        ID3D12CommandList *lists[] = { cmdList };
        rawQueue->ExecuteCommandLists(1, lists);

        ID3D12Fence *fence = nullptr;
        if (SUCCEEDED(d3dDev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
            rawQueue->Signal(fence, 1);
            HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (event) {
                fence->SetEventOnCompletion(1, event);
                WaitForSingleObject(event, INFINITE);
                CloseHandle(event);
            }
            fence->Release();
        }
        copyOk = true;
    }
    if (cmdList) cmdList->Release();
    if (cmdAlloc) cmdAlloc->Release();

    if (!copyOk) {
        stagingResource->Release();
        return nullptr;
    }
    return stagingResource;
}

} // namespace

GstD3D12VideoBuffer::GstD3D12VideoBuffer(GstSample *sample,
                                         const GstVideoInfo &videoInfo,
                                         const QVideoFrameFormat &format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{
}

GstD3D12VideoBuffer::~GstD3D12VideoBuffer() = default;

QAbstractVideoBuffer::MapData GstD3D12VideoBuffer::map(QVideoFrame::MapMode /*mode*/)
{
    return {};
}

bool GstD3D12VideoBuffer::validatePlaneHandles() const
{
    if (!_sample) return false;
    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) return false;
    const int memCount = qMin(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    if (memCount <= 0) return false;
    for (int i = 0; i < memCount; ++i) {
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_d3d12_memory(mem)) return false;
        if (!gst_d3d12_memory_get_resource_handle(GST_D3D12_MEMORY_CAST(mem))) {
            return false;
        }
    }
    return true;
}

QVideoFrameTexturesUPtr GstD3D12VideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr & /*old*/)
{
    Q_ASSERT(rhi.thread()->isCurrentThread()); // Qt's contract: mapTextures runs on the QRhi (render) thread.
    // Shared-device wiring is provided by GstD3D12ContextBridge — when primed, gst-d3d12
    // decoders allocate resources on QRhi's adapter, so the handles below are
    // directly QRhi-importable. Without the bridge, resources are on an isolated device
    // and createFrom() will succeed but rendering will produce garbage / crashes.
    if (!_sample) {
        QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedNullSample, "mapTextures: GstSample is null");
        return fail(s_diag);
    }
    if (rhi.backend() != QRhi::D3D12) {
        QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedBadBackend,
                          "mapTextures: QRhi backend is" << rhi.backendName() << "(D3D12 required)");
        return fail(s_diag);
    }

    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) {
        QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedNullBuffer, "mapTextures: GstSample has no buffer");
        return fail(s_diag);
    }

    const int memCount = qMin(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    std::array<ID3D12Resource *, kMaxPlanes> resources{};
    QVarLengthArray<ID3D12Resource *, kMaxPlanes> refdResources;
    for (int i = 0; i < memCount; ++i) {
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_d3d12_memory(mem)) {
            QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedNonD3DMemory,
                              "mapTextures: plane" << i << "memory is not GstD3D12Memory (allocator="
                                                   << (mem && mem->allocator ? mem->allocator->mem_type : "null")
                                                   << ")");
            for (auto *r : refdResources) r->Release();
            return fail(s_diag);
        }
        GstD3D12Memory *d3dmem = GST_D3D12_MEMORY_CAST(mem);
        // Per-buffer device guard: gst-d3d12 may run on an isolated device when our
        // NEED_CONTEXT response was preempted. Importing a foreign-device ID3D12Resource
        // into QRhi either fails or silently corrupts. Check once on the first plane.
        if (i == 0) {
            // currentDevice() is transfer-full — unref both branches to avoid UAF after reset().
            GstD3D12Device *bridgeDev = GstD3D12ContextBridge::currentDevice();
            if (bridgeDev && d3dmem->device != bridgeDev) {
                const gint64 bridgeLuid = GstD3DContextBridgeCommon::readAdapterLuid(bridgeDev);
                const gint64 bufLuid = GstD3DContextBridgeCommon::readAdapterLuid(d3dmem->device);
                gst_object_unref(bridgeDev);
                QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedDeviceMismatch,
                                  "mapTextures: GstD3D12Memory on foreign device (bridge LUID="
                                  << bridgeLuid << "buffer LUID=" << bufLuid
                                  << "); bridge missed NEED_CONTEXT race — rejecting frame");
                return fail(s_diag);
            }
            if (bridgeDev) gst_object_unref(bridgeDev);
        }
        // Block on the decoder's fence before reading; otherwise QRhi may sample mid-write
        // (decoder still has a CL writing this resource). gst-d3d12 stores the fence on the
        // memory when the producing element calls set_fence; sync() flushes it.
        if (!gst_d3d12_memory_sync(d3dmem)) {
            QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedNullResource,
                              "mapTextures: gst_d3d12_memory_sync failed for plane" << i);
            for (auto *r : refdResources) r->Release();
            return fail(s_diag);
        }
        ID3D12Resource *resource = gst_d3d12_memory_get_resource_handle(d3dmem);
        if (!resource) {
            QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedNullResource,
                              "mapTextures: gst_d3d12_memory_get_resource_handle returned null for plane" << i);
            for (auto *r : refdResources) r->Release();
            return fail(s_diag);
        }
        // QRhi::createFrom has no subresource parameter — copy the slice when needed.
        guint subIdx = 0;
        gst_d3d12_memory_get_subresource_index(d3dmem, guint(i), &subIdx);
        D3D12_RESOURCE_DESC srcDesc = resource->GetDesc();
        if (subIdx > 0 || srcDesc.DepthOrArraySize > 1) {
            ID3D12Resource *stagingResource = copySliceToStaging(resource, subIdx, i, srcDesc, d3dmem);
            if (!stagingResource) {
                for (auto *r : refdResources) r->Release();
                return fail(s_diag);
            }
            refdResources.append(stagingResource);
            resources[i] = stagingResource;
        } else {
            resource->AddRef();
            refdResources.append(resource);
            resources[i] = resource;
        }
    }

    auto textures = std::make_unique<D3D12FrameTextures>(&rhi, _format.frameSize(),
                                                         _format.pixelFormat(), resources, memCount);
    // Per-plane: NV12 chroma can fail while luma succeeds. Returning a partial bundle
    // would render with missing planes and no failure-counter increment.
    for (int i = 0; i < memCount; ++i) {
        if (!textures->texture(static_cast<uint>(i))) {
            QGC_D3D_WARN_ONCE(GstD3D12Log, s_diag.loggedTextureCreateFail,
                              "mapTextures: QRhiTexture::createFrom failed plane=" << i
                              << " (size=" << _format.frameSize()
                              << " format=" << int(_format.pixelFormat()) << " planes=" << memCount << ")");
            return fail(s_diag);
        }
    }

    if (!s_diag.loggedFirstSuccess.exchange(true, std::memory_order_relaxed)) {
        qCInfo(GstD3D12Log) << "First D3D12 zero-copy mapTextures success: size=" << _format.frameSize()
                            << "format=" << int(_format.pixelFormat()) << "planes=" << memCount;
    }
    return textures;
}

quint64 GstD3D12VideoBuffer::takeMapFailureCount()
{
    return GstD3DVideoBufferCommon::takeMapFailureCount(s_diag);
}

quint64 GstD3D12VideoBuffer::peekMapFailureCount()
{
    return GstD3DVideoBufferCommon::peekMapFailureCount(s_diag);
}

#endif // Q_OS_WIN && QGC_HAS_GST_D3D12_GPU_PATH
