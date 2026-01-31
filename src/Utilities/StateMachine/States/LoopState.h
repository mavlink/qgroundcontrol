#pragma once

#include "QGCState.h"

#include <functional>

/// A state that iterates over a collection, executing an action for each item.
///
/// The state calls the iterator function for each item, emitting itemProcessed()
/// after each. When all items are processed, it emits advance().
///
/// Example usage:
/// @code
/// QStringList files = {"a.txt", "b.txt", "c.txt"};
/// auto* loopState = new LoopState<QString>("ProcessFiles", &machine, files,
///     [](const QString& file, int index) {
///         qDebug() << "Processing file" << index << ":" << file;
///     });
///
/// loopState->addTransition(loopState, &LoopState<QString>::advance, nextState);
/// @endcode
template<typename T>
class LoopState : public QGCState
{
public:
    using ItemAction = std::function<void(const T& item, int index)>;
    using ItemPredicate = std::function<bool(const T& item, int index)>;

    /// Create a LoopState
    /// @param stateName Name for logging
    /// @param parent Parent state
    /// @param items Collection to iterate over
    /// @param action Action to execute for each item
    LoopState(const QString& stateName, QState* parent,
              const QList<T>& items, ItemAction action)
        : QGCState(stateName, parent)
        , _items(items)
        , _action(std::move(action))
    {
    }

    /// Set a predicate to filter items (only items where predicate returns true are processed)
    void setFilter(ItemPredicate filter) { _filter = std::move(filter); }

    /// Get the current index being processed
    int currentIndex() const { return _currentIndex; }

    /// Get the current item being processed
    T currentItem() const { return _currentIndex >= 0 && _currentIndex < _items.size()
                                   ? _items[_currentIndex] : T(); }

    /// Get the total number of items
    int totalItems() const { return _items.size(); }

    /// Get the number of items processed so far
    int processedCount() const { return _processedCount; }

protected:
    void onEnter() override
    {
        _currentIndex = -1;
        _processedCount = 0;
        _processNextItem();
    }

private:
    void _processNextItem()
    {
        _currentIndex++;

        // Skip filtered items
        while (_currentIndex < _items.size() && _filter && !_filter(_items[_currentIndex], _currentIndex)) {
            _currentIndex++;
        }

        if (_currentIndex >= _items.size()) {
            // All items processed
            emit advance();
            return;
        }

        // Process current item
        if (_action) {
            _action(_items[_currentIndex], _currentIndex);
        }
        _processedCount++;

        // Schedule next item processing (allow event loop to process)
        QMetaObject::invokeMethod(this, &LoopState::_processNextItem, Qt::QueuedConnection);
    }

    QList<T> _items;
    ItemAction _action;
    ItemPredicate _filter;
    int _currentIndex = -1;
    int _processedCount = 0;
};
