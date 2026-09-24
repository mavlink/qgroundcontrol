#pragma once

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include <QtCore/QMetaMethod>
#include <QtCore/QObject>
#include <QtCore/QPointer>

/// Holds an owner's notifications until its outermost operation returns, so observers never run
/// inside an operation that is still changing state. Observers may re-enter or delete the owner
/// while notifications are delivered; delivery stops once the owner is gone.
class GPSNotificationQueue
{
public:
    explicit GPSNotificationQueue(QObject* owner)
        : _owner(owner)
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

    /// Queues an event signal; every emission is delivered.
    template <typename Object, typename... Parameters, typename... Arguments>
    void emitEvent(Object* object, void (Object::*signal)(Parameters...), Arguments&&... arguments)
    {
        post(0, [object, signal, ... values = std::forward<Arguments>(arguments)]() { (object->*signal)(values...); });
    }

    /// Drops pending notifications and ignores later ones, for owner teardown.
    void close()
    {
        _closed = true;
        _pending.clear();
    }

private:
    void _deliver()
    {
        if (_delivering) {
            return;
        }
        const QPointer<QObject> owner(_owner);
        _delivering = true;
        while (!_pending.empty()) {
            auto notification = std::move(_pending.front().second);
            _pending.erase(_pending.begin());
            notification();
            if (!owner) {
                return;
            }
        }
        _delivering = false;
    }

    QObject* const _owner;
    std::vector<std::pair<quintptr, std::function<void()>>> _pending;
    int _depth = 0;
    bool _delivering = false;
    bool _closed = false;
};
