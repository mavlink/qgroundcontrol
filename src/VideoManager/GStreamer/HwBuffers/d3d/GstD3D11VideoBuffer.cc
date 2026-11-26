#include "GstD3DVideoBufferCommon.h"

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <algorithm>
#include <array>
#include <atomic>
#include <d3d11.h>
#include <gst/d3d11/gstd3d11.h>
#include <rhi/qrhi_platform.h>

#include <QtCore/QScopeGuard>

#include "GstD3D11ContextBridge.h"
#include "GstD3D11PictureCopy.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"

QGC_LOGGING_CATEGORY(GstD3D11Log, "Video.GStreamer.HwBuffers.GstD3D11Buf")

namespace {

using GstHw::kMaxPlanes;

/// D3D11 pieces of GstD3DVideoBufferCommon's resolvePlanes(). A texture-array slice is only recorded here:
/// D3D11VideoBuffer copies it on the render thread.
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

    // A frame written through the CPU, as d3d11upload writes system memory, reaches the texture only when its memory
    // is mapped for D3D11; a decoder's frame has no such upload pending.
    static bool prepare(Memory* mem, int plane)
    {
        GstMapInfo info;
        if (!gst_memory_map(GST_MEMORY_CAST(mem), &info, static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_D3D11))) {
            QGC_HW_WARN_ONCE(GstD3D11Log, "resolve: mapping the GstD3D11Memory for D3D11 failed for plane" << plane);
            return false;
        }
        gst_memory_unmap(GST_MEMORY_CAST(mem), &info);
        return true;
    }

    Handle* copySlice(Handle* tex, guint subIdx, int plane)
    {
        tex->AddRef();
        _slices[std::size_t(plane)] = subIdx;
        _copied[std::size_t(plane)] = true;
        return tex;
    }

    const std::array<guint, kMaxPlanes>& slices() const noexcept { return _slices; }

    const std::array<bool, kMaxPlanes>& copied() const noexcept { return _copied; }

private:
    std::array<guint, kMaxPlanes> _slices{};
    std::array<bool, kMaxPlanes> _copied{};
};

/// GstD3D11Memory frame. The streaming thread only resolves the decoder's textures. mapTextures() runs on the render
/// thread, where QRhi drives the same immediate context: a texture-array slice or a padded texture is copied there,
/// ordered with QRhi's own reads and writes without a Flush(), and only for frames that are actually shown.
class D3D11VideoBuffer final : public GstD3DVideoBufferCommon::VideoBuffer<D3D11PlaneResolver>
{
public:
    D3D11VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format)
        : VideoBuffer(sample, videoInfo, format, HwVideoBufferPath::D3D11)
    {
        D3D11PlaneResolver resolver;
        if (!resolve(resolver)) {
            return;
        }
        _slices = resolver.slices();
        _copied = resolver.copied();

        // Decoders pad their textures to their alignment (16 rows for H.264, 128 for HEVC/AV1) with no crop meta. The
        // picture is copied out like an array slice's, so the frame keeps the picture's size and its CPU layout: Qt
        // maps it to the CPU for a window on another device and for screenshots.
        D3D11_TEXTURE2D_DESC desc = {};
        _handles[0]->GetDesc(&desc);
        if (GstHw::texturePadsPicture(QSize(int(desc.Width), int(desc.Height)), _format.frameSize(),
                                      GstD3D11PictureCopy::hasEvenSizes(desc.Format))) {
            std::fill_n(_copied.begin(), _count, true);
        }
    }

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old) override
    {
        GstBuffer* const buffer = beginMap(rhi, QRhi::D3D11);
        if (!buffer) {
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D11);
        }
        // Also catches an unprimed bridge, which skips resolvePlanes()'s device guard: QRhi can't sample another
        // device's texture.
        const auto* native = static_cast<const QRhiD3D11NativeHandles*>(rhi.nativeHandles());
        auto* device = native ? static_cast<ID3D11Device*>(native->dev) : nullptr;
        auto* context = native ? static_cast<ID3D11DeviceContext*>(native->context) : nullptr;
        if (!device || !context || !GstD3D11PictureCopy::onDevice(_handles[0], device)) {
            QGC_HW_WARN_ONCE(GstD3D11Log, "mapTextures: decoder texture is not on QRhi's device — CPU fallback");
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D11);
        }

        std::array<ID3D11Texture2D*, kMaxPlanes> planes = _handles;
        std::array<ID3D11Texture2D*, kMaxPlanes> copies{};
        const auto releaseCopies = qScopeGuard([&copies] {
            for (ID3D11Texture2D* copy : copies) {
                if (copy) {
                    copy->Release();
                }
            }
        });
        for (int i = 0; i < _count; ++i) {
            if (!_copied[std::size_t(i)]) {
                continue;
            }
            copies[std::size_t(i)] = copyPicture(rhi, device, context, buffer, i, old);
            if (!copies[std::size_t(i)]) {
                return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D11);
            }
            planes[std::size_t(i)] = copies[std::size_t(i)];
        }

        return finishMap(rhi, old, planes);
    }

private:
    /// Copies the picture of plane @p plane's array slice or padded texture into a texture of exactly its size (the
    /// decoder's surfaces carry alignment padding, cf. Qt's TextureBridge::copyToSharedTex). AddRef'd; null on failure.
    ID3D11Texture2D* copyPicture(QRhi& rhi, ID3D11Device* device, ID3D11DeviceContext* context, GstBuffer* buffer,
                                 int plane, QVideoFrameTexturesUPtr& old)
    {
        // One texture holds every plane (NV12, P010), else this plane's texture holds only its own share of the frame.
        const auto* textureDescription = QVideoTextureHelper::textureDescription(_format.pixelFormat());
        const QSize picture = ((_count > 1) && textureDescription)
                                  ? textureDescription->rhiPlaneSize(_format.frameSize(), plane, &rhi)
                                  : _format.frameSize();

        // Serializes with GStreamer's own use of this immediate context (the decoder, on the streaming thread).
        auto* memory = reinterpret_cast<GstD3D11Memory*>(gst_buffer_peek_memory(buffer, guint(plane)));
        gst_d3d11_device_lock(memory->device);
        ID3D11Texture2D* const target =
            GstD3D11PictureCopy::copy(device, context, _handles[std::size_t(plane)], _slices[std::size_t(plane)],
                                      QRect(QPoint(0, 0), picture), old, plane);
        gst_d3d11_device_unlock(memory->device);
        if (!target) {
            QGC_HW_WARN_ONCE(GstD3D11Log,
                             "mapTextures: CreateTexture2D for the slice copy failed (plane=" << plane << ")");
        }
        return target;
    }

    std::array<guint, kMaxPlanes> _slices{};
    std::array<bool, kMaxPlanes> _copied{};
};

/// HwPathDescriptor::proposePool: adds a pool on the bridge's device (Qt's) to an ALLOCATION query whose caps carry
/// D3D11Memory, so the decoder allocates on the device QRhi samples from.
bool proposePool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size, guint minBuffers)
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
        // SHADER_RESOURCE matches the importer's sampled texture.
        if (GstD3D11AllocationParams* params = gst_d3d11_allocation_params_new(
                device, vinfo, GST_D3D11_ALLOCATION_FLAG_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0)) {
            gst_buffer_pool_config_set_d3d11_allocation_params(config, params);
            gst_d3d11_allocation_params_free(params);
        }
        if (gst_buffer_pool_set_config(pool, config)) {
            // The pool may grow the buffer size to the allocated texture's pitch; re-read it.
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
    .matches = [](GstMemory* mem) -> bool { return gst_is_d3d11_memory(mem); },
    .construct = &GstHwPaths::construct<D3D11VideoBuffer>,
    .proposePool = &proposePool,
    .bridge = &GstD3D11ContextBridge::instance,
};

#endif  // QGC_HAS_GST_D3D11_GPU_PATH
