#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <gst/gst.h>

/// Static registry that fans out GstBus sync messages to every compiled context bridge.
///
/// Each bridge registers a handler at file scope. _contextSyncDispatch in
/// GstVideoReceiver reduces to a single dispatchBridges() call.
namespace GstContextBridgeRegistry {

using BridgeHandler = GstBusSyncReply(*)(GstMessage *);
using ResetCallback = void(*)();

/// Opaque slot handle returned by register*; pass to unregister* to clear that slot.
/// kInvalidHandle indicates the registry was full or the handler was already registered.
using RegistrationHandle = int;
constexpr RegistrationHandle kInvalidHandle = -1;

/// Register a bridge handler. Called from static initializers before any pipeline starts.
/// Duplicate registrations of the same handler pointer are ignored (returns the existing slot).
RegistrationHandle registerBridgeHandler(BridgeHandler handler);

/// Register a per-bridge reset callback that drops cached devices/contexts.
/// Invoked on QQuickWindow scene-graph teardown to keep wrapped GPU handles
/// from outliving the underlying QRhi. Duplicates dedup as with registerBridgeHandler.
RegistrationHandle registerResetCallback(ResetCallback callback);

/// Clear a previously-registered bridge handler slot. Safe no-op on kInvalidHandle.
/// Slot is reusable by subsequent registerBridgeHandler calls.
void unregisterBridgeHandler(RegistrationHandle handle);

/// Clear a previously-registered reset callback slot. Safe no-op on kInvalidHandle.
void unregisterResetCallback(RegistrationHandle handle);

/// Dispatch message to all registered handlers; returns GST_BUS_DROP on first consumer.
GstBusSyncReply dispatchBridges(GstMessage *message);

/// Invoke every registered reset callback. Safe to call from any thread; each bridge's
/// reset() takes its own mutex.
void resetAllBridges();

#ifdef QT_TESTLIB_LIB
/// Reset handler list so unit tests can register controlled handlers in isolation.
/// Also calls resetAllBridges() so cached state from prior tests doesn't leak across.
void clearForTest();
#endif

} // namespace GstContextBridgeRegistry

#endif // QGC_HAS_GST_GLMEMORY_GPU_PATH || QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH
