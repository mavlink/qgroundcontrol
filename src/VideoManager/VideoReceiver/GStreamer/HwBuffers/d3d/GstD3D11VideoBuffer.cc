#include "GstD3D11VideoBuffer.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <QtCore/QMutex>
#include <algorithm>
#include <array>
#include <cstddef>
#include <d3d11.h>
#include <gst/d3d11/gstd3d11.h>

#include "GstContextBridgeRegistry.h"
#include "GstD3D11ContextBridge.h"
#include "GstD3DContextBridgeCommon.h"
#include "GstD3DVideoBufferCommon.h"
#include "GstHwImportCache.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstD3D11Log, "Video.GStreamer.HwBuffers.GstD3D11Buf")

namespace {

using GstD3DVideoBufferCommon::kMaxPlanes;
using GstD3DVideoBufferCommon::MapDiagnostics;
using D3D11FrameTextures = GstD3DVideoBufferCommon::FrameTextures<ID3D11Texture2D>;
using GstD3DVideoBufferCommon::StagingKey;
using GstD3DVideoBufferCommon::StagingKeyHash;

MapDiagnostics s_diag;

/// Caches a small ring of staging ID3D11Texture2D per (size, format, plane); acquire() rotates through the ring so a
/// new frame's CopySubresourceRegion never lands on the texture a reused QRhi view may still be sampling (D3D11 has no
/// fence against QRhi's frame boundary). Pool keeps one ref per texture; acquire() returns an extra AddRef'd ref.
/// Bounded to cap memory under resolution churn.
class StagingTexturePool
{
public:
    static StagingTexturePool& instance()
    {
        static StagingTexturePool pool;
        return pool;
    }

    ~StagingTexturePool() { clear(); }

    /// Returns an AddRef'd staging texture for @p key (caller owns the ref), round-robin over the ring, creating slots
    /// lazily. nullptr on CreateTexture2D failure.
    ID3D11Texture2D* acquire(ID3D11Device* dev, const StagingKey& key, const D3D11_TEXTURE2D_DESC& dstDesc,
                             int planeIdx, guint subIdx)
    {
        QMutexLocker lock(&_mutex);
        StagingRing* ring = _entries.find(key);
        if (!ring) {
            _entries.insert(key, StagingRing{});
            ring = _entries.find(key);
        }
        const std::size_t slot = ring->next;
        ring->next = (ring->next + 1) % kRingSize;
        if (ring->textures[slot]) {
            GstHwPathTelemetry::recordImageCacheHit(HwVideoBufferPath::D3D11);
            ring->textures[slot]->AddRef();
            return ring->textures[slot];
        }
        ID3D11Texture2D* tex = nullptr;
        if (FAILED(dev->CreateTexture2D(&dstDesc, nullptr, &tex))) {
            QGC_HW_WARN_ONCE(GstD3D11Log, s_diag.loggedTextureCreateFail,
                             "mapTextures: CreateTexture2D for slice copy failed (plane=" << planeIdx << "subresource="
                                                                                          << subIdx << ")");
            return nullptr;
        }
        tex->AddRef();
        ring->textures[slot] = tex;  // pool keeps the create ref
        GstHwPathTelemetry::recordImageCacheMiss(HwVideoBufferPath::D3D11);
        return tex;
    }

    void clear()
    {
        QMutexLocker lock(&_mutex);
        _entries.clear();
    }

private:
    // Ring depth 3: the streaming thread can copy a new frame while QRhi still samples the previous one.
    static constexpr std::size_t kRingSize = 3;

    struct StagingRing
    {
        std::array<ID3D11Texture2D*, kRingSize> textures{};
        std::size_t next = 0;
    };

    static constexpr std::size_t kMaxEntries = 8;
    QMutex _mutex;
    GstHw::GstHwImportCache<StagingKey, StagingRing, StagingKeyHash> _entries{
        kMaxEntries,
        [](const StagingKey&, StagingRing& ring) {
            for (ID3D11Texture2D*& tex : ring.textures) {
                if (tex) {
                    tex->Release();
                    tex = nullptr;
                }
            }
        }};
};

/// Copy one subresource slice into a pooled ID3D11Texture2D for QRhi (which has no subresource selector); returns the
/// staging texture (caller owns the ref) or nullptr. Does NOT flush — the caller flushes once after all planes.
ID3D11Texture2D* copySliceToStaging(ID3D11Texture2D* tex, guint subIdx, int planeIdx,
                                    const D3D11_TEXTURE2D_DESC& srcDesc, GstD3D11Memory* d3dmem)
{
    ID3D11Device* d3dDev = gst_d3d11_device_get_device_handle(d3dmem->device);
    ID3D11DeviceContext* d3dCtx = gst_d3d11_device_get_device_context_handle(d3dmem->device);
    D3D11_TEXTURE2D_DESC dstDesc = srcDesc;
    dstDesc.ArraySize = 1;
    dstDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    dstDesc.MiscFlags = 0;
    dstDesc.MipLevels = 1;
    const StagingKey key{srcDesc.Width, srcDesc.Height, UINT(srcDesc.Format), planeIdx};
    ID3D11Texture2D* stagingTex = StagingTexturePool::instance().acquire(d3dDev, key, dstDesc, planeIdx, subIdx);
    if (!stagingTex) {
        return nullptr;
    }
    // The immediate ID3D11DeviceContext is not free-threaded; gst-d3d11 device-lock contract requires this guard.
    gst_d3d11_device_lock(d3dmem->device);
    d3dCtx->CopySubresourceRegion(stagingTex, 0, 0, 0, 0, tex, subIdx, nullptr);
    gst_d3d11_device_unlock(d3dmem->device);
    return stagingTex;
}

}  // namespace

void GstD3D11VideoBuffer::resetCachedState() noexcept
{
    StagingTexturePool::instance().clear();
    s_diag.reset();
}

namespace {
struct D3D11CacheResetRegistrar
{
    D3D11CacheResetRegistrar() { GstContextBridgeRegistry::registerCacheReset(&GstD3D11VideoBuffer::resetCachedState); }
};

const D3D11CacheResetRegistrar s_d3d11CacheResetRegistrar;
}  // namespace

GstD3D11VideoBuffer::GstD3D11VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo,
                                         const QVideoFrameFormat& format)
    : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
{
    // Device guard + slice copy + flush on the streaming thread; mapTextures (render thread) only imports resolved
    // textures.
    resolvePlaneResources();
}

GstD3D11VideoBuffer::~GstD3D11VideoBuffer()
{
    for (int i = 0; i < _resolvedCount; ++i) {
        if (_textures[i])
            _textures[i]->Release();
    }
}

bool GstD3D11VideoBuffer::validatePlaneHandles() const
{
    // Constructor-time resolve failed (device mismatch, null resource, staging copy) — let the factory demote to CPU.
    if (!_resolved) {
        return false;
    }
    return validatePlanes([](GstMemory* mem) {
        if (!mem || !gst_is_d3d11_memory(mem))
            return false;
        // Cheap field read; confirms the wrapper actually backs an ID3D11Texture2D.
        return gst_d3d11_memory_get_resource_handle(GST_D3D11_MEMORY_CAST(mem)) != nullptr;
    });
}

void GstD3D11VideoBuffer::resolvePlaneResources()
{
    GstBuffer* buffer = _sample ? gst_sample_get_buffer(_sample) : nullptr;
    if (!buffer)
        return;

    const int memCount = (std::min)(int(gst_buffer_n_memory(buffer)), kMaxPlanes);
    GstD3DVideoBufferCommon::PlaneResourceGuard<ID3D11Texture2D> guard;
    GstD3D11Device* copyDevice = nullptr;

    for (int i = 0; i < memCount; ++i) {
        GstMemory* mem = gst_buffer_peek_memory(buffer, i);
        if (!mem || !gst_is_d3d11_memory(mem)) {
            QGC_HW_WARN_ONCE(GstD3D11Log, s_diag.loggedNonD3DMemory,
                             "resolve: plane" << i << "memory is not GstD3D11Memory (allocator="
                                              << (mem && mem->allocator ? mem->allocator->mem_type : "null") << ")");
            return;
        }
        // Device guard on plane 0: gst-d3d11 may land on an isolated device when NEED_CONTEXT was preempted; sampling a
        // foreign-device texture from QRhi corrupts silently.
        if (i == 0) {
            // currentDevice() is transfer-full — unref both branches to avoid UAF after reset().
            GstD3D11Device* bridgeDev = GstD3D11ContextBridge::currentDevice();
            GstD3D11Device* bufDev = GST_D3D11_MEMORY_CAST(mem)->device;
            if (bridgeDev && bufDev != bridgeDev) {
                const gint64 bridgeLuid = GstD3DContextBridgeCommon::readAdapterLuid(bridgeDev);
                const gint64 bufLuid = GstD3DContextBridgeCommon::readAdapterLuid(bufDev);
                gst_object_unref(bridgeDev);
                QGC_HW_WARN_ONCE(GstD3D11Log, s_diag.loggedDeviceMismatch,
                                 "resolve: GstD3D11Memory on foreign device (bridge LUID="
                                     << bridgeLuid << "buffer LUID=" << bufLuid
                                     << "); bridge missed NEED_CONTEXT race — rejecting frame");
                return;
            }
            if (bridgeDev)
                gst_object_unref(bridgeDev);
        }
        ID3D11Texture2D* tex =
            reinterpret_cast<ID3D11Texture2D*>(gst_d3d11_memory_get_resource_handle(GST_D3D11_MEMORY_CAST(mem)));
        if (!tex) {
            QGC_HW_WARN_ONCE(GstD3D11Log, s_diag.loggedNullResource,
                             "resolve: gst_d3d11_memory_get_resource_handle returned null for plane" << i);
            return;
        }
        // QRhi::createFrom has no subresource selector — copy array slices here on the streaming thread.
        const guint subIdx = gst_d3d11_memory_get_subresource_index(GST_D3D11_MEMORY_CAST(mem));
        D3D11_TEXTURE2D_DESC srcDesc = {};
        tex->GetDesc(&srcDesc);
        if (subIdx > 0 || srcDesc.ArraySize > 1) {
            ID3D11Texture2D* stagingTex = copySliceToStaging(tex, subIdx, i, srcDesc, GST_D3D11_MEMORY_CAST(mem));
            if (!stagingTex) {
                return;
            }
            copyDevice = GST_D3D11_MEMORY_CAST(mem)->device;
            guard.handles[i] = stagingTex;
        } else {
            tex->AddRef();
            guard.handles[i] = tex;
        }
        guard.owned = i + 1;
    }

    // Single flush for every staged slice instead of one per plane: QRhi must see the copies in the immediate queue.
    if (copyDevice) {
        ID3D11DeviceContext* d3dCtx = gst_d3d11_device_get_device_context_handle(copyDevice);
        gst_d3d11_device_lock(copyDevice);
        d3dCtx->Flush();
        gst_d3d11_device_unlock(copyDevice);
    }

    _textures = guard.handles;
    _resolvedCount = memCount;
    _resolved = true;
    guard.commit();
}

QVideoFrameTexturesUPtr GstD3D11VideoBuffer::mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old)
{
    // GstD3D11ContextBridge must be primed; without a shared device createFrom() silently renders garbage.
    GstBuffer* buffer = nullptr;
    if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::D3D11), GstD3D11Log(), s_diag, buffer)) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D11);
    }
    if (!_resolved) {
        return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D11);
    }

    return GstD3DVideoBufferCommon::mapResolvedTextures(
        *this, rhi, old, HwVideoBufferPath::D3D11, _textures, _resolvedCount, _format.frameSize(),
        _format.pixelFormat(), s_diag.loggedFirstSuccess, s_diag.loggedTextureCreateFail, GstD3D11Log, "D3D11");
}

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
