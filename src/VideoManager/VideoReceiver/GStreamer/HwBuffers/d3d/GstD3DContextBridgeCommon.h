#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <atomic>
#include <gst/gst.h>

/// Bookkeeping shared by GstD3D11/D3D12ContextBridge (prime/reset state machine, backend gate, NEED_CONTEXT dispatch);
/// device construction differs per bridge.
namespace GstD3DContextBridgeCommon {

/// Per-bridge state. Both bridges hold one static instance.
struct BridgeState
{
    QMutex mutex;
    bool primed = false;
    bool warnedWrongBackend = false;
    std::atomic<bool> loggedFirstHandoff{false};
    GstObject* device = nullptr;  ///< owned wrapped device (GstD3D11Device/GstD3D12Device); reset() clears it.
};

/// Per-API hooks for the shared prime/handoff/reset skeleton — the only parts that differ between the D3D11 and D3D12
/// bridges.
struct BridgeOps
{
    const char* apiName;                                  ///< "D3D11" / "D3D12" — log + diagnostics label.
    const char* contextType;                              ///< GST_D3D1x_DEVICE_HANDLE_CONTEXT_TYPE.
    const QLoggingCategory& (*cat)();                     ///< bridge's logging category accessor.
    int qrhiBackend;                                      ///< int(QRhi::D3D11) / int(QRhi::D3D12).
    GstObject* (*createDevice)(const QLoggingCategory&);  ///< build the wrapped device; nullptr (logged) on failure.
    GstContext* (*makeContext)(GstObject* device);        ///< wrap the device in a GstContext (refs internally).
};

/// True if QGCRhiCapture's snapshot matches @p expectedBackend. Caller MUST hold state.mutex — touches non-atomic
/// warnedWrongBackend.
bool checkSnapshotBackend(BridgeState& state, const QLoggingCategory& cat, int expectedBackend,
                          const char* backendName);

/// Logs the first element handoff at qCInfo; subsequent at qCDebug.
void logHandoff(BridgeState& state, const QLoggingCategory& cat, GstElement* element, const char* apiName);

/// Reads `adapter-luid` from a GstD3D11Device/GstD3D12Device; 0 on null/read failure.
gint64 readAdapterLuid(gpointer device);

/// One-shot LUID compare at prime; mismatch = gst wrapped a different adapter than QRhi (corrupts).
void logAdapterMatch(gint64 expectedLuid, gpointer gstDevice, const QLoggingCategory& cat, const char* apiName);

/// Register a D3D bridge's sync handler + reset callback (the two calls every D3D bridge repeats verbatim).
void registerBridge(const QLoggingCategory& cat, GstBusSyncReply (*handler)(GstMessage*), void (*reset)());

/// Idempotent prime: gate on backend, build the device via @p ops, cache it. Thread-safe.
bool prime(BridgeState& state, const BridgeOps& ops);

/// Transfer-full ref to the cached device (caller unrefs) as a GstObject*, nullptr if not primed.
GstObject* currentDevice(BridgeState& state);

/// Answer a NEED_CONTEXT for @p ops.contextType with the shared device. GST_BUS_DROP when consumed.
GstBusSyncReply handleSyncMessage(BridgeState& state, const BridgeOps& ops, GstMessage* message);

/// Answer a GST_QUERY_CONTEXT for @p ops.contextType with the shared device (sink-bin query path). True when consumed.
bool answerContextQuery(BridgeState& state, const BridgeOps& ops, GstQuery* query);

/// Drop the cached device so the next prime() rebuilds it.
void reset(BridgeState& state, const BridgeOps& ops);

}  // namespace GstD3DContextBridgeCommon

#endif  // Q_OS_WIN && (QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH)
