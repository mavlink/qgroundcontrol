#include "GstD3D12ContextBridge.h"
#include "GstContextBridgeRegistry.h"
#include "GstD3DContextBridgeCommon.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include "QGCLoggingCategory.h"
#include "QGCRhiCapture.h"

#include <QtCore/QMutexLocker>

#include <rhi/qrhi.h>  // QRhi::Implementation enum only; native handles come from the snapshot

#include <gst/d3d12/gstd3d12.h>

QGC_LOGGING_CATEGORY(GstD3D12BridgeLog, "Video.GStreamer.HwBuffers.GstD3D12Bridge")

namespace GstD3D12ContextBridge {
namespace {

GstD3DContextBridgeCommon::BridgeState s_state;
GstD3D12Device *s_device = nullptr;

bool primeLocked()
{
    if (s_state.primed) return true;

    if (!GstD3DContextBridgeCommon::checkSnapshotBackend(
            s_state, GstD3D12BridgeLog(), int(QRhi::D3D12), "D3D12")) {
        return false;
    }

    // Snapshot is composed on the render thread (sceneGraphInitialized) where the LUID halves
    // are sign-extended into a single 64-bit value matching LARGE_INTEGER::QuadPart.
    const gint64 luid = QGCRhiCapture::deviceSnapshot().adapterLuid.load(std::memory_order_acquire);

    s_device = gst_d3d12_device_new_for_adapter_luid(luid);
    if (!s_device) {
        qCWarning(GstD3D12BridgeLog) << "gst_d3d12_device_new_for_adapter_luid failed (luid=" << luid << ")";
        return false;
    }
    s_state.primed = true;
    qCInfo(GstD3D12BridgeLog) << "D3D12 bridge primed: shared device =" << s_device
                              << "luid=" << luid;
    GstD3DContextBridgeCommon::logAdapterMatch(luid, s_device,
                                                GstD3D12BridgeLog(), "D3D12");
    return true;
}

} // namespace

bool prime()
{
    QMutexLocker lock(&s_state.mutex);
    return primeLocked();
}

GstD3D12Device *currentDevice()
{
    QMutexLocker lock(&s_state.mutex);
    if (!s_device) return nullptr;
    return GST_D3D12_DEVICE_CAST(gst_object_ref(s_device));
}

GstBusSyncReply handleSyncMessage(GstMessage *message)
{
    GstElement *element = GstD3DContextBridgeCommon::matchNeedContext(
        message, GST_D3D12_DEVICE_HANDLE_CONTEXT_TYPE);
    if (!element) {
        return GST_BUS_PASS;
    }

    QMutexLocker lock(&s_state.mutex);
    if (!primeLocked() || !s_device) {
        return GST_BUS_PASS;
    }

    // gst_d3d12_context_new internally gst_object_ref's s_device; caller retains ownership.
    GstContext *ctx = gst_d3d12_context_new(s_device);
    if (!ctx) {
        qCWarning(GstD3D12BridgeLog) << "gst_d3d12_context_new failed for element"
                                     << GST_ELEMENT_NAME(element);
        return GST_BUS_PASS;
    }
    gst_element_set_context(element, ctx);
    gst_context_unref(ctx);
    gst_message_unref(message);

    GstD3DContextBridgeCommon::logHandoff(s_state, GstD3D12BridgeLog(), element, "D3D12");
    return GST_BUS_DROP;
}

void reset()
{
    QMutexLocker lock(&s_state.mutex);
    gst_clear_object(&s_device);
    s_state.primed = false;
    s_state.warnedWrongBackend = false;
    qCDebug(GstD3D12BridgeLog) << "D3D12 bridge reset";
}

namespace {
struct D3D12BridgeRegistrar {
    D3D12BridgeRegistrar() {
        GstContextBridgeRegistry::registerBridgeHandler(&GstD3D12ContextBridge::handleSyncMessage);
        GstContextBridgeRegistry::registerResetCallback(&GstD3D12ContextBridge::reset);
    }
};
static D3D12BridgeRegistrar s_d3d12BridgeRegistrar;
} // namespace

} // namespace GstD3D12ContextBridge

#endif // Q_OS_WIN && QGC_HAS_GST_D3D12_GPU_PATH
