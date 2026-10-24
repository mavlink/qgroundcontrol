#include "GstD3D11ContextBridge.h"

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <d3d11.h>
#include <d3d11_4.h>  // ID3D11Multithread
#include <gst/d3d11/gstd3d11.h>
#include <rhi/qrhi.h>  // QRhi::Implementation enum only; no runtime use across threads

#include "GstD3DContextBridge.h"
#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"

QGC_LOGGING_CATEGORY(GstD3D11BridgeLog, "Video.GStreamer.HwBuffers.GstD3D11Bridge")

namespace {

constexpr const char* kContextTypes[] = {GST_D3D11_DEVICE_HANDLE_CONTEXT_TYPE};

class D3D11ContextBridge final : public GstD3DContextBridge
{
public:
    constexpr D3D11ContextBridge() noexcept
        : GstD3DContextBridge("D3D11", kContextTypes, &GstD3D11BridgeLog, int(QRhi::D3D11))
    {}

private:
    GstObject* createDevice() override;
    GstContext* makeDeviceContext(GstObject* device) override;
};

constinit D3D11ContextBridge s_bridge;

GstObject* D3D11ContextBridge::createDevice()
{
    // Snapshot populated on the render thread (sceneGraphInitialized); only atomic loads here.
    auto* dev = static_cast<ID3D11Device*>(QGCRhiCapture::deviceSnapshot().d3d11Device.load(std::memory_order_acquire));
    if (!dev) {
        qCWarning(GstD3D11BridgeLog) << "QRhi D3D11 snapshot missing ID3D11Device*";
        return nullptr;
    }

    // GStreamer's elements share QRhi's immediate context. QGCRhiCapture turns its multithread protection on from the
    // render thread before publishing the device; switching it here, while QRhi may be inside the context, is only the
    // fallback for a device published some other way.
    ID3D11DeviceContext* immediate = nullptr;
    dev->GetImmediateContext(&immediate);
    if (immediate) {
        ID3D11Multithread* multithread = nullptr;
        if (SUCCEEDED(immediate->QueryInterface(__uuidof(ID3D11Multithread), reinterpret_cast<void**>(&multithread))) &&
            multithread) {
            if (!multithread->GetMultithreadProtected()) {
                (void) multithread->SetMultithreadProtected(TRUE);
            }
            multithread->Release();
        }
        immediate->Release();
    }

    // gst_d3d11_device_new_wrapped: shared device keeps textures QRhi-sampleable without keyed-mutex transfer.
    GstD3D11Device* device = gst_d3d11_device_new_wrapped(dev);
    if (!device) {
        qCWarning(GstD3D11BridgeLog) << "gst_d3d11_device_new_wrapped failed";
        return nullptr;
    }

    const gint64 expectedLuid = QGCRhiCapture::deviceSnapshot().adapterLuid.load(std::memory_order_acquire);
    logAdapterMatch(expectedLuid, device);
    return GST_OBJECT(device);
}

GstContext* D3D11ContextBridge::makeDeviceContext(GstObject* device)
{
    // gst_d3d11_context_new gst_object_ref's the device internally; caller retains ownership.
    return gst_d3d11_context_new(GST_D3D11_DEVICE_CAST(device));
}

}  // namespace

GstContextBridgeBase& GstD3D11ContextBridge::instance()
{
    return s_bridge;
}

GstD3D11Device* GstD3D11ContextBridge::currentDevice()
{
    return GST_D3D11_DEVICE_CAST(s_bridge.refDevice());
}

#endif  // QGC_HAS_GST_D3D11_GPU_PATH
