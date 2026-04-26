#pragma once

#include <QtCore/QMetaObject>
#include <QtCore/Qt>

#include <atomic>
#include <memory>
#include <utility>

/// Queue a lambda onto `target`'s thread, guarded by a shared `destroyed` flag.
///
/// The sentinel pattern pairs with a QObject destructor that:
///   1. Sets `*destroyed = true` (release).
///   2. Drains the worker thread's event queue (BlockingQueuedConnection).
///   3. Stops and joins the worker thread before `~QObject` runs.
///
/// Under that invariant, any lambda already queued before `*destroyed` was set
/// runs to completion on a live object; any callback that reads `*destroyed`
/// after the store sees `true` and skips the queue push entirely, which
/// synchronously releases any RAII-held refs captured by the lambda.
///
/// `destroyed` is passed by shared_ptr so the queued lambda's copy keeps the
/// flag alive even after the owning object is destroyed and releases its copy.
///
/// Pass ref-owning values as lambda captures by-move to transfer ownership
/// safely across the thread hop.
namespace GstCallbackDispatch {

template <typename Target, typename Fn>
inline void dispatchGuarded(Target* target, const std::shared_ptr<std::atomic<bool>>& destroyed, Fn&& fn)
{
    if (destroyed->load(std::memory_order_acquire)) {
        return;
    }

    QMetaObject::invokeMethod(
        target,
        [destroyed, fn = std::forward<Fn>(fn)]() mutable {
            if (destroyed->load(std::memory_order_acquire)) {
                return;
            }
            fn();
        },
        Qt::QueuedConnection);
}

}  // namespace GstCallbackDispatch
