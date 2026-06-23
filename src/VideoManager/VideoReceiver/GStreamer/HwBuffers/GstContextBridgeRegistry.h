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

/// Register a bridge handler. Called from static initializers before any pipeline starts.
void registerBridgeHandler(BridgeHandler handler);

/// Register a per-bridge reset callback that drops cached devices/contexts.
/// Invoked on QQuickWindow scene-graph teardown to keep wrapped GPU handles
/// from outliving the underlying QRhi.
void registerResetCallback(ResetCallback callback);

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
