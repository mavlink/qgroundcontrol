#include "GstD3D11VideoBuffer.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include "GstD3D11ContextBridge.h"
#include "GstD3DContextBridgeCommon.h"
#include "GstD3DVideoBufferCommon.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QVarLengthArray>

#include <gst/d3d11/gstd3d11.h>

#include <d3d11.h>

QGC_LOGGING_CATEGORY(GstD3D11Log, "Video.GStreamer.HwBuffers.GstD3D11Buf")

namespace {

using GstD3DVideoBufferCommon::kMaxPlanes;
using GstD3DVideoBufferCommon::MapDiagnostics;
using GstD3DVideoBufferCommon::fail;
using D3D11FrameTextures = GstD3DVideoBufferCommon::FrameTextures<ID3D11Texture2D>;

MapDiagnostics s_diag;

/// Copies a single subresource slice out of a multi-slice ID3D11Texture2D into
/// a fresh ID3D11Texture2D so QRhi (which has no subresource selector) can
/// sample it. Returns the staging texture (caller owns the ref) or nullptr.
ID3D11Texture2D *copySliceToStaging(ID3D11Texture2D *tex, guint subIdx, int planeIdx,
                                    const D3D11_TEXTURE2D_DESC &srcDesc,
                                    GstD3D11Memory *d3dmem)
{
    ID3D11Device *d3dDev = gst_d3d11_device_get_device_handle(d3dmem->device);
    ID3D11DeviceContext *d3dCtx = gst_d3d11_device_get_device_context_handle(d3dmem->device);
    D3D11_TEXTURE2D_DESC dstDesc = srcDesc;
    dstDesc.ArraySize = 1;
    dstDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    dstDesc.MiscFlags = 0;
    dstDesc.MipLevels = 1;
    ID3D11Texture2D *stagingTex = nullptr;
    if (FAILED(d3dDev->CreateTexture2D(&dstDesc, nullptr, &stagingTex))) {
        QGC_D3D_WARN_ONCE(GstD3D11Log, s_diag.loggedTextureCreateFail,
                          "mapTextures: CreateTexture2D for slice copy failed (plane=" << planeIdx
                          << "subresource=" << subIdx << ")");
        return nullptr;
    }
    d3dCtx->CopySubresourceRegion(stagingTex, 0, 0, 0, 0, tex, subIdx, nullptr);
    // Flush so QRhi sees the staged copy in the immediate context queue before binding it.
    d3dCtx->Flush();
    return stagingTex;
}

} // namespace

GstD3D11VideoBuffer::GstD3D11VideoBuffer(GstSample *sample,
                                         const GstVideoInfo &videoInfo,
                                         const QVideoFrameFormat &format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{
}

GstD3D11VideoBuffer::~GstD3D11VideoBuffer() = default;

QAbstractVideoBuffer::MapData GstD3D11VideoBuffer::map(QVideoFrame::MapMode /*mode*/)
{
    return {};
}

bool GstD3D11VideoBuffer::validatePlaneHandles() const
{
    if (!_sample) return false;
    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) return false;
    const int memCount = qMin(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    if (memCount <= 0) return false;
    for (int i = 0; i < memCount; ++i) {
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_d3d11_memory(mem)) return false;
        // Cheap field read; confirms the wrapper actually backs an ID3D11Texture2D.
        if (!gst_d3d11_memory_get_resource_handle(GST_D3D11_MEMORY_CAST(mem))) {
            return false;
        }
    }
    return true;
}

QVideoFrameTexturesUPtr GstD3D11VideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr & /*old*/)
{
    Q_ASSERT(rhi.thread()->isCurrentThread()); // Qt's contract: mapTextures runs on the QRhi (render) thread.
    // *** UNTESTED on Windows hardware. CI compiles this path; runtime validation TBD. ***
    // Shared-device wiring is provided by GstD3D11ContextBridge — when primed, gst-d3d11
    // decoders allocate textures on QRhi's ID3D11Device, so the handles below are
    // directly QRhi-sampleable. Without the bridge, textures are on an isolated device
    // and createFrom() will succeed but rendering will produce garbage / crashes.
    if (!_sample) {
        QGC_D3D_WARN_ONCE(GstD3D11Log, s_diag.loggedNullSample, "mapTextures: GstSample is null");
        return fail(s_diag);
    }
    if (rhi.backend() != QRhi::D3D11) {
        QGC_D3D_WARN_ONCE(GstD3D11Log, s_diag.loggedBadBackend,
                          "mapTextures: QRhi backend is" << rhi.backendName() << "(D3D11 required)");
        return fail(s_diag);
    }

    GstBuffer *buffer = gst_sample_get_buffer(_sample);
    if (!buffer) {
        QGC_D3D_WARN_ONCE(GstD3D11Log, s_diag.loggedNullBuffer, "mapTextures: GstSample has no buffer");
        return fail(s_diag);
    }

    const int memCount = qMin(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    std::array<ID3D11Texture2D *, kMaxPlanes> texs{};
    QVarLengthArray<ID3D11Texture2D *, kMaxPlanes> refdTexs;
    for (int i = 0; i < memCount; ++i) {
        GstMemory *mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_d3d11_memory(mem)) {
            QGC_D3D_WARN_ONCE(GstD3D11Log, s_diag.loggedNonD3DMemory,
                              "mapTextures: plane" << i << "memory is not GstD3D11Memory (allocator="
                                                   << (mem && mem->allocator ? mem->allocator->mem_type : "null")
                                                   << ")");
            for (auto *t : refdTexs) t->Release();
            return fail(s_diag);
        }
        // Per-buffer device guard: gst-d3d11 elements may run on an isolated device when our
        // NEED_CONTEXT response was preempted by another bridge. Sampling a foreign-device
        // ID3D11Texture2D from QRhi corrupts silently (texture handle is valid on the wrong
        // device). Check once per first plane; same buffer ⇒ same device for all planes.
        if (i == 0) {
            // currentDevice() is transfer-full — unref both branches to avoid UAF after reset().
            GstD3D11Device *bridgeDev = GstD3D11ContextBridge::currentDevice();
            GstD3D11Device *bufDev = GST_D3D11_MEMORY_CAST(mem)->device;
            if (bridgeDev && bufDev != bridgeDev) {
                const gint64 bridgeLuid = GstD3DContextBridgeCommon::readAdapterLuid(bridgeDev);
                const gint64 bufLuid = GstD3DContextBridgeCommon::readAdapterLuid(bufDev);
                gst_object_unref(bridgeDev);
                QGC_D3D_WARN_ONCE(GstD3D11Log, s_diag.loggedDeviceMismatch,
                                  "mapTextures: GstD3D11Memory on foreign device (bridge LUID="
                                  << bridgeLuid << "buffer LUID=" << bufLuid
                                  << "); bridge missed NEED_CONTEXT race — rejecting frame");
                return fail(s_diag);
            }
            if (bridgeDev) gst_object_unref(bridgeDev);
        }
        ID3D11Texture2D *tex = reinterpret_cast<ID3D11Texture2D *>(
            gst_d3d11_memory_get_resource_handle(GST_D3D11_MEMORY_CAST(mem)));
        if (!tex) {
            QGC_D3D_WARN_ONCE(GstD3D11Log, s_diag.loggedNullResource,
                              "mapTextures: gst_d3d11_memory_get_resource_handle returned null for plane" << i);
            for (auto *t : refdTexs) t->Release();
            return fail(s_diag);
        }
        // QRhi::createFrom has no subresource parameter — copy the slice when needed.
        const guint subIdx = gst_d3d11_memory_get_subresource_index(GST_D3D11_MEMORY_CAST(mem));
        D3D11_TEXTURE2D_DESC srcDesc{};
        tex->GetDesc(&srcDesc);
        if (subIdx > 0 || srcDesc.ArraySize > 1) {
            ID3D11Texture2D *stagingTex = copySliceToStaging(tex, subIdx, i, srcDesc,
                                                             GST_D3D11_MEMORY_CAST(mem));
            if (!stagingTex) {
                for (auto *t : refdTexs) t->Release();
                return fail(s_diag);
            }
            refdTexs.append(stagingTex);
            texs[i] = stagingTex;
        } else {
            tex->AddRef();
            refdTexs.append(tex);
            texs[i] = tex;
        }
    }

    auto textures = std::make_unique<D3D11FrameTextures>(&rhi, _format.frameSize(),
                                                         _format.pixelFormat(), texs, memCount);
    // Per-plane: NV12 chroma can fail while luma succeeds. Returning a partial bundle
    // would render with missing planes and no failure-counter increment.
    for (int i = 0; i < memCount; ++i) {
        if (!textures->texture(static_cast<uint>(i))) {
            QGC_D3D_WARN_ONCE(GstD3D11Log, s_diag.loggedTextureCreateFail,
                              "mapTextures: QRhiTexture::createFrom failed plane=" << i
                              << " (size=" << _format.frameSize()
                              << "format=" << int(_format.pixelFormat()) << "planes=" << memCount << ")");
            return fail(s_diag);
        }
    }

    if (!s_diag.loggedFirstSuccess.exchange(true, std::memory_order_relaxed)) {
        qCInfo(GstD3D11Log) << "First D3D11 zero-copy mapTextures success: size=" << _format.frameSize()
                            << "format=" << int(_format.pixelFormat()) << "planes=" << memCount;
    }
    return textures;
}

quint64 GstD3D11VideoBuffer::takeMapFailureCount()
{
    return GstD3DVideoBufferCommon::takeMapFailureCount(s_diag);
}

quint64 GstD3D11VideoBuffer::peekMapFailureCount()
{
    return GstD3DVideoBufferCommon::peekMapFailureCount(s_diag);
}

#endif // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
