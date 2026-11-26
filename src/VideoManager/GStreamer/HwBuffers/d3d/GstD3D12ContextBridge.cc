#include "GstD3D12ContextBridge.h"

#if defined(QGC_HAS_GST_D3D_GPU_PATH)

#include <gst/d3d12/gstd3d12.h>
#include <rhi/qrhi.h>  // QRhi::Implementation enum only; native handles come from the snapshot

#include "GstD3DContextBridge.h"
#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"

QGC_LOGGING_CATEGORY(GstD3D12BridgeLog, "Video.GStreamer.HwBuffers.GstD3D12Bridge")

namespace {

constexpr const char* kContextTypes[] = {GST_D3D12_DEVICE_HANDLE_CONTEXT_TYPE};

/// Under a Direct3D 11 QRhi the device is GStreamer's own on Qt's adapter: a frame decoded there opens as a texture of
/// Qt's device (GstD3D12VideoBuffer), which GStreamer then never touches.
class D3D12ContextBridge final : public GstD3DContextBridge
{
public:
    constexpr D3D12ContextBridge() noexcept
        : GstD3DContextBridge("D3D12", kContextTypes, &GstD3D12BridgeLog, int(QRhi::D3D12), int(QRhi::D3D11))
    {}

private:
    GstObject* createDevice() override;
    GstContext* makeDeviceContext(GstObject* device) override;
};

constinit D3D12ContextBridge s_bridge;

GstObject* D3D12ContextBridge::createDevice()
{
    const QGCRhiCapture::DeviceSnapshot& snapshot = QGCRhiCapture::deviceSnapshot();
    // Every pipeline that takes a decoder gets the device: under a Direct3D 11 QRhi it is created only where Qt's
    // device opens its frames.
    if ((snapshot.backend.load(std::memory_order_acquire) == int(QRhi::D3D11)) &&
        !snapshot.d3d11OpensD3D12.load(std::memory_order_acquire)) {
        qCDebug(GstD3D12BridgeLog) << "Qt's D3D11 device opens no D3D12 resources; no D3D12 device shared";
        return nullptr;
    }
    // LUID halves were sign-extended into a 64-bit value matching LARGE_INTEGER::QuadPart when the snapshot was
    // composed.
    const gint64 luid = snapshot.adapterLuid.load(std::memory_order_acquire);

    GstD3D12Device* device = gst_d3d12_device_new_for_adapter_luid(luid);
    if (!device) {
        qCWarning(GstD3D12BridgeLog) << "gst_d3d12_device_new_for_adapter_luid failed (luid=" << luid << ")";
        return nullptr;
    }

    logAdapterMatch(luid, device);
    return GST_OBJECT(device);
}

GstContext* D3D12ContextBridge::makeDeviceContext(GstObject* device)
{
    // gst_d3d12_context_new gst_object_ref's the device internally; caller retains ownership.
    return gst_d3d12_context_new(GST_D3D12_DEVICE_CAST(device));
}

}  // namespace

GstContextBridgeBase& GstD3D12ContextBridge::instance()
{
    return s_bridge;
}

GstD3D12Device* GstD3D12ContextBridge::currentDevice()
{
    return GST_D3D12_DEVICE_CAST(s_bridge.refDevice());
}

#endif  // QGC_HAS_GST_D3D_GPU_PATH
