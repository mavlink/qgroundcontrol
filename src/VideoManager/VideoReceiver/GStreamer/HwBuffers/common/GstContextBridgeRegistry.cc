#include "GstContextBridgeRegistry.h"

#include "QGCLoggingCategory.h"

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <QtCore/QMutex>
#include <array>
#include <atomic>

QGC_LOGGING_CATEGORY(GstContextBridgeRegistryLog, "Video.GStreamer.HwBuffers.GstContextBridgeRegistry")

namespace GstContextBridgeRegistry {

namespace {

// Arrays are write-once at static-init (happen-before any GstBus thread), so dispatch reads are lock-free; mutex only
// serializes concurrent registrations. unregister is NOT safe with concurrent dispatch.
// One slot per compiled GPU path that registers a bridge; deriving the cap from the path macros (not a fixed count)
// makes a silent drop impossible.
constexpr int kMaxBridges = 0
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    + 1
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    + 1
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    + 1
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    + 1
#endif
    ;
// Cache-reset participants: the import-cache-owning paths (NOT the bridge set). Sized from its own
// macro list so adding a cache path without a slot is a compile-time array overflow, never a silent drop.
constexpr int kMaxCacheResets = 0
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    + 1
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    + 1
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    + 1
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    + 1
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    + 1
#endif
    ;
QMutex s_mutex;
std::array<BridgeHandler, kMaxBridges> s_handlers{};
std::atomic<int> s_handlerCount{0};
std::array<ResetCallback, kMaxBridges> s_resets{};
std::atomic<int> s_resetCount{0};
std::array<ResetCallback, (kMaxCacheResets > 0 ? kMaxCacheResets : 1)> s_cacheResets{};
std::atomic<int> s_cacheResetCount{0};

}  // namespace

RegistrationHandle registerBridgeHandler(BridgeHandler handler)
{
    if (handler == nullptr) {
        return kInvalidHandle;
    }
    QMutexLocker lock(&s_mutex);
    const int count = s_handlerCount.load(std::memory_order_relaxed);
    for (int i = 0; i < count; ++i) {
        if (s_handlers[i] == handler)
            return static_cast<RegistrationHandle>(i);
    }
    if (count >= kMaxBridges) {
        qCWarning(GstContextBridgeRegistryLog) << "bridge handler registry full (kMaxBridges=" << kMaxBridges
                                               << "); handler will not receive GstBus messages";
        return kInvalidHandle;
    }
    s_handlers[count] = handler;
    s_handlerCount.store(count + 1, std::memory_order_release);
    return static_cast<RegistrationHandle>(count);
}

RegistrationHandle registerResetCallback(ResetCallback callback)
{
    if (callback == nullptr) {
        return kInvalidHandle;
    }
    QMutexLocker lock(&s_mutex);
    const int count = s_resetCount.load(std::memory_order_relaxed);
    for (int i = 0; i < count; ++i) {
        if (s_resets[i] == callback)
            return static_cast<RegistrationHandle>(i);
    }
    if (count >= kMaxBridges) {
        qCWarning(GstContextBridgeRegistryLog) << "reset callback registry full (kMaxBridges=" << kMaxBridges
                                               << "); callback will not run on QRhi teardown";
        return kInvalidHandle;
    }
    s_resets[count] = callback;
    s_resetCount.store(count + 1, std::memory_order_release);
    return static_cast<RegistrationHandle>(count);
}

RegistrationHandle registerCacheReset(ResetCallback callback)
{
    if (callback == nullptr) {
        return kInvalidHandle;
    }
    QMutexLocker lock(&s_mutex);
    const int count = s_cacheResetCount.load(std::memory_order_relaxed);
    for (int i = 0; i < count; ++i) {
        if (s_cacheResets[i] == callback)
            return static_cast<RegistrationHandle>(i);
    }
    if (count >= kMaxCacheResets) {
        qCWarning(GstContextBridgeRegistryLog) << "cache-reset registry full (kMaxCacheResets=" << kMaxCacheResets
                                               << "); callback will not run on GPU device-loss";
        return kInvalidHandle;
    }
    s_cacheResets[count] = callback;
    s_cacheResetCount.store(count + 1, std::memory_order_release);
    return static_cast<RegistrationHandle>(count);
}

// First GST_BUS_DROP wins; bridges must differ on context-type so no bridge shadows another.
GstBusSyncReply dispatchBridges(GstMessage* message)
{
    // Registry lock must not be held across a bridge callout — each handler takes its own per-bridge mutex.
    const int count = s_handlerCount.load(std::memory_order_acquire);
    for (int i = 0; i < count; ++i) {
        const BridgeHandler h = s_handlers[i];
        if (h && (h(message) == GST_BUS_DROP))
            return GST_BUS_DROP;
    }
    return GST_BUS_PASS;
}

void resetAllBridges()
{
    const int count = s_resetCount.load(std::memory_order_acquire);
    for (int i = 0; i < count; ++i) {
        const ResetCallback cb = s_resets[i];
        if (cb)
            cb();
    }
}

void resetAllCaches()
{
    // No registry lock across the callout: each cache reset takes its own per-backend mutex (or is a
    // lock-free atomic-store deferred flag). Array is write-once at static-init (happens-before any
    // GstBus thread), so iteration is lock-free.
    const int count = s_cacheResetCount.load(std::memory_order_acquire);
    for (int i = 0; i < count; ++i) {
        const ResetCallback cb = s_cacheResets[i];
        if (cb)
            cb();
    }
}

#ifdef QGC_GST_BUILD_TESTING
void clearForTest()
{
    resetAllBridges();
    resetAllCaches();
    QMutexLocker lock(&s_mutex);
    s_handlers.fill(nullptr);
    s_resets.fill(nullptr);
    s_cacheResets.fill(nullptr);
    s_handlerCount.store(0, std::memory_order_release);
    s_resetCount.store(0, std::memory_order_release);
    s_cacheResetCount.store(0, std::memory_order_release);
}
#endif

}  // namespace GstContextBridgeRegistry

#endif  // QGC_HAS_ANY_GPU_PATH
