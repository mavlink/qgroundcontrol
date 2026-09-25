#pragma once

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include <QtCore/QMetaMethod>
#include <QtCore/QObject>
#include <QtCore/QPointer>

/// Holds an owner's notifications until its outermost operation returns, so observers never run
/// inside an operation that is still changing state.
///
/// Lifetime rule: an observer of these notifications may reconfigure, stop or restart the owner
/// synchronously, but must not delete it; it uses deleteLater(). Owner code relies on this once
/// delivery returns. A synchronous deletion still stops delivery safely, but is reported as a warning.
class GPSNotificationQueue
{
public:
    template <typename Owner>
    explicit GPSNotificationQueue(Owner* owner)
        : _owner(owner)
        , _ownerClass(Owner::staticMetaObject.className())
    {}

    Q_DISABLE_COPY_MOVE(GPSNotificationQueue)

    /// Marks an operation; the outermost scope delivers what the operation queued.
    class Scope
    {
    public:
        explicit Scope(GPSNotificationQueue& queue)
            : _owner(queue._owner)
            , _queue(&queue)
        {
            ++_queue->_depth;
        }

        ~Scope()
        {
            if (_owner && --_queue->_depth == 0) {
                _queue->_deliver();
            }
        }

        Q_DISABLE_COPY_MOVE(Scope)

    private:
        QPointer<QObject> _owner;
        GPSNotificationQueue* _queue;
    };

    /// Replaces a pending notification with the same nonzero key in place; key zero is never coalesced.
    /// Delivers at once outside an operation.
    void post(quintptr key, std::function<void()> notification)
    {
        if (_closed) {
            return;
        }
        const auto pending = key == 0 ? _pending.end()
                                      : std::find_if(_pending.begin(), _pending.end(),
                                                     [key](const auto& entry) { return entry.first == key; });
        if (pending != _pending.end()) {
            pending->second = std::move(notification);
        } else {
            _pending.emplace_back(key, std::move(notification));
        }
        if (_depth == 0) {
            _deliver();
        }
    }

    /// Queues a state-change signal, coalesced with an earlier pending emission of the same signal.
    template <typename Object, typename... Parameters, typename... Arguments>
    void emitSignal(Object* object, void (Object::*signal)(Parameters...), Arguments&&... arguments)
    {
        // Method indices start after QObject's own, so they are never zero.
        post(static_cast<quintptr>(QMetaMethod::fromSignal(signal).methodIndex()),
             [object, signal, ... values = std::forward<Arguments>(arguments)]() { (object->*signal)(values...); });
    }

    /// Drops pending notifications and ignores later ones, for owner teardown.
    void close()
    {
        _closed = true;
        _pending.clear();
    }

private:
    /// Stops once an observer deletes the owner, and reports that violation of the lifetime rule.
    void _deliver();

    QObject* const _owner;
    // Static meta-object data stays valid after the owner is gone.
    const char* const _ownerClass;
    std::vector<std::pair<quintptr, std::function<void()>>> _pending;
    int _depth = 0;
    bool _delivering = false;
    bool _closed = false;
};
