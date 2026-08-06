#include "GstD3D12ContextBridge.h"

#include "GstD3DContextBridgeCommon.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <gst/d3d12/gstd3d12.h>
#include <rhi/qrhi.h>  // QRhi::Implementation enum only; native handles come from the snapshot

#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"

QGC_LOGGING_CATEGORY(GstD3D12BridgeLog, "Video.GStreamer.HwBuffers.GstD3D12Bridge")

namespace GstD3D12ContextBridge {
namespace {

GstD3DContextBridgeCommon::BridgeState s_state;

GstObject* createDevice(const QLoggingCategory& cat)
{
    // LUID halves were sign-extended into a 64-bit value matching LARGE_INTEGER::QuadPart when the snapshot was
    // composed.
    const gint64 luid = QGCRhiCapture::deviceSnapshot().adapterLuid.load(std::memory_order_acquire);

    GstD3D12Device* device = gst_d3d12_device_new_for_adapter_luid(luid);
    if (!device) {
        qCWarning(cat) << "gst_d3d12_device_new_for_adapter_luid failed (luid=" << luid << ")";
        return nullptr;
    }

    GstD3DContextBridgeCommon::logAdapterMatch(luid, device, cat, "D3D12");
    return GST_OBJECT(device);
}

GstContext* makeContext(GstObject* device)
{
    // gst_d3d12_context_new gst_object_ref's the device internally; caller retains ownership.
    return gst_d3d12_context_new(GST_D3D12_DEVICE_CAST(device));
}

const GstD3DContextBridgeCommon::BridgeOps s_ops = {
    "D3D12", GST_D3D12_DEVICE_HANDLE_CONTEXT_TYPE, &GstD3D12BridgeLog, int(QRhi::D3D12), &createDevice, &makeContext,
};

struct D3D12BridgeRegistrar
{
    D3D12BridgeRegistrar() { GstD3DContextBridgeCommon::registerBridge(GstD3D12BridgeLog(), &handleSyncMessage, &reset); }
};

static D3D12BridgeRegistrar s_d3d12BridgeRegistrar;

}  // namespace

bool prime()
{
    return GstD3DContextBridgeCommon::prime(s_state, s_ops);
}

GstD3D12Device* currentDevice()
{
    return GST_D3D12_DEVICE_CAST(GstD3DContextBridgeCommon::currentDevice(s_state));
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

}  // namespace GstD3D12ContextBridge

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D12_GPU_PATH
