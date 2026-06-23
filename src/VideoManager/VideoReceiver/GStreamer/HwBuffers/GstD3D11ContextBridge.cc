#include "GstD3D11ContextBridge.h"
#include "GstContextBridgeRegistry.h"
#include "GstD3DContextBridgeCommon.h"

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include "QGCLoggingCategory.h"

#include <QtCore/QMutexLocker>

#include <QtGui/rhi/qrhi.h>
#include <QtGui/rhi/qrhi_platform.h>

#include <gst/d3d11/gstd3d11.h>

QGC_LOGGING_CATEGORY(GstD3D11BridgeLog, "Video.GStreamer.HwBuffers.GstD3D11Bridge")

namespace GstD3D11ContextBridge {
namespace {

GstD3DContextBridgeCommon::BridgeState s_state;
GstD3D11Device *s_device = nullptr;

bool primeLocked()
{
    if (s_state.primed) return true;

    QRhi *rhi = GstD3DContextBridgeCommon::checkRhiBackend(
        s_state, GstD3D11BridgeLog(), int(QRhi::D3D11), "D3D11");
    if (!rhi) return false;

    auto *handles = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
    if (!handles || !handles->dev) {
        qCWarning(GstD3D11BridgeLog) << "QRhiD3D11NativeHandles missing ID3D11Device*";
        return false;
    }

    // gst_d3d11_device_new_wrapped (renamed from gst_d3d11_device_wrap in 1.28): shared device keeps textures sampleable by QRhi without keyed-mutex transfer.
    s_device = gst_d3d11_device_new_wrapped(static_cast<ID3D11Device *>(handles->dev));
    if (!s_device) {
        qCWarning(GstD3D11BridgeLog) << "gst_d3d11_device_new_wrapped failed";
        return false;
    }
    s_state.primed = true;
    qCInfo(GstD3D11BridgeLog) << "D3D11 bridge primed: shared device =" << s_device;
    // Sign-extend qint32 HighPart so it matches LARGE_INTEGER::QuadPart bit-for-bit.
    const gint64 expectedLuid = (static_cast<gint64>(handles->adapterLuidHigh) << 32)
                                | (static_cast<gint64>(handles->adapterLuidLow) & 0xFFFFFFFFLL);
    GstD3DContextBridgeCommon::logAdapterMatch(rhi, expectedLuid, s_device,
                                                GstD3D11BridgeLog(), "D3D11");
    return true;
}

} // namespace

bool prime()
{
    QMutexLocker lock(&s_state.mutex);
    return primeLocked();
}

GstD3D11Device *currentDevice()
{
    QMutexLocker lock(&s_state.mutex);
    if (!s_device) return nullptr;
    return GST_D3D11_DEVICE_CAST(gst_object_ref(s_device));
}

GstBusSyncReply handleSyncMessage(GstMessage *message)
{
    GstElement *element = GstD3DContextBridgeCommon::matchNeedContext(
        message, GST_D3D11_DEVICE_HANDLE_CONTEXT_TYPE);
    if (!element) {
        return GST_BUS_PASS;
    }

    QMutexLocker lock(&s_state.mutex);
    if (!primeLocked() || !s_device) {
        return GST_BUS_PASS;
    }

    // gst_d3d11_context_new wraps s_device in a GstContext; gst_object_ref's s_device internally.
    GstContext *ctx = gst_d3d11_context_new(s_device);
    if (!ctx) {
        qCWarning(GstD3D11BridgeLog) << "gst_d3d11_context_new failed for element"
                                     << GST_ELEMENT_NAME(element);
        return GST_BUS_PASS;
    }
    gst_element_set_context(element, ctx);
    gst_context_unref(ctx);
    gst_message_unref(message);
    GstD3DContextBridgeCommon::logHandoff(s_state, GstD3D11BridgeLog(), element, "D3D11");
    return GST_BUS_DROP;
}

void reset()
{
    QMutexLocker lock(&s_state.mutex);
    gst_clear_object(&s_device);
    s_state.primed = false;
    s_state.warnedWrongBackend = false;
    qCDebug(GstD3D11BridgeLog) << "D3D11 bridge reset";
}

namespace {
struct D3D11BridgeRegistrar {
    D3D11BridgeRegistrar() {
        GstContextBridgeRegistry::registerBridgeHandler(&GstD3D11ContextBridge::handleSyncMessage);
        GstContextBridgeRegistry::registerResetCallback(&GstD3D11ContextBridge::reset);
    }
};
static D3D11BridgeRegistrar s_d3d11BridgeRegistrar;
} // anonymous namespace

} // namespace GstD3D11ContextBridge

#endif // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
