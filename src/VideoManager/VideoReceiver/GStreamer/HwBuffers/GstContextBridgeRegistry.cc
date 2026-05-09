#include "GstContextBridgeRegistry.h"
#include "QGCLoggingCategory.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <array>
#include <atomic>
#include <mutex>

QGC_LOGGING_CATEGORY(GstContextBridgeRegistryLog, "Video.GStreamer.HwBuffers.GstContextBridgeRegistry")

namespace GstContextBridgeRegistry {

namespace {

constexpr int kMaxHandlers = 8;
// Atomic entries close the write-after-read hazard: dispatch loads each slot with acquire
// ordering, paired with the release store in registerBridgeHandler.
std::array<std::atomic<BridgeHandler>, kMaxHandlers> s_handlers{};
std::atomic<int> s_count{0};
std::array<std::atomic<ResetCallback>, kMaxHandlers> s_resets{};
std::atomic<int> s_resetCount{0};
std::mutex s_registerMutex;

} // namespace

RegistrationHandle registerBridgeHandler(BridgeHandler handler)
{
    if (!handler) return kInvalidHandle;
    std::lock_guard<std::mutex> lock(s_registerMutex);
    const int count = s_count.load(std::memory_order_relaxed);
    // Dedup: same handler twice is a no-op. Also lets a freed slot be reused by a re-register
    // when the original handle was already returned.
    int firstFree = -1;
    for (int i = 0; i < count; ++i) {
        BridgeHandler existing = s_handlers[i].load(std::memory_order_relaxed);
        if (existing == handler) return i;
        if (!existing && firstFree < 0) firstFree = i;
    }
    if (firstFree >= 0) {
        s_handlers[firstFree].store(handler, std::memory_order_release);
        return firstFree;
    }
    if (count >= kMaxHandlers) {
        qCWarning(GstContextBridgeRegistryLog)
            << "Bridge handler limit (" << kMaxHandlers << ") exceeded — dropping registration";
        return kInvalidHandle;
    }
    // Store handler before publishing the new count so dispatchBridges can't observe a count
    // that includes a slot whose handler write isn't yet visible.
    s_handlers[count].store(handler, std::memory_order_release);
    s_count.store(count + 1, std::memory_order_release);
    return count;
}

void unregisterBridgeHandler(RegistrationHandle handle)
{
    if (handle < 0 || handle >= kMaxHandlers) return;
    std::lock_guard<std::mutex> lock(s_registerMutex);
    s_handlers[handle].store(nullptr, std::memory_order_release);
    // Don't shrink s_count: dispatchBridges loops up to count and skips null slots, so leaving
    // count stable preserves slot indices for any other registered handlers.
}

// Handlers run in registration (link) order; first GST_BUS_DROP wins. Bridges must mutually
// exclude on context-type so order doesn't shadow another bridge's intended handoff.
GstBusSyncReply dispatchBridges(GstMessage *message)
{
    const int count = s_count.load(std::memory_order_acquire);
    for (int i = 0; i < count && i < kMaxHandlers; ++i) {
        BridgeHandler h = s_handlers[i].load(std::memory_order_acquire);
        if (h && h(message) == GST_BUS_DROP) {
            return GST_BUS_DROP;
        }
    }
    return GST_BUS_PASS;
}

RegistrationHandle registerResetCallback(ResetCallback callback)
{
    if (!callback) return kInvalidHandle;
    std::lock_guard<std::mutex> lock(s_registerMutex);
    const int count = s_resetCount.load(std::memory_order_relaxed);
    int firstFree = -1;
    for (int i = 0; i < count; ++i) {
        ResetCallback existing = s_resets[i].load(std::memory_order_relaxed);
        if (existing == callback) return i;
        if (!existing && firstFree < 0) firstFree = i;
    }
    if (firstFree >= 0) {
        s_resets[firstFree].store(callback, std::memory_order_release);
        return firstFree;
    }
    if (count >= kMaxHandlers) {
        qCWarning(GstContextBridgeRegistryLog)
            << "Reset callback limit (" << kMaxHandlers << ") exceeded — dropping registration";
        return kInvalidHandle;
    }
    s_resets[count].store(callback, std::memory_order_release);
    s_resetCount.store(count + 1, std::memory_order_release);
    return count;
}

void unregisterResetCallback(RegistrationHandle handle)
{
    if (handle < 0 || handle >= kMaxHandlers) return;
    std::lock_guard<std::mutex> lock(s_registerMutex);
    s_resets[handle].store(nullptr, std::memory_order_release);
}

void resetAllBridges()
{
    const int count = s_resetCount.load(std::memory_order_acquire);
    for (int i = 0; i < count && i < kMaxHandlers; ++i) {
        ResetCallback cb = s_resets[i].load(std::memory_order_acquire);
        if (cb) cb();
    }
}

#ifdef QT_TESTLIB_LIB
void clearForTest()
{
    // Drop cached device/context state in every bridge first; otherwise a test that primes
    // a bridge then re-registers via registerBridgeHandler would observe stale s_primed=true.
    // Done before mutex acquisition because each bridge takes its own lock.
    resetAllBridges();
    std::lock_guard<std::mutex> lock(s_registerMutex);
    // Zero count before the slot stores so a concurrent dispatchBridges can't read a slot
    // we've already nulled while count is still high. Pairs with the count-after-slot store
    // ordering in registerBridgeHandler.
    s_count.store(0, std::memory_order_release);
    s_resetCount.store(0, std::memory_order_release);
    for (auto &slot : s_handlers) slot.store(nullptr, std::memory_order_release);
    for (auto &slot : s_resets) slot.store(nullptr, std::memory_order_release);
}
#endif

} // namespace GstContextBridgeRegistry

#endif // QGC_HAS_GST_GLMEMORY_GPU_PATH || QGC_HAS_GST_D3D11_GPU_PATH || QGC_HAS_GST_D3D12_GPU_PATH
