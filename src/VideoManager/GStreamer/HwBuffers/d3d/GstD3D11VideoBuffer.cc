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
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"

QGC_LOGGING_CATEGORY(GstD3D11Log, "Video.GStreamer.HwBuffers.GstD3D11Buf")

namespace {

using GstD3DVideoBufferCommon::MapDiagnostics;
using GstHw::kMaxPlanes;
using D3D11FrameTextures = GstD3DVideoBufferCommon::FrameTextures<ID3D11Texture2D>;

MapDiagnostics s_diag;
std::atomic<bool> s_loggedForeignDevice{false};

// Tags the copy targets this path creates: only those may be overwritten by a later frame. A previous bundle can also
// wrap a texture GStreamer still owns.
constexpr GUID kCopyTargetTag = {0x6d3c1f52, 0x8a4e, 0x4b7f, {0x9c, 0x21, 0x5e, 0x0b, 0x7a, 0x93, 0xd4, 0x18}};

/// D3D11 pieces of GstD3DVideoBufferCommon's resolvePlanes() and proposePool(). A texture-array slice is only recorded
/// here: D3D11VideoBuffer copies it on the render thread.
class D3D11PlaneResolver
{
public:
    using Handle = ID3D11Texture2D;
    using Memory = GstD3D11Memory;
    using Device = GstD3D11Device;

    static constexpr const char* kMemoryName = "GstD3D11Memory";
    static constexpr const char* kCapsFeature = GST_CAPS_FEATURE_MEMORY_D3D11_MEMORY;

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
            QGC_HW_WARN_ONCE(GstD3D11Log, s_diag.loggedNullResource,
                             "resolve: mapping the GstD3D11Memory for D3D11 failed for plane" << plane);
            return false;
        }
        gst_memory_unmap(GST_MEMORY_CAST(mem), &info);
        return true;
    }

    Handle* copySlice(Handle* tex, guint subIdx, int plane, Memory* /*mem*/)
    {
        tex->AddRef();
        _slices[std::size_t(plane)] = subIdx;
        _copied[std::size_t(plane)] = true;
        return tex;
    }

    static bool finish() { return true; }

    const std::array<guint, kMaxPlanes>& slices() const noexcept { return _slices; }

    const std::array<bool, kMaxPlanes>& copied() const noexcept { return _copied; }

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
    std::array<guint, kMaxPlanes> _slices{};
    std::array<bool, kMaxPlanes> _copied{};
};

/// D3D11 creates 4:2:0 textures with even sizes only.
bool hasEvenSizes(DXGI_FORMAT format)
{
    switch (format) {
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            return true;
        default:
            return false;
    }
}

/// Copy extent of @p extent pixels into a @p format texture.
UINT copyExtent(int extent, DXGI_FORMAT format)
{
    return static_cast<UINT>(hasEvenSizes(format) ? ((extent + 1) & ~1) : extent);
}

constexpr quint8 kCopyTargetMark = 1;

bool isCopyTarget(ID3D11Texture2D* texture)
{
    quint8 mark = 0;
    UINT size = static_cast<UINT>(sizeof(mark));
    return SUCCEEDED(texture->GetPrivateData(kCopyTargetTag, &size, &mark)) && (mark == kCopyTargetMark);
}

/// True when @p texture lives on @p device, the comparison Qt's D3D11 texture bridge makes.
bool onDevice(ID3D11Texture2D* texture, ID3D11Device* device)
{
    ID3D11Device* owner = nullptr;
    texture->GetDevice(&owner);
    const bool same = (owner != nullptr) && (owner == device);
    if (owner) {
        owner->Release();
    }
    return same;
}

/// GstD3D11Memory frame. The streaming thread only resolves the decoder's textures. mapTextures() runs on the render
/// thread, where QRhi drives the same immediate context: a texture-array slice or a padded texture is copied there,
/// ordered with QRhi's own reads and writes without a Flush(), and only for frames that are actually shown.
class D3D11VideoBuffer final : public GstHwVideoBuffer
{
public:
    D3D11VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format)
        : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, videoInfo, format)
    {
        GstBuffer* buffer = _sample ? gst_sample_get_buffer(_sample) : nullptr;
        if (!buffer) {
            return;
        }
        GstD3DVideoBufferCommon::PlaneResourceGuard<ID3D11Texture2D> guard;
        D3D11PlaneResolver resolver;
        const int count = GstD3DVideoBufferCommon::resolvePlanes(buffer, resolver, s_diag, guard);
        if (count == 0) {
            return;
        }
        _textures = guard.handles;
        _count = count;
        guard.commit();
        _slices = resolver.slices();
        _copied = resolver.copied();

        // Decoders pad their textures to their alignment (16 rows for H.264, 128 for HEVC/AV1) with no crop meta. The
        // picture is copied out like an array slice's, so the frame keeps the picture's size and its CPU layout: Qt
        // maps it to the CPU for a window on another device and for screenshots.
        D3D11_TEXTURE2D_DESC desc = {};
        _textures[0]->GetDesc(&desc);
        if (GstHw::texturePadsPicture(QSize(int(desc.Width), int(desc.Height)), _format.frameSize(),
                                      hasEvenSizes(desc.Format))) {
            std::fill_n(_copied.begin(), _count, true);
        }
    }

    ~D3D11VideoBuffer() override
    {
        for (int i = 0; i < _count; ++i) {
            if (_textures[std::size_t(i)]) {
                _textures[std::size_t(i)]->Release();
            }
        }
    }

    bool validatePlaneHandles() const override
    {
        // A failed constructor-time resolve (device mismatch, missing resource) sends the frame to the CPU.
        if (_count == 0) {
            return false;
        }
        return validatePlanes([](GstMemory* mem) {
            return mem && gst_is_d3d11_memory(mem) &&
                   (D3D11PlaneResolver::resource(reinterpret_cast<GstD3D11Memory*>(mem)) != nullptr);
        });
    }

    QVideoFrameTexturesUPtr mapTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old) override
    {
        GstBuffer* buffer = nullptr;
        if (!checkMapPreconditions(rhi, static_cast<int>(QRhi::D3D11), GstD3D11Log(), s_diag, buffer) ||
            (_count == 0)) {
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D11);
        }
        // Also catches an unprimed bridge, which skips resolvePlanes()'s device guard: QRhi can't sample another
        // device's texture.
        const auto* native = static_cast<const QRhiD3D11NativeHandles*>(rhi.nativeHandles());
        auto* device = native ? static_cast<ID3D11Device*>(native->dev) : nullptr;
        auto* context = native ? static_cast<ID3D11DeviceContext*>(native->context) : nullptr;
        if (!device || !context || !onDevice(_textures[0], device)) {
            QGC_HW_WARN_ONCE(GstD3D11Log, s_loggedForeignDevice,
                             "mapTextures: decoder texture is not on QRhi's device — CPU fallback");
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D11);
        }

        std::array<ID3D11Texture2D*, kMaxPlanes> planes = _textures;
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

        const GstHwMapTail tail{.path = HwVideoBufferPath::D3D11,
                                .frameSize = _format.frameSize(),
                                .pixelFormat = _format.pixelFormat(),
                                .log = &GstD3D11Log(),
                                .diag = &s_diag,
                                .successTag = GstHw::toString(HwVideoBufferPath::D3D11)};
        return GstD3DVideoBufferCommon::mapResolvedTextures(*this, rhi, old, tail, planes, _count);
    }

private:
    /// Copies the picture of plane @p plane's array slice or padded texture into a texture of exactly its size (the
    /// decoder's surfaces carry alignment padding, cf. Qt's TextureBridge::copyToSharedTex): @p old's copy target when
    /// it has that layout, whose last read QRhi already queued on this context, else a new one. AddRef'd; null on
    /// failure.
    ID3D11Texture2D* copyPicture(QRhi& rhi, ID3D11Device* device, ID3D11DeviceContext* context, GstBuffer* buffer,
                                 int plane, QVideoFrameTexturesUPtr& old)
    {
        ID3D11Texture2D* source = _textures[std::size_t(plane)];
        D3D11_TEXTURE2D_DESC sourceDesc = {};
        source->GetDesc(&sourceDesc);
        // One texture holds every plane (NV12, P010), else this plane's texture holds only its own share of the frame.
        const auto* textureDescription = QVideoTextureHelper::textureDescription(_format.pixelFormat());
        const QSize picture = ((_count > 1) && textureDescription)
                                  ? textureDescription->rhiPlaneSize(_format.frameSize(), plane, &rhi)
                                  : _format.frameSize();
        const UINT width = (std::min) (copyExtent(picture.width(), sourceDesc.Format), sourceDesc.Width);
        const UINT height = (std::min) (copyExtent(picture.height(), sourceDesc.Format), sourceDesc.Height);

        D3D11_TEXTURE2D_DESC targetDesc = {};
        targetDesc.Width = width;
        targetDesc.Height = height;
        targetDesc.MipLevels = 1;
        targetDesc.ArraySize = 1;
        targetDesc.Format = sourceDesc.Format;
        targetDesc.SampleDesc.Count = 1;
        targetDesc.Usage = D3D11_USAGE_DEFAULT;
        targetDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        ID3D11Texture2D* target = reusableTarget(old, plane, targetDesc);
        if (!target) {
            if (FAILED(device->CreateTexture2D(&targetDesc, nullptr, &target)) || !target) {
                QGC_HW_WARN_ONCE(GstD3D11Log, s_diag.loggedTextureCreateFail,
                                 "mapTextures: CreateTexture2D for the slice copy failed (plane=" << plane << ")");
                return nullptr;
            }
            (void) target->SetPrivateData(kCopyTargetTag, static_cast<UINT>(sizeof(kCopyTargetMark)), &kCopyTargetMark);
        }

        // Serializes with GStreamer's own use of this immediate context (the decoder, on the streaming thread).
        auto* memory = reinterpret_cast<GstD3D11Memory*>(gst_buffer_peek_memory(buffer, guint(plane)));
        const D3D11_BOX pictureBox = {0, 0, 0, width, height, 1};
        gst_d3d11_device_lock(memory->device);
        context->CopySubresourceRegion(target, 0, 0, 0, 0, source, _slices[std::size_t(plane)], &pictureBox);
        gst_d3d11_device_unlock(memory->device);
        return target;
    }

    /// @p old's plane @p plane texture, AddRef'd, when it is a copy target this path made with @p desc's layout.
    static ID3D11Texture2D* reusableTarget(QVideoFrameTexturesUPtr& old, int plane, const D3D11_TEXTURE2D_DESC& desc)
    {
        auto* previous = dynamic_cast<D3D11FrameTextures*>(old.get());
        ID3D11Texture2D* texture = previous ? previous->handle(plane) : nullptr;
        if (!texture || !isCopyTarget(texture)) {
            return nullptr;
        }
        D3D11_TEXTURE2D_DESC have = {};
        texture->GetDesc(&have);
        if ((have.Width != desc.Width) || (have.Height != desc.Height) || (have.Format != desc.Format)) {
            return nullptr;
        }
        texture->AddRef();
        return texture;
    }

    std::array<ID3D11Texture2D*, kMaxPlanes> _textures{};
    std::array<guint, kMaxPlanes> _slices{};
    std::array<bool, kMaxPlanes> _copied{};
    int _count = 0;
};

void resetCachedState() noexcept
{
    s_diag.reset();
    s_loggedForeignDevice.store(false, std::memory_order_relaxed);
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kD3D11 = {
    .path = HwVideoBufferPath::D3D11,
    .matches = [](GstMemory* mem, const HwVideoBufferContext&) -> bool { return gst_is_d3d11_memory(mem); },
    .construct = &GstHwPaths::construct<D3D11VideoBuffer>,
    .resetCaches = &resetCachedState,
    .proposePool = &GstD3DVideoBufferCommon::proposePool<D3D11PlaneResolver>,
    .bridge = &GstD3D11ContextBridge::instance,
};

#endif  // QGC_HAS_GST_D3D11_GPU_PATH
