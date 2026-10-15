#include "GstD3D11VideoBuffer.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <algorithm>
#include <array>
#include <cstddef>
#include <d3d11.h>
#include <gst/d3d11/gstd3d11.h>

#include <QtCore/QMutex>

#include "GstD3D11ContextBridge.h"
#include "GstD3DVideoBufferCommon.h"
#include "GstHwImportCache.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstD3D11Log, "Video.GStreamer.HwBuffers.GstD3D11Buf")

namespace {

using GstD3DVideoBufferCommon::kMaxPlanes;
using GstD3DVideoBufferCommon::MapDiagnostics;
using D3D11FrameTextures = GstD3DVideoBufferCommon::FrameTextures<ID3D11Texture2D>;
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

/// D3D11 pieces of GstD3DVideoBufferCommon::resolvePlanes().
class D3D11PlaneResolver
{
public:
    using Handle = ID3D11Texture2D;
    using Memory = GstD3D11Memory;
    using Device = GstD3D11Device;

    static constexpr const char* kMemoryName = "GstD3D11Memory";

    static const QLoggingCategory& log() { return GstD3D11Log(); }

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

private:
    GstD3D11Device* _copyDevice = nullptr;
};

}  // namespace

void GstD3D11VideoBuffer::resetCachedState() noexcept
{
    StagingTexturePool::instance().clear();
    s_diag.reset();
}


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

    GstD3DVideoBufferCommon::PlaneResourceGuard<ID3D11Texture2D> guard;
    D3D11PlaneResolver resolver;
    const int resolvedCount = GstD3DVideoBufferCommon::resolvePlanes(buffer, resolver, s_diag, guard);
    if (resolvedCount == 0) {
        return;
    }

    _textures = guard.handles;
    _resolvedCount = resolvedCount;
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

    const GstHwMapTail tail{.path = HwVideoBufferPath::D3D11,
                            .frameSize = _format.frameSize(),
                            .pixelFormat = _format.pixelFormat(),
                            .log = &GstD3D11Log(),
                            .diag = &s_diag,
                            .successTag = "D3D11"};
    return GstD3DVideoBufferCommon::mapResolvedTextures(*this, rhi, old, tail, _textures, _resolvedCount);
}

namespace {

// Bind allocation to QRhi's shared device so import stays same-device instead of hitting
// GstD3D11VideoBuffer's foreign-device copy path; SHADER_RESOURCE matches the importer's sampled texture.
bool proposeD3D11Pool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size, guint minBuffers)
{
    GstCapsFeatures* features = gst_caps_get_features(caps, 0);
    if (!features || !gst_caps_features_contains(features, GST_CAPS_FEATURE_MEMORY_D3D11_MEMORY)) {
        return false;
    }
    GstD3D11Device* device = GstD3D11ContextBridge::currentDevice();
    if (!device) {
        return false;
    }
    bool proposed = false;
    if (GstBufferPool* pool = gst_d3d11_buffer_pool_new(device)) {
        GstStructure* config = gst_buffer_pool_get_config(pool);
        gst_buffer_pool_config_set_params(config, caps, size, minBuffers, 0);
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
            gst_query_add_allocation_pool(query, pool, poolSize, minBuffers, 0);
            proposed = true;
        }
        gst_object_unref(pool);
    }
    gst_object_unref(device);  // currentDevice() returns a transfer-full ref.
    return proposed;
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kD3D11 = {
    .path = HwVideoBufferPath::D3D11,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool { return gst_is_d3d11_memory(mem); },
    .construct = &GstHwPaths::construct<GstD3D11VideoBuffer>,
    .resetCaches = &GstD3D11VideoBuffer::resetCachedState,
    .proposePool = &proposeD3D11Pool,
    .bridge = &GstD3D11ContextBridge::instance,
};

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
