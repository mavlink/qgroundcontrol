#include "GstD3DVideoBufferCommon.h"

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <algorithm>
#include <array>
#include <cstddef>
#include <d3d11.h>
#include <gst/d3d11/gstd3d11.h>

#include <QtCore/QMutex>

#include "GstD3D11ContextBridge.h"
#include "GstHwImportCache.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstD3D11Log, "Video.GStreamer.HwBuffers.GstD3D11Buf")

namespace {

using GstD3DVideoBufferCommon::MapDiagnostics;
using GstD3DVideoBufferCommon::StagingKey;

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
        StagingRing* ring = _entries.find(key).resource;
        if (!ring) {
            ring = _entries.insert(key, StagingRing{}).resource;
        }
        const std::size_t slot = ring->next;
        ring->next = (ring->next + 1) % kRingSize;
        if (ring->textures[slot]) {
            GstHwPathTelemetry::record(HwVideoBufferPath::D3D11, GstHwPathTelemetry::Counter::ImageCacheHits);
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
        GstHwPathTelemetry::record(HwVideoBufferPath::D3D11, GstHwPathTelemetry::Counter::ImageCacheMisses);
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

    static void releaseRing(StagingRing& ring)
    {
        for (ID3D11Texture2D*& tex : ring.textures) {
            if (tex) {
                tex->Release();
                tex = nullptr;
            }
        }
    }

    static constexpr std::size_t kMaxEntries = 8;
    QMutex _mutex;
    GstHw::GstHwImportCache<StagingKey, StagingRing, kMaxEntries> _entries{&releaseRing};
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

/// D3D11 pieces of GstD3DVideoBufferCommon's resolvePlanes(), VideoBuffer and proposePool().
class D3D11PlaneResolver
{
public:
    using Handle = ID3D11Texture2D;
    using Memory = GstD3D11Memory;
    using Device = GstD3D11Device;

    static constexpr const char* kMemoryName = "GstD3D11Memory";
    static constexpr HwVideoBufferPath kPath = HwVideoBufferPath::D3D11;
    static constexpr QRhi::Implementation kBackend = QRhi::D3D11;
    static constexpr const char* kCapsFeature = GST_CAPS_FEATURE_MEMORY_D3D11_MEMORY;

    static const QLoggingCategory& log() { return GstD3D11Log(); }

    static MapDiagnostics& diag() { return s_diag; }

    static bool isMemory(GstMemory* mem) { return gst_is_d3d11_memory(mem); }

    static Device* bridgeDevice() { return GstD3D11ContextBridge::currentDevice(); }

    // Compare the wrapped ID3D11Device, not the wrapper: each bridge prime wraps QRhi's device anew.
    static bool sameDevice(Device* a, Device* b)
    {
        return gst_d3d11_device_get_device_handle(a) == gst_d3d11_device_get_device_handle(b);
    }

    static Handle* resource(Memory* mem)
    {
        return reinterpret_cast<ID3D11Texture2D*>(gst_d3d11_memory_get_resource_handle(mem));
    }

    static guint subresource(Memory* mem, int /*plane*/) { return gst_d3d11_memory_get_subresource_index(mem); }

    static bool isArraySlice(Handle* tex, guint subIdx)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        tex->GetDesc(&desc);
        return subIdx > 0 || desc.ArraySize > 1;
    }

    static bool prepare(Memory* /*mem*/, int /*plane*/) { return true; }

    Handle* copySlice(Handle* tex, guint subIdx, int plane, Memory* mem)
    {
        D3D11_TEXTURE2D_DESC srcDesc = {};
        tex->GetDesc(&srcDesc);
        Handle* staging = copySliceToStaging(tex, subIdx, plane, srcDesc, mem);
        if (staging) {
            _copyDevice = mem->device;
        }
        return staging;
    }

    // Single flush for every staged slice instead of one per plane: QRhi must see the copies in the immediate queue.
    bool finish()
    {
        if (_copyDevice) {
            ID3D11DeviceContext* d3dCtx = gst_d3d11_device_get_device_context_handle(_copyDevice);
            gst_d3d11_device_lock(_copyDevice);
            d3dCtx->Flush();
            gst_d3d11_device_unlock(_copyDevice);
        }
        return true;
    }

    // The staging ring rotates its textures itself; nothing for the bundle to hold.
    static std::shared_ptr<void> takeKeepAlive() { return {}; }

    static GstBufferPool* newPool(Device* device) { return gst_d3d11_buffer_pool_new(device); }

    // SHADER_RESOURCE matches the importer's sampled texture.
    static void setAllocationParams(GstStructure* config, Device* device, const GstVideoInfo* vinfo)
    {
        if (GstD3D11AllocationParams* params = gst_d3d11_allocation_params_new(
                device, vinfo, GST_D3D11_ALLOCATION_FLAG_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0)) {
            gst_buffer_pool_config_set_d3d11_allocation_params(config, params);
            gst_d3d11_allocation_params_free(params);
        }
    }

private:
    GstD3D11Device* _copyDevice = nullptr;
};

void resetCachedState() noexcept
{
    StagingTexturePool::instance().clear();
    s_diag.reset();
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kD3D11 = {
    .path = HwVideoBufferPath::D3D11,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool { return gst_is_d3d11_memory(mem); },
    .construct = &GstHwPaths::construct<GstD3DVideoBufferCommon::VideoBuffer<D3D11PlaneResolver>>,
    .resetCaches = &resetCachedState,
    .proposePool = &GstD3DVideoBufferCommon::proposePool<D3D11PlaneResolver>,
    .bridge = &GstD3D11ContextBridge::instance,
};

#endif  // QGC_HAS_GST_D3D11_GPU_PATH
