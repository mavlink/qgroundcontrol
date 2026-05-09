#include "GstD3DContextBridgeCommon.h"

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))

#include "QGCRhiCapture.h"

#include <glib-object.h>

namespace GstD3DContextBridgeCommon {

// Reads from QGCRhiCapture::deviceSnapshot() — atomic loads, safe from the bus-sync thread.
// The snapshot is populated on the render thread when sceneGraphInitialized fires, so the
// previous "safe by inspection" cross-thread QRhi access is gone.
bool checkSnapshotBackend(BridgeState &state,
                          const QLoggingCategory &cat,
                          int expectedBackend,
                          const char *backendName)
{
    const int backend = QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire);
    if (backend < 0) {
        qCDebug(cat) << "QRhi snapshot not yet populated; will retry on next NEED_CONTEXT";
        return false;
    }
    if (backend != expectedBackend) {
        if (!state.warnedWrongBackend) {
            qCInfo(cat) << "QRhi backend tag is" << backend
                        << "(not" << backendName << "); bridge inactive";
            state.warnedWrongBackend = true;
        }
        return false;
    }
    return true;
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

void logAdapterMatch(gint64 expectedLuid, gpointer gstDevice,
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
    qCInfo(cat).noquote()
        << apiName << "bridge adapter LUID="
        << QString::asprintf("0x%llx", static_cast<long long>(expectedLuid));
}

} // namespace GstD3DContextBridgeCommon

#endif // Q_OS_WIN && (QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH)
