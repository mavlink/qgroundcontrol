#include "GstD3D11ContextBridge.h"

#include "GstD3DContextBridgeCommon.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <d3d11.h>
#include <d3d11_4.h>  // ID3D11Multithread
#include <gst/d3d11/gstd3d11.h>
#include <rhi/qrhi.h>  // QRhi::Implementation enum only; no runtime use across threads

#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"

QGC_LOGGING_CATEGORY(GstD3D11BridgeLog, "Video.GStreamer.HwBuffers.GstD3D11Bridge")

namespace GstD3D11ContextBridge {
namespace {

GstD3DContextBridgeCommon::BridgeState s_state;

GstObject* createDevice(const QLoggingCategory& cat)
{
    // Snapshot populated on the render thread (sceneGraphInitialized); only atomic loads here.
    auto* dev = static_cast<ID3D11Device*>(QGCRhiCapture::deviceSnapshot().d3d11Device.load(std::memory_order_acquire));
    if (!dev) {
        qCWarning(cat) << "QRhi D3D11 snapshot missing ID3D11Device*";
        return nullptr;
    }

    // QRhi creates this device with devFlags=0 (qrhid3d11.cpp) — no multithread protection. GStreamer's streaming
    // thread and the QRhi render thread share its immediate context, so guard it explicitly. Idempotent: a no-op if
    // GStreamer or a future QRhi already enabled it.
    ID3D11DeviceContext* immediate = nullptr;
    dev->GetImmediateContext(&immediate);
    if (immediate) {
        ID3D11Multithread* multithread = nullptr;
        if (SUCCEEDED(immediate->QueryInterface(__uuidof(ID3D11Multithread), reinterpret_cast<void**>(&multithread))) &&
            multithread) {
            multithread->SetMultithreadProtected(TRUE);
            multithread->Release();
        }
        immediate->Release();
    }

    // gst_d3d11_device_new_wrapped: shared device keeps textures QRhi-sampleable without keyed-mutex transfer.
    GstD3D11Device* device = gst_d3d11_device_new_wrapped(dev);
    if (!device) {
        qCWarning(cat) << "gst_d3d11_device_new_wrapped failed";
        return nullptr;
    }

    const gint64 expectedLuid = QGCRhiCapture::deviceSnapshot().adapterLuid.load(std::memory_order_acquire);
    GstD3DContextBridgeCommon::logAdapterMatch(expectedLuid, device, cat, "D3D11");
    return GST_OBJECT(device);
}

GstContext* makeContext(GstObject* device)
{
    // gst_d3d11_context_new gst_object_ref's the device internally; caller retains ownership.
    return gst_d3d11_context_new(GST_D3D11_DEVICE_CAST(device));
}

const GstD3DContextBridgeCommon::BridgeOps s_ops = {
    "D3D11", GST_D3D11_DEVICE_HANDLE_CONTEXT_TYPE, &GstD3D11BridgeLog, int(QRhi::D3D11), &createDevice, &makeContext,
};

struct D3D11BridgeRegistrar
{
    D3D11BridgeRegistrar() { GstD3DContextBridgeCommon::registerBridge(GstD3D11BridgeLog(), &handleSyncMessage, &reset); }
};

static D3D11BridgeRegistrar s_d3d11BridgeRegistrar;

}  // namespace

bool prime()
{
    return GstD3DContextBridgeCommon::prime(s_state, s_ops);
}

GstD3D11Device* currentDevice()
{
    return GST_D3D11_DEVICE_CAST(GstD3DContextBridgeCommon::currentDevice(s_state));
}

GstBusSyncReply handleSyncMessage(GstMessage* message)
{
    return GstD3DContextBridgeCommon::handleSyncMessage(s_state, s_ops, message);
}

bool answerContextQuery(GstQuery* query)
{
    return GstD3DContextBridgeCommon::answerContextQuery(s_state, s_ops, query);
}

void reset()
{
    GstD3DContextBridgeCommon::reset(s_state, s_ops);
}

}  // namespace GstD3D11ContextBridge

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
