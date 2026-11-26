#include "GstD3DVideoBufferCommon.h"

#if defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <array>
#include <atomic>
#include <d3d12.h>
#include <gst/d3d12/gstd3d12.h>
#include <rhi/qrhi_platform.h>
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include <d3d11_4.h>  // ID3D11Device5, ID3D11DeviceContext4, ID3D11Fence

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#endif

#include "GstD3D12ContextBridge.h"
#include "GstHwPathRegistry.h"
#include "GstHwPathTelemetry.h"
#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include "GstD3D11PictureCopy.h"
#endif

QGC_LOGGING_CATEGORY(GstD3D12Log, "Video.GStreamer.HwBuffers.GstD3D12Buf")

namespace {

/// True once Qt Quick is known to render with Direct3D 12. A stream may start before the render thread says what it
/// renders with.
bool qtRendersWithD3D12()
{
    return QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire) == static_cast<int>(QRhi::D3D12);
}

/// Qt's D3D12 QRhi creates no per-plane views, so it can't sample an NV12/P010 resource's chroma.
bool d3d12RhiSamples(QVideoFrameFormat::PixelFormat pixelFormat)
{
    const auto* description = QVideoTextureHelper::textureDescription(pixelFormat);
    return description && (description->nplanes == 1);
}

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
/// True when @p device says it opens a Direct3D 12 device's resources as textures, NV12 with both planes: what
/// gst-d3d11 asks of a device before its own interop (gst_d3d11_device_d3d12_import_supported()). WARP says so too
/// and then opens NV12 without its chroma plane, but no decoder shares its adapter.
bool d3d11OpensD3D12(ID3D11Device* device)
{
    D3D11_FEATURE_DATA_D3D11_OPTIONS options = {};
    D3D11_FEATURE_DATA_D3D11_OPTIONS4 options4 = {};
    D3D11_FEATURE_DATA_D3D11_OPTIONS5 options5 = {};
    return SUCCEEDED(device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS5, &options5, sizeof(options5))) &&
           (options5.SharedResourceTier >= D3D11_SHARED_RESOURCE_TIER_2) &&
           SUCCEEDED(device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &options, sizeof(options))) &&
           options.ExtendedResourceSharing &&
           SUCCEEDED(device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS4, &options4, sizeof(options4))) &&
           options4.ExtendedNV12SharedTextureSupported;
}
#endif

/// D3D12 pieces of GstD3DVideoBufferCommon's resolvePlanes().
class D3D12PlaneResolver
{
public:
    using Handle = ID3D12Resource;
    using Memory = GstD3D12Memory;
    using Device = GstD3D12Device;

    static constexpr const char* kMemoryName = "GstD3D12Memory";

    static const QLoggingCategory& log() { return GstD3D12Log(); }

    static bool isMemory(GstMemory* mem) { return gst_is_d3d12_memory(mem); }

    static Device* bridgeDevice() { return GstD3D12ContextBridge::currentDevice(); }

    // Compare the underlying device, not the wrapper: each bridge prime creates a new GstD3D12Device.
    static bool sameDevice(Device* a, Device* b) { return gst_d3d12_device_is_equal(a, b); }

    static Handle* resource(Memory* mem) { return gst_d3d12_memory_get_resource_handle(mem); }

    static guint subresource(Memory* mem, int plane)
    {
        guint subIdx = 0;
        gst_d3d12_memory_get_subresource_index(mem, guint(plane), &subIdx);
        return subIdx;
    }

    static bool isArraySlice(Handle* resource, guint subIdx)
    {
        return subIdx > 0 || resource->GetDesc().DepthOrArraySize > 1;
    }

    // Block on the decoder's fence before reading, else QRhi samples mid-write while the decoder is still writing this
    // resource.
    static bool prepare(Memory* mem, int plane)
    {
        if (!gst_d3d12_memory_sync(mem)) {
            QGC_HW_WARN_ONCE(GstD3D12Log, "resolve: gst_d3d12_memory_sync failed for plane" << plane);
            return false;
        }
        return true;
    }

    // QRhi can't sample one slice of a texture array, and none reaches the sink: gst-d3d12's pools hold single
    // textures, and its decoders copy a frame out of a reference-only array before they hand it on.
    static Handle* copySlice(Handle* /*resource*/, guint subIdx, int plane)
    {
        QGC_HW_WARN_ONCE(GstD3D12Log,
                         "resolve: plane" << plane << "is slice" << subIdx << "of a texture array — CPU fallback");
        return nullptr;
    }
};

/// True when @p resource lives on the device behind @p qrhiDevice (QRhiD3D12NativeHandles::dev). The two are compared
/// as COM identities: Qt holds another interface of the device than a resource reports.
bool onDevice(ID3D12Resource* resource, void* qrhiDevice)
{
    IUnknown* owner = nullptr;
    IUnknown* device = nullptr;
    const bool same = qrhiDevice && SUCCEEDED(resource->GetDevice(IID_PPV_ARGS(&owner))) &&
                      SUCCEEDED(static_cast<IUnknown*>(qrhiDevice)->QueryInterface(IID_PPV_ARGS(&device))) &&
                      (owner == device);
    if (owner) {
        owner->Release();
    }
    if (device) {
        device->Release();
    }
    return same;
}

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
/// A fence the Direct3D 11 context of Qt's device signals and a Direct3D 12 device waits on. The two devices order
/// nothing between them, so this is what tells that Direct3D 11 is done reading a resource of the other.
class InteropFence
{
public:
    InteropFence(ID3D11Device* device11, ID3D12Device* device12)
        : _device11(device11)
        , _device12(device12)
    {
        ID3D11Device5* device5 = nullptr;
        HANDLE shared = nullptr;
        if (d3d11OpensD3D12(device11) && SUCCEEDED(device11->QueryInterface(IID_PPV_ARGS(&device5))) && device5 &&
            SUCCEEDED(device5->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&_fence11))) && _fence11 &&
            SUCCEEDED(_fence11->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &shared)) && shared) {
            (void) device12->OpenSharedHandle(shared, IID_PPV_ARGS(&_fence12));
        }
        if (shared) {
            CloseHandle(shared);
        }
        if (device5) {
            device5->Release();
        }
    }

    ~InteropFence()
    {
        if (_fence12) {
            _fence12->Release();
        }
        if (_fence11) {
            _fence11->Release();
        }
    }

    InteropFence(const InteropFence&) = delete;
    InteropFence& operator=(const InteropFence&) = delete;

    bool isValid() const noexcept { return _fence11 && _fence12; }

    bool joins(ID3D11Device* device11, ID3D12Device* device12) const noexcept
    {
        return (_device11 == device11) && (_device12 == device12);
    }

    /// Queues a signal behind what @p context was given so far and submits it all; the value wait() takes, or 0 when
    /// the context can't signal. Render thread.
    quint64 signal(ID3D11DeviceContext* context)
    {
        ID3D11DeviceContext4* context4 = nullptr;
        if (FAILED(context->QueryInterface(IID_PPV_ARGS(&context4))) || !context4) {
            return 0;
        }
        const quint64 value = ++_value;
        const HRESULT signalled = context4->Signal(_fence11, value);
        context4->Release();
        if (FAILED(signalled)) {
            return 0;
        }
        // Qt submits only when it presents: without this the fence would follow the next frame's pace.
        context->Flush();
        return value;
    }

    /// Blocks until the GPU reached signal() @p value, or kWaitMs passed: a lost device never gets there.
    void wait(quint64 value) const
    {
        if ((value == 0) || (_fence12->GetCompletedValue() >= value)) {
            return;
        }
        if (const HANDLE reached = CreateEventW(nullptr, FALSE, FALSE, nullptr)) {
            if (SUCCEEDED(_fence12->SetEventOnCompletion(value, reached))) {
                (void) WaitForSingleObject(reached, kWaitMs);
            }
            CloseHandle(reached);
        }
    }

private:
    static constexpr DWORD kWaitMs = 100;

    // Identities only: the fences hold their devices.
    ID3D11Device* const _device11;
    ID3D12Device* const _device12;
    ID3D11Fence* _fence11 = nullptr;
    ID3D12Fence* _fence12 = nullptr;
    quint64 _value = 0;
};

QMutex s_interopFenceMutex;
std::shared_ptr<InteropFence> s_interopFence;

/// The fence between Qt's @p device11 and GStreamer's @p device12, made on first use; null when @p device11 can't open
/// the other's resources, or they can't share a fence.
std::shared_ptr<InteropFence> interopFence(ID3D11Device* device11, ID3D12Device* device12)
{
    const QMutexLocker lock(&s_interopFenceMutex);
    if (!s_interopFence || !s_interopFence->joins(device11, device12)) {
        s_interopFence = std::make_shared<InteropFence>(device11, device12);
    }
    return s_interopFence->isValid() ? s_interopFence : nullptr;
}
#endif

/// Wraps a D3D12Memory-backed GstSample as a QHwVideoBuffer. The device guard and the decoder sync run in the
/// constructor, on the streaming thread (blocking there is harmless: the decoder is async), so mapTextures() on the
/// render thread only imports resolved resources. A Direct3D 12 QRhi samples the resource itself. A Direct3D 11 QRhi
/// can't, but the resource opens as a texture of its device: the picture is copied out of that on the render thread,
/// as the D3D11 path copies it out of a decoder's padded texture, and GStreamer never uses Qt's device.
class D3D12VideoBuffer final : public GstD3DVideoBufferCommon::VideoBuffer<D3D12PlaneResolver>
{
public:
    D3D12VideoBuffer(GstSample* sample, const GstVideoInfo& videoInfo, const QVideoFrameFormat& format)
        : VideoBuffer(sample, videoInfo, format, HwVideoBufferPath::D3D12)
    {
        // The D3D12Convert bin negotiates single-plane formats only; anything else takes the CPU path, here for the
        // whole stream rather than frame by frame in mapTextures().
        if (qtRendersWithD3D12() && !d3d12RhiSamples(format.pixelFormat())) {
            QGC_HW_WARN_ONCE(GstD3D12Log,
                             "resolve: multi-plane D3D12Memory" << format.pixelFormat() << "can't be sampled by QRhi");
            return;
        }
        D3D12PlaneResolver resolver;
        (void) resolve(resolver);
    }

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    ~D3D12VideoBuffer() override
    {
        // The decoder writes its next frame into this resource once the sample goes: not before Direct3D 11 copied it.
        if (_readFence) {
            _readFence->wait(_readValue);
        }
    }
#endif

    QVideoFrameTexturesUPtr importTextures(QRhi& rhi, QVideoFrameTexturesUPtr& old) override
    {
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        if (rhi.backend() == QRhi::D3D11) {
            return mapOnD3D11(rhi, old);
        }
#endif
        if (!beginMap(rhi, QRhi::D3D12)) {
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
        }
        if (!d3d12RhiSamples(_format.pixelFormat())) {
            QGC_HW_WARN_ONCE(GstD3D12Log, "mapTextures: multi-plane D3D12Memory" << _format.pixelFormat()
                                                                                 << "can't be sampled by QRhi");
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
        }
        // Also catches an unprimed bridge, which skips resolvePlanes()'s device guard: a resource of another device
        // imports fine but renders corrupt.
        const auto* native = static_cast<const QRhiD3D12NativeHandles*>(rhi.nativeHandles());
        if (!native || !onDevice(_handles[0], native->dev)) {
            QGC_HW_WARN_ONCE(GstD3D12Log, "mapTextures: decoder resource is not on QRhi's device — CPU fallback");
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
        }
        return finishMap(rhi, old, _handles);
    }

private:
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    /// mapTextures() for a Direct3D 11 QRhi.
    QVideoFrameTexturesUPtr mapOnD3D11(QRhi& rhi, QVideoFrameTexturesUPtr& old)
    {
        GstBuffer* const buffer = beginMap(rhi, QRhi::D3D11);
        const auto* native = static_cast<const QRhiD3D11NativeHandles*>(rhi.nativeHandles());
        auto* device = native ? static_cast<ID3D11Device*>(native->dev) : nullptr;
        auto* context = native ? static_cast<ID3D11DeviceContext*>(native->context) : nullptr;
        if (!buffer || !device || !context) {
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
        }
        // One NV12/P010 resource holds every plane. Its Direct3D 11 view belongs to the memory, which keeps one per
        // device; none opens for a resource on another adapter, or one whose heap isn't shared.
        auto* memory = reinterpret_cast<GstD3D12Memory*>(gst_buffer_peek_memory(buffer, 0));
        ID3D11Texture2D* const shared = gst_d3d12_memory_get_d3d11_texture(memory, device);
        const std::shared_ptr<InteropFence> fence =
            shared ? interopFence(device, gst_d3d12_device_get_device_handle(memory->device)) : nullptr;
        if (!fence) {
            QGC_HW_WARN_ONCE(GstD3D12Log,
                             "mapTextures: the decoder's resource can't be shared with QRhi's Direct3D 11 device "
                             "— CPU fallback");
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
        }

        ID3D11Texture2D* const picture =
            GstD3D11PictureCopy::copy(device, context, shared, 0, QRect(QPoint(0, 0), _format.frameSize()), old, 0);
        if (!picture) {
            QGC_HW_WARN_ONCE(GstD3D12Log, "mapTextures: CreateTexture2D for the picture copy failed");
            return GstHwPathTelemetry::fail(HwVideoBufferPath::D3D12);
        }
        _readFence = fence;
        _readValue = fence->signal(context);

        const GstHwMapTail tail{.path = HwVideoBufferPath::D3D12,
                                .frameSize = _format.frameSize(),
                                .pixelFormat = _format.pixelFormat(),
                                .log = &GstD3D12Log(),
                                .successTag = "D3D12 on D3D11"};
        const std::array<ID3D11Texture2D*, GstHw::kMaxPlanes> planes{picture};
        QVideoFrameTexturesUPtr textures =
            GstD3DVideoBufferCommon::mapResolvedTextures(*this, rhi, old, tail, planes, 1);
        picture->Release();
        return textures;
    }

    /// What mapOnD3D11() signalled last, for the destructor to wait on.
    std::shared_ptr<InteropFence> _readFence;
    quint64 _readValue = 0;
#endif
};

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
/// HwPathDescriptor::probeDevice: whether the Direct3D 11 device Qt Quick renders with takes this path's frames, for
/// the sink to offer their memory.
void probeDevice(QRhi& rhi)
{
    const auto* native =
        (rhi.backend() == QRhi::D3D11) ? static_cast<const QRhiD3D11NativeHandles*>(rhi.nativeHandles()) : nullptr;
    auto* device = native ? static_cast<ID3D11Device*>(native->dev) : nullptr;
    QGCRhiCapture::deviceSnapshot().d3d11OpensD3D12.store(device && d3d11OpensD3D12(device), std::memory_order_release);
}
#endif

void resetCachedState() noexcept
{
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    {
        const QMutexLocker lock(&s_interopFenceMutex);
        s_interopFence.reset();
    }
#endif
}

}  // namespace

constinit const HwPathDescriptor GstHwPaths::kD3D12 = {
    .path = HwVideoBufferPath::D3D12,
    .matches = [](GstMemory* mem) -> bool { return gst_is_d3d12_memory(mem); },
    .construct = &GstHwPaths::construct<D3D12VideoBuffer>,
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    .probeDevice = &probeDevice,
#endif
    .resetCaches = &resetCachedState,
    .bridge = &GstD3D12ContextBridge::instance,
};

#endif  // QGC_HAS_GST_D3D12_GPU_PATH
