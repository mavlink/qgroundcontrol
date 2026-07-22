#pragma once

#include <QtCore/QHash>
#include <QtCore/QPersistentModelIndex>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <algorithm>
#include <cstdint>

#include "ObjectListModelBase.h"

class QmlObjectListModel : public ObjectListModelBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    QmlObjectListModel(QObject* parent = nullptr);

    // Overrides from ObjectListModelBase
    int count() const override final;

    bool isEmpty() const override final { return count() == 0; }

    void setDirty(bool dirty) override final;
    void clear() override final;

    QObject* removeOne(const QObject* object) override final { return removeAt(indexOf(object)); }

    bool contains(const QObject* object) override final { return indexOf(object) != -1; }

    void clearAndDeleteContents() override final; ///< Clears the list and calls deleteLater on each entry

    // QmlObjectListModel specific methods
    Q_INVOKABLE QObject* get(int index);
    QObject* operator[](int index);
    const QObject* operator[](int index) const;
    void append(QObject* object); ///< Caller maintains responsibility for object ownership and deletion
    void append(QList<QObject*> objects); ///< Caller maintains responsibility for object ownership and deletion
    QObjectList swapObjectList(const QObjectList& newlist);
    QObject* removeAt(int index);
    void insert(int index, QObject* object);
    void insert(int index, QList<QObject*> objects);
    int indexOf(const QObject* object) const;
    void move(int from, int to);

    template <class T>
    T value(int index) const
    {
        return qobject_cast<T>(at(index));
    }

    QObjectList objectList() const;

    /// Returns the insertion index for value in a list ordered by compare without copying the model contents.
    template <typename Compare>
    int lowerBoundIndex(const QObject* value, Compare compare) const
    {
        const auto iterator = std::lower_bound(_objectList.cbegin(), _objectList.cend(), value,
                                               [&compare](const ObjectSlot& slot, const QObject* candidate) {
                                                   return compare(slot.object.data(), candidate);
                                               });
        return static_cast<int>(iterator - _objectList.cbegin());
    }

protected:
    // Overrides from QAbstractListModel
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool insertRows(int position, int rows, const QModelIndex& index = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex& index = QModelIndex()) override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

private:
    struct ObjectSlot
    {
        QPointer<QObject> object;
        QObject* identity = nullptr;
};

    enum class PendingChangeType : uint8_t
    {
        Insert,
        Remove,
        Move,
        Clear,
        ClearAndDelete,
    };

    struct PendingChange
    {
        PendingChangeType type = PendingChangeType::Clear;
        QList<QPointer<QObject>> objects;
        QList<ObjectSlot> objectSlots;
        QList<QPersistentModelIndex> indexes;
        QPersistentModelIndex firstIndex;
        QPersistentModelIndex secondIndex;
        int fallbackPosition = 0;
        bool append = false;
    };

    QObject* at(int index) const;
    void _addOccurrence(QObject* object, bool dirtyEligible);
    void _removeOccurrence(QObject* object, bool dirtyEligible);
    void _addObjectIndex(QObject* object, int row);
    void _removeObjectIndex(QObject* object, int row);
    void _pruneObjectIndexes(QObject* object);
    void _rebuildObjectIndexes();
    void _addDirtyOccurrence(QObject* object);
    void _removeDirtyOccurrence(QObject* object);
    void _clearTracking();
    void _emitObjectNameChanged(QObject* object);
    bool _emitPendingObjectNameChanges();
    void _scheduleNullCleanup();
    void _removeNullObjects();
    bool _finishMutation(bool countChanged);
    void _insert(int position, const QList<ObjectSlot>& objectSlots);
    void _queueInsert(int position, const QList<ObjectSlot>& objectSlots);
    void _queueRemove(int position, int rows);
    void _queueMove(int from, int to);
    void _applyPendingChanges();
    void _clearAndDeleteContents(const QList<QPointer<QObject>>& additionalObjects);

    QList<ObjectSlot> _objectList;
    QHash<QObject*, int> _objectOccurrences;
    QHash<QObject*, QList<QPersistentModelIndex>> _objectIndexes;
    QHash<QObject*, int> _dirtyOccurrences;
    QHash<QObject*, QMetaObject::Connection> _destroyedConnections;
    QHash<QObject*, QMetaObject::Connection> _objectNameConnections;
    QSet<QObject*> _pendingObjectNameChanges;
    QList<PendingChange> _pendingChanges;
    bool _modelChangeInProgress = false;
    bool _nullCleanupPending = false;
    bool _nullCleanupScheduled = false;
    bool _applyingPendingChanges = false;
};
