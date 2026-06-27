#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <gst/gst.h>

/// Static registry that fans out GstBus sync messages to every compiled context bridge; each bridge registers at file
/// scope.
namespace GstContextBridgeRegistry {

using BridgeHandler = GstBusSyncReply (*)(GstMessage*);
using ResetCallback = void (*)();

/// Opaque slot handle returned by register*; pass to unregister* to clear that slot. Strong-typed so int indices can't
/// be silently passed where a handle is expected.
enum class RegistrationHandle : int
{
    Invalid = -1
};
inline constexpr RegistrationHandle kInvalidHandle = RegistrationHandle::Invalid;

/// Register a bridge handler (call from static initializers); duplicate handler pointers are deduped.
RegistrationHandle registerBridgeHandler(BridgeHandler handler);

/// Register a reset callback invoked on QRhi teardown to drop cached GPU handles; duplicates are deduped.
RegistrationHandle registerResetCallback(ResetCallback callback);

/// Register a per-backend import-cache reset (DMABuf/D3D/IOSurface/AHB), invoked on GPU device-loss.
/// Separate array from the bridge resets so its capacity covers the non-bridge cache paths too.
RegistrationHandle registerCacheReset(ResetCallback callback);

/// Dispatch message to all registered handlers; returns GST_BUS_DROP on first consumer.
GstBusSyncReply dispatchBridges(GstMessage* message);

/// Invoke every registered reset callback; each bridge's reset() takes its own mutex.
void resetAllBridges();

/// Invoke every registered cache-reset callback; each backend's reset takes its own cache mutex.
void resetAllCaches();

#ifdef QGC_GST_BUILD_TESTING
/// Reset all registrations and call resetAllBridges() so tests start from a clean state.
void clearForTest();
#endif

}  // namespace GstContextBridgeRegistry

#endif  // QGC_HAS_ANY_GPU_PATH
