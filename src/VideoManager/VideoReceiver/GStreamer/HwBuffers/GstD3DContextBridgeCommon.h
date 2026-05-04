#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>

#include <atomic>

#include <gst/gst.h>

class QRhi;

/// Common bookkeeping shared by GstD3D11ContextBridge and GstD3D12ContextBridge.
///
/// The two bridges differ in how they construct the shared GstD3DXDevice
/// (`gst_d3d11_device_wrap` vs `gst_d3d12_device_new_for_adapter_luid`) and in
/// how they hand a context back to the requesting element
/// (`gst_d3d11_handle_set_context` vs `gst_d3d12_context_new` + `set_context`).
/// Everything else — the prime/reset state machine, the QRhi-up-and-correct-
/// backend gate, the NEED_CONTEXT message dispatch, and the first-handoff log
/// — is shared via this header.
namespace GstD3DContextBridgeCommon {

/// Per-bridge state. Both bridges hold one static instance.
struct BridgeState {
    QMutex mutex;
    bool primed = false;
    bool warnedWrongBackend = false;
    std::atomic<bool> loggedFirstHandoff{false};
};

/// Returns the live QRhi if (a) it exists and (b) it matches @p expectedBackend.
/// @p backendName is purely for logging ("D3D11" / "D3D12"). Returns nullptr if
/// QRhi isn't ready yet (caller should retry on next NEED_CONTEXT) or if the
/// backend is wrong (caller logs once via @p state.warnedWrongBackend).
QRhi *checkRhiBackend(BridgeState &state,
                      const QLoggingCategory &cat,
                      int expectedBackend,
                      const char *backendName);

/// Inspects @p message; if it's a NEED_CONTEXT for @p expectedContextType,
/// returns the source element. Otherwise returns nullptr (caller passes the
/// message through). Cheap when the message isn't relevant.
GstElement *matchNeedContext(GstMessage *message, const char *expectedContextType);

/// Logs the first successful element handoff at qCInfo level; subsequent
/// handoffs go to qCDebug.
void logHandoff(BridgeState &state,
                const QLoggingCategory &cat,
                GstElement *element,
                const char *apiName);

/// Reads the `adapter-luid` GObject property from a GstD3D11Device or GstD3D12Device.
/// Returns 0 on null input or if the property read fails (both APIs expose this property
/// since their initial gst-plugins-bad release).
gint64 readAdapterLuid(gpointer device);

/// One-shot LUID compare at prime time. Mismatch = gst wrapped a different physical adapter
/// than QRhi; zero-copy will corrupt at sample time.
void logAdapterMatch(QRhi *rhi, gint64 expectedLuid, gpointer gstDevice,
                     const QLoggingCategory &cat, const char *apiName);

} // namespace GstD3DContextBridgeCommon

#endif // Q_OS_WIN && (QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH)
