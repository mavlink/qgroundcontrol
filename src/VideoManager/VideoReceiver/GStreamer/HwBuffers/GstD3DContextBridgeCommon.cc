#include "GstD3DContextBridgeCommon.h"

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))

#include "QGCRhiCapture.h"

#include <rhi/qrhi.h>
#include <glib-object.h>

namespace GstD3DContextBridgeCommon {

// Caller must hold state.mutex — warnedWrongBackend is a plain bool, not atomic.
// Also called from the bus-sync thread; backend()/backendName() are read-only enum/string
// accessors on QRhi and don't touch GPU state, but QRhi is documented single-thread so this
// is "safe by inspection" rather than by API contract — keep the calls limited to these.
QRhi *checkRhiBackend(BridgeState &state,
                      const QLoggingCategory &cat,
                      int expectedBackend,
                      const char *backendName)
{
    QRhi *rhi = QGCRhiCapture::cachedRhi();
    if (!rhi) {
        qCDebug(cat) << "QRhi not yet available; will retry on next NEED_CONTEXT";
        return nullptr;
    }
    if (static_cast<int>(rhi->backend()) != expectedBackend) {
        if (!state.warnedWrongBackend) {
            qCInfo(cat) << "QRhi backend is" << rhi->backendName()
                        << "(not" << backendName << "); bridge inactive";
            state.warnedWrongBackend = true;
        }
        return nullptr;
    }
    return rhi;
}

GstElement *matchNeedContext(GstMessage *message, const char *expectedContextType)
{
    if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_NEED_CONTEXT) {
        return nullptr;
    }
    const gchar *contextType = nullptr;
    if (!gst_message_parse_context_type(message, &contextType) || !contextType) {
        return nullptr;
    }
    if (g_strcmp0(contextType, expectedContextType) != 0) {
        return nullptr;
    }
    return GST_ELEMENT(GST_MESSAGE_SRC(message));
}

void logHandoff(BridgeState &state,
                const QLoggingCategory &cat,
                GstElement *element,
                const char *apiName)
{
    if (!state.loggedFirstHandoff.exchange(true, std::memory_order_relaxed)) {
        qCInfo(cat) << "First" << apiName << "device handoff to element"
                    << GST_ELEMENT_NAME(element);
    } else {
        qCDebug(cat) << "Provided" << apiName << "device context to" << GST_ELEMENT_NAME(element);
    }
}

gint64 readAdapterLuid(gpointer device)
{
    if (!device || !G_IS_OBJECT(device)) return 0;
    gint64 luid = 0;
    g_object_get(G_OBJECT(device), "adapter-luid", &luid, nullptr);
    return luid;
}

void logAdapterMatch(QRhi *rhi, gint64 expectedLuid, gpointer gstDevice,
                     const QLoggingCategory &cat, const char *apiName)
{
    const gint64 actualLuid = readAdapterLuid(gstDevice);
    if (actualLuid != expectedLuid) {
        qCWarning(cat).noquote()
            << apiName << "bridge: gst device LUID mismatch — QRhi LUID="
            << expectedLuid << "but wrapped device LUID=" << actualLuid
            << "(zero-copy will appear corrupt; check NEED_CONTEXT race)";
        return;
    }
    if (!rhi) return;
    const QRhiDriverInfo info = rhi->driverInfo();
    qCInfo(cat).noquote()
        << apiName << "bridge adapter:" << info.deviceName
        << QString::asprintf("(vendorId=0x%04X deviceId=0x%04X type=%d luid=%lld)",
                             unsigned(info.vendorId), unsigned(info.deviceId),
                             int(info.deviceType), static_cast<long long>(expectedLuid));
}

} // namespace GstD3DContextBridgeCommon

#endif // Q_OS_WIN && (QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH)
