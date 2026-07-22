#include "QmlObjectListModel.h"

#include <QtCore/QMetaMethod>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtQml/QQmlEngine>
#include <algorithm>
#include <functional>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QmlObjectListModelLog, "API.QmlObjectListModel")

namespace {

constexpr const char* kDirtyChangedSignature = "dirtyChanged(bool)";
constexpr const char* kChildDirtyChangedSlotSignature = "_childDirtyChanged(bool)";

QMetaMethod childDirtyChangedSlot()
{
    static const QMetaMethod slot = []() {
        const QMetaObject* metaObject = &ObjectListModelBase::staticMetaObject;
        const int slotIndex = metaObject->indexOfSlot(kChildDirtyChangedSlotSignature);
        if (slotIndex < 0) {
            qCCritical(QmlObjectListModelLog) << "Slot signature mismatch:" << kChildDirtyChangedSlotSignature;
            return QMetaMethod();
        }
        return metaObject->method(slotIndex);
    }();
    return slot;
}

QMetaMethod dirtyChangedSignal(const QObject* object)
{
    if (!object) {
        return QMetaMethod();
    }

    const int signalIndex = object->metaObject()->indexOfSignal(kDirtyChangedSignature);
    return (signalIndex >= 0) ? object->metaObject()->method(signalIndex) : QMetaMethod();
}

void connectDirtyChangedIfAvailable(QObject* object, ObjectListModelBase* receiver)
{
    const QMetaMethod signal = dirtyChangedSignal(object);
    const QMetaMethod slot = childDirtyChangedSlot();
    if (signal.isValid() && slot.isValid()) {
        (void) QObject::connect(object, signal, receiver, slot, Qt::UniqueConnection);
    }
}

void disconnectDirtyChangedIfAvailable(QObject* object, ObjectListModelBase* receiver)
{
    const QMetaMethod signal = dirtyChangedSignal(object);
    const QMetaMethod slot = childDirtyChangedSlot();
    if (signal.isValid() && slot.isValid()) {
        QObject::disconnect(object, signal, receiver, slot);
    }
}

}  // namespace

QmlObjectListModel::QmlObjectListModel(QObject* parent) : ObjectListModelBase(parent)
{
    // Keep the lookup cache current before external rowsInserted observers run.
    (void) connect(this, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex&, int first, int last) {
        for (int row = first; row <= last; ++row) {
            _addObjectIndex(at(row), row);
        }
    });
    (void) connect(this, &QAbstractItemModel::modelReset, this, &QmlObjectListModel::_rebuildObjectIndexes);
}

QObject* QmlObjectListModel::get(int index)
{
    if (index < 0 || index >= _objectList.size()) {
        qCWarning(QmlObjectListModelLog) << "InternalError: Invalid index - index:count" << index << _objectList.size()
                                         << this;
        return nullptr;
    }
    return at(index);
}

int QmlObjectListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : _objectList.size();
}

QVariant QmlObjectListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() < 0 || index.row() >= _objectList.size()) {
        return QVariant();
    }

    if (role == ObjectRole) {
        return QVariant::fromValue(at(index.row()));
    } else if (role == TextRole) {
        const QObject* const object = at(index.row());
        return object ? QVariant::fromValue(object->objectName()) : QVariant();
    } else {
        return QVariant();
    }
}

bool QmlObjectListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || (index.model() != this) || (index.row() < 0) || (index.row() >= _objectList.size()) ||
        (role != ObjectRole)) {
        return false;
    }

    QObject* const object = value.value<QObject*>();
    if (_modelChangeInProgress) {
        return false;
    }

    QObject* const previousObject = at(index.row());
    if (previousObject == object) {
        return true;
    }

    const bool dirtyEligible = !_skipDirtyFirstItem || (index.row() != 0);
    const QPointer<QObject> guardedObject(object);
    _modelChangeInProgress = true;
    _removeObjectIndex(previousObject, index.row());
    _removeOccurrence(previousObject, dirtyEligible);
    _objectList.replace(index.row(), ObjectSlot{guardedObject, guardedObject.data()});
    _addOccurrence(guardedObject, dirtyEligible);
    _addObjectIndex(guardedObject, index.row());
    const QPointer<QmlObjectListModel> self(this);
    emit dataChanged(index, index, {ObjectRole, TextRole});
    if (!self) {
        return true;
    }
    setDirty(true);
    if (self) {
        (void) _finishMutation(false);
    }
    return true;
}

bool QmlObjectListModel::insertRows(int position, int rows, const QModelIndex& parent)
{
    if (parent.isValid() || (position < 0) || (position > _objectList.size()) || (rows <= 0)) {
        qCWarning(QmlObjectListModelLog) << "Invalid position - position:count" << position << _objectList.size()
                                         << this;
        return false;
    }

    if (_modelChangeInProgress) {
        return false;
    }

    const QPointer<QObject> previousFirst((_skipDirtyFirstItem && (position == 0)) ? at(0) : nullptr);
    _modelChangeInProgress = true;
    const QPointer<QmlObjectListModel> self(this);
    if (_resetModelNestingCount == 0) {
        beginInsertRows(QModelIndex(), position, position + rows - 1);
        if (!self) {
            return false;
        }
    }
    for (int row = 0; row < rows; ++row) {
        _objectList.insert(position + row, ObjectSlot{});
    }
    if (_resetModelNestingCount == 0) {
        endInsertRows();
        if (!self) {
            return true;
    }
    }
    if (previousFirst) {
        _addDirtyOccurrence(previousFirst.data());
    }
    setDirty(true);
    if (self) {
        (void) _finishMutation(true);
    }

    return true;
}

bool QmlObjectListModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    if (parent.isValid() || (position < 0) || (position >= _objectList.size()) || (rows <= 0)) {
        qCWarning(QmlObjectListModelLog) << "Invalid position - position:count" << position << _objectList.size()
                                         << this;
        return false;
    }
    if (position + rows > _objectList.size()) {
        qCWarning(QmlObjectListModelLog) << "Invalid rows - position:rows:count" << position << rows
                                         << _objectList.size() << this;
        return false;
    }

    if (_modelChangeInProgress) {
        return false;
    }

    QList<std::pair<QObject*, bool>> removedObjects;
    removedObjects.reserve(rows);
    for (int row = position; row < (position + rows); ++row) {
        removedObjects.append({at(row), !_skipDirtyFirstItem || (row != 0)});
    }

    _modelChangeInProgress = true;
    const QPointer<QmlObjectListModel> self(this);
    if (_resetModelNestingCount == 0) {
        beginRemoveRows(QModelIndex(), position, position + rows - 1);
        if (!self) {
            return false;
    }
    }
    for (const auto& [object, dirtyEligible] : std::as_const(removedObjects)) {
        _removeOccurrence(object, dirtyEligible);
    }
    _objectList.remove(position, rows);
    if (_skipDirtyFirstItem && (position == 0) && !_objectList.isEmpty()) {
        _removeDirtyOccurrence(at(0));
    }
    if (_resetModelNestingCount == 0) {
        endRemoveRows();
        if (!self) {
            return true;
        }
    }
    for (const auto& removedObject : std::as_const(removedObjects)) {
        _pruneObjectIndexes(removedObject.first);
    }

    setDirty(true);
    if (self) {
        (void) _finishMutation(true);
    }

    return true;
}

void QmlObjectListModel::move(int from, int to)
{
    if ((from < 0) || (from >= count()) || (to < 0) || (to >= count()) || (from == to)) {
        return;
        }
    if (_modelChangeInProgress) {
        _queueMove(from, to);
        return;
    }

    const QPointer<QObject> oldFirst(_skipDirtyFirstItem ? at(0) : nullptr);
    const int destinationChild = (to > from) ? (to + 1) : to;
    _modelChangeInProgress = true;
    const QPointer<QmlObjectListModel> self(this);
    const bool notifyMove = _resetModelNestingCount == 0;
    if (notifyMove) {
        if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), destinationChild)) {
            if (self) {
                (void) _finishMutation(false);
            }
        return;
        }
        if (!self) {
        return;
        }
    }
        _objectList.move(from, to);
    if (_skipDirtyFirstItem) {
        QObject* const newFirst = at(0);
        if (oldFirst.data() != newFirst) {
            _addDirtyOccurrence(oldFirst.data());
            _removeDirtyOccurrence(newFirst);
        }
    }
    if (notifyMove) {
        endMoveRows();
        if (!self) {
        return;
        }
    }
    setDirty(true);
    if (self) {
        (void) _finishMutation(false);
    }
}

QObject* QmlObjectListModel::operator[](int index)
{
    return at(index);
}

const QObject* QmlObjectListModel::operator[](int index) const
{
    return at(index);
}

void QmlObjectListModel::clear()
{
    if (_modelChangeInProgress) {
        PendingChange change;
        change.type = PendingChangeType::Clear;
        _pendingChanges.append(std::move(change));
        return;
    }
    if (_objectList.isEmpty()) {
        return;
    }

    _modelChangeInProgress = true;
    const QPointer<QmlObjectListModel> self(this);
    beginResetModel();
    if (!self) {
        return;
    }
    _clearTracking();
    _objectList.clear();
    endResetModel();
    if (!self) {
        return;
    }
    _nullCleanupPending = false;
    setDirty(true);
    if (self) {
        (void) _finishMutation(false);
    }
}

QObject* QmlObjectListModel::removeAt(int i)
{
    if ((i < 0) || (i >= _objectList.size())) {
        qCWarning(QmlObjectListModelLog) << "Invalid index - index:count" << i << _objectList.size() << this;
        return nullptr;
        }

    const QPointer<QObject> removedObject(at(i));
    if (_modelChangeInProgress) {
        _queueRemove(i, 1);
        return nullptr;
    }
    (void) removeRows(i, 1);
    return removedObject.data();
}

void QmlObjectListModel::insert(int i, QObject* object)
{
    insert(i, QList<QObject*>{object});
}

void QmlObjectListModel::insert(int i, QList<QObject*> objects)
{
    if (i < 0 || i > _objectList.size()) {
        qCWarning(QmlObjectListModelLog) << "Invalid index - index:count" << i << _objectList.size() << this;
        return;
    }

    if (objects.isEmpty()) {
        return;
    }

    QList<ObjectSlot> objectSlots;
    objectSlots.reserve(objects.size());
    for (QObject* const object : std::as_const(objects)) {
        objectSlots.append(ObjectSlot{.object = object, .identity = object});
    }
    if (_modelChangeInProgress) {
        _queueInsert(i, objectSlots);
        return;
        }

    _insert(i, objectSlots);
    }

void QmlObjectListModel::_insert(int position, const QList<ObjectSlot>& objectSlots)
{
    if (objectSlots.isEmpty()) {
        return;
    }

    const QPointer<QObject> previousFirst((_skipDirtyFirstItem && (position == 0)) ? at(0) : nullptr);
    _modelChangeInProgress = true;
    const QPointer<QmlObjectListModel> self(this);
    if (_resetModelNestingCount == 0) {
        beginInsertRows(QModelIndex(), position, position + static_cast<int>(objectSlots.size()) - 1);
        if (!self) {
        return;
        }
    }
    int insertionRow = position;
    for (const ObjectSlot& objectSlot : objectSlots) {
        _objectList.insert(insertionRow, objectSlot);
        _addOccurrence(objectSlot.object, !_skipDirtyFirstItem || (insertionRow != 0));
        _nullCleanupPending = _nullCleanupPending || (!objectSlot.object && (objectSlot.identity != nullptr));
        ++insertionRow;
    }
    if (previousFirst) {
        _addDirtyOccurrence(previousFirst.data());
    }
    if (_resetModelNestingCount == 0) {
        endInsertRows();
        if (!self) {
            return;
        }
    } else {
        for (int row = position; row < insertionRow; ++row) {
            _addObjectIndex(at(row), row);
        }
    }
    setDirty(true);
    if (self) {
        (void) _finishMutation(true);
    }
}

void QmlObjectListModel::append(QObject* object)
{
    insert(_objectList.size(), object);
}

void QmlObjectListModel::append(QList<QObject*> objects)
{
    insert(_objectList.size(), objects);
}

QObjectList QmlObjectListModel::swapObjectList(const QObjectList& newlist)
{
    if (_modelChangeInProgress) {
        qCWarning(QmlObjectListModelLog) << "Cannot swap object list during a model change" << this;
        return {};
    }

    QList<ObjectSlot> guardedNewList;
    guardedNewList.reserve(newlist.size());
    for (QObject* const object : newlist) {
        guardedNewList.append(ObjectSlot{.object = object, .identity = object});
    }

    QList<QPointer<QObject>> guardedOldList;
    guardedOldList.reserve(_objectList.size());
    for (const ObjectSlot& slot : std::as_const(_objectList)) {
        guardedOldList.append(slot.object);
    }
    _modelChangeInProgress = true;
    const QPointer<QmlObjectListModel> self(this);
    beginResetModel();
    if (!self) {
        return {};
    }
    _clearTracking();
    _objectList.clear();
    _objectList.reserve(guardedNewList.size());
    for (int row = 0; row < guardedNewList.size(); ++row) {
        const ObjectSlot& objectSlot = guardedNewList.at(row);
        _objectList.append(objectSlot);
        _addOccurrence(objectSlot.object, !_skipDirtyFirstItem || (row != 0));
        _nullCleanupPending = _nullCleanupPending || (!objectSlot.object && (objectSlot.identity != nullptr));
    }
    endResetModel();
    if (!self) {
        return {};
    }
    setDirty(true);
    if (self) {
        (void) _finishMutation(false);
    }

    QObjectList oldlist;
    oldlist.reserve(guardedOldList.size());
    for (const QPointer<QObject>& object : std::as_const(guardedOldList)) {
        oldlist.append(object.data());
    }
    return oldlist;
}

int QmlObjectListModel::count() const
{
    return rowCount();
}

void QmlObjectListModel::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        const QPointer<QmlObjectListModel> self(this);
        if (!dirty) {
            // Need to clear dirty from all children
            QList<QPointer<QObject>> trackedObjects;
            trackedObjects.reserve(_objectOccurrences.size());
            for (QObject* const object : _objectOccurrences.keys()) {
                trackedObjects.append(object);
            }
            for (const QPointer<QObject>& object : std::as_const(trackedObjects)) {
                if (object && object->property("dirty").isValid()) {
                    object->setProperty("dirty", false);
                    if (!self) {
        return;
                    }
                }
            }
        }
        emit dirtyChanged(_dirty);
    }
}

void QmlObjectListModel::clearAndDeleteContents()
{
    if (_modelChangeInProgress) {
        PendingChange change;
        change.type = PendingChangeType::ClearAndDelete;
        change.objects.reserve(_objectList.size());
        for (const ObjectSlot& slot : std::as_const(_objectList)) {
            change.objects.append(slot.object);
    }
        _pendingChanges.append(std::move(change));
        return;
    }

    _clearAndDeleteContents({});
}

QObject* QmlObjectListModel::at(int index) const
{
    return ((index >= 0) && (index < _objectList.size())) ? _objectList.at(index).object.data() : nullptr;
}

int QmlObjectListModel::indexOf(const QObject* object) const
{
    if (!object) {
        for (int row = 0; row < _objectList.size(); ++row) {
            if (!at(row)) {
                return row;
            }
        }
        return -1;
    }

    int firstRow = _objectList.size();
    const auto indexesIt = _objectIndexes.constFind(const_cast<QObject*>(object));
    if (indexesIt != _objectIndexes.cend()) {
        for (const QPersistentModelIndex& objectIndex : indexesIt.value()) {
            if (objectIndex.isValid() && (objectIndex.model() == this) && (at(objectIndex.row()) == object)) {
                firstRow = (std::min) (firstRow, objectIndex.row());
            }
        }
    }
    return (firstRow < _objectList.size()) ? firstRow : -1;
}

QObjectList QmlObjectListModel::objectList() const
{
    QObjectList objects;
    objects.reserve(_objectList.size());
    for (const ObjectSlot& slot : _objectList) {
        objects.append(slot.object.data());
    }
    return objects;
}

void QmlObjectListModel::_addOccurrence(QObject* object, bool dirtyEligible)
{
    if (!object) {
        return;
    }

    int& occurrenceCount = _objectOccurrences[object];
    if (occurrenceCount == 0) {
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
        _objectNameConnections.insert(object, connect(object, &QObject::objectNameChanged, this, [this, object]() {
                                          if (_modelChangeInProgress) {
                                              _pendingObjectNameChanges.insert(object);
    } else {
                                              _emitObjectNameChanged(object);
                                          }
                                      }));
        _destroyedConnections.insert(object, connect(object, &QObject::destroyed, this, [this, object]() {
                                         _objectOccurrences.remove(object);
                                         _objectIndexes.remove(object);
                                         _dirtyOccurrences.remove(object);
                                         _destroyedConnections.remove(object);
                                         _objectNameConnections.remove(object);
                                         _pendingObjectNameChanges.remove(object);
                                         _nullCleanupPending = true;
                                         _scheduleNullCleanup();
                                     }));
    }
    ++occurrenceCount;
    if (dirtyEligible) {
        _addDirtyOccurrence(object);
    }
}

void QmlObjectListModel::_removeOccurrence(QObject* object, bool dirtyEligible)
{
    if (!object) {
        return;
    }

    if (dirtyEligible) {
        _removeDirtyOccurrence(object);
    }
    auto occurrenceIt = _objectOccurrences.find(object);
    if (occurrenceIt == _objectOccurrences.end()) {
        return;
    }
    if (occurrenceIt.value() > 1) {
        --occurrenceIt.value();
        return;
    }

    _objectOccurrences.erase(occurrenceIt);
    _objectIndexes.remove(object);
    _dirtyOccurrences.remove(object);
    (void) disconnect(_destroyedConnections.take(object));
    (void) disconnect(_objectNameConnections.take(object));
    _pendingObjectNameChanges.remove(object);
}

void QmlObjectListModel::_addObjectIndex(QObject* object, int row)
{
    if (object && (row >= 0) && (row < _objectList.size())) {
        _objectIndexes[object].append(QPersistentModelIndex(index(row, 0)));
    }
}

void QmlObjectListModel::_removeObjectIndex(QObject* object, int row)
{
    auto indexesIt = _objectIndexes.find(object);
    if (indexesIt == _objectIndexes.end()) {
        return;
    }

    QList<QPersistentModelIndex>& indexes = indexesIt.value();
    for (auto indexIt = indexes.begin(); indexIt != indexes.end();) {
        if (!indexIt->isValid() || (indexIt->row() == row)) {
            indexIt = indexes.erase(indexIt);
        } else {
            ++indexIt;
        }
    }
    if (indexes.isEmpty()) {
        _objectIndexes.erase(indexesIt);
    }
}

void QmlObjectListModel::_pruneObjectIndexes(QObject* object)
{
    auto indexesIt = _objectIndexes.find(object);
    if (indexesIt == _objectIndexes.end()) {
        return;
    }

    QList<QPersistentModelIndex>& indexes = indexesIt.value();
    indexes.removeIf([this, object](const QPersistentModelIndex& objectIndex) {
        return !objectIndex.isValid() || (objectIndex.model() != this) || (at(objectIndex.row()) != object);
    });
    if (indexes.isEmpty()) {
        _objectIndexes.erase(indexesIt);
    }
}

void QmlObjectListModel::_rebuildObjectIndexes()
{
    _objectIndexes.clear();
    for (int row = 0; row < _objectList.size(); ++row) {
        _addObjectIndex(at(row), row);
    }
}

void QmlObjectListModel::_addDirtyOccurrence(QObject* object)
{
    if (!object) {
        return;
    }

    int& dirtyCount = _dirtyOccurrences[object];
    if (dirtyCount == 0) {
            connectDirtyChangedIfAvailable(object, this);
    }
    ++dirtyCount;
}

void QmlObjectListModel::_removeDirtyOccurrence(QObject* object)
{
    if (!object) {
        return;
    }

    auto dirtyIt = _dirtyOccurrences.find(object);
    if (dirtyIt == _dirtyOccurrences.end()) {
        return;
    }
    if (dirtyIt.value() > 1) {
        --dirtyIt.value();
        return;
    }

    _dirtyOccurrences.erase(dirtyIt);
    disconnectDirtyChangedIfAvailable(object, this);
}

void QmlObjectListModel::_clearTracking()
{
    QList<QPointer<QObject>> dirtyObjects;
    dirtyObjects.reserve(_dirtyOccurrences.size());
    for (QObject* const object : _dirtyOccurrences.keys()) {
        dirtyObjects.append(object);
    }
    for (const QPointer<QObject>& object : std::as_const(dirtyObjects)) {
    if (object) {
            disconnectDirtyChangedIfAvailable(object, this);
        }
    }
    for (const QMetaObject::Connection& connection : std::as_const(_destroyedConnections)) {
        (void) disconnect(connection);
    }
    for (const QMetaObject::Connection& connection : std::as_const(_objectNameConnections)) {
        (void) disconnect(connection);
    }
    _destroyedConnections.clear();
    _objectNameConnections.clear();
    _pendingObjectNameChanges.clear();
    _objectOccurrences.clear();
    _objectIndexes.clear();
    _dirtyOccurrences.clear();
}

void QmlObjectListModel::_emitObjectNameChanged(QObject* object)
{
    if (!object || !_objectOccurrences.contains(object)) {
        return;
    }

    QList<QPersistentModelIndex> changedIndexes = _objectIndexes.value(object);
    std::sort(changedIndexes.begin(), changedIndexes.end(), [](const QPersistentModelIndex& left,
                                                               const QPersistentModelIndex& right) {
        return left.row() < right.row();
    });

    const QPointer<QmlObjectListModel> self(this);
    for (const QPersistentModelIndex& changedIndex : std::as_const(changedIndexes)) {
        if (changedIndex.isValid() && (changedIndex.model() == this)) {
            emit dataChanged(changedIndex, changedIndex, {TextRole});
            if (!self) {
        return;
            }
        }
    }
}

bool QmlObjectListModel::_emitPendingObjectNameChanges()
{
    const QSet<QObject*> pendingChanges = std::exchange(_pendingObjectNameChanges, {});
    const QPointer<QmlObjectListModel> self(this);
    for (QObject* const object : pendingChanges) {
        _emitObjectNameChanged(object);
        if (!self) {
            return false;
        }
    }
    return true;
}

void QmlObjectListModel::_scheduleNullCleanup()
{
    if (_nullCleanupScheduled) {
        return;
    }

    _nullCleanupScheduled = true;
    QTimer::singleShot(0, this, [this]() {
        _nullCleanupScheduled = false;
        if (_modelChangeInProgress) {
            _scheduleNullCleanup();
            return;
        }
        _removeNullObjects();
    });
}

void QmlObjectListModel::_removeNullObjects()
{
    if (_modelChangeInProgress || !_nullCleanupPending) {
        return;
    }

    _modelChangeInProgress = true;
    const QPointer<QmlObjectListModel> self(this);
    do {
        _nullCleanupPending = false;
        bool removedObjects = false;
        bool removedFirstRow = false;
        for (int row = _objectList.size() - 1; row >= 0;) {
            if (_objectList.at(row).object || !_objectList.at(row).identity) {
                --row;
                continue;
            }

            const int last = row;
            while ((row >= 0) && !_objectList.at(row).object && _objectList.at(row).identity) {
                --row;
            }
            const int first = row + 1;
            removedFirstRow = removedFirstRow || (first == 0);
    if (_resetModelNestingCount == 0) {
                beginRemoveRows(QModelIndex(), first, last);
                if (!self) {
        return;
                }
            }
            _objectList.remove(first, last - first + 1);
    if (_resetModelNestingCount == 0) {
        endRemoveRows();
                if (!self) {
        return;
                }
            }
            removedObjects = true;
        }
        if (_skipDirtyFirstItem && removedFirstRow && !_objectList.isEmpty()) {
            _removeDirtyOccurrence(at(0));
        }
        if (removedObjects) {
            setDirty(true);
            if (!self) {
                return;
            }
            _signalCountChangedIfNotNested();
            if (!self) {
                return;
            }
        }
    } while (_nullCleanupPending);

    _modelChangeInProgress = false;
    if (!_emitPendingObjectNameChanges() || !self) {
        return;
    }
    _applyPendingChanges();
}

bool QmlObjectListModel::_finishMutation(bool countChanged)
{
    const QPointer<QmlObjectListModel> self(this);
    if (countChanged) {
    _signalCountChangedIfNotNested();
        if (!self) {
            return false;
        }
    }
    _modelChangeInProgress = false;
    _removeNullObjects();
    if (!self) {
        return false;
    }
    if (!_emitPendingObjectNameChanges() || !self) {
        return false;
    }
    _applyPendingChanges();
    return self;
}

void QmlObjectListModel::_queueInsert(int position, const QList<ObjectSlot>& objectSlots)
{
    PendingChange change;
    change.type = PendingChangeType::Insert;
    change.objectSlots = objectSlots;
    change.append = position == _objectList.size();
    change.fallbackPosition = position;
    if (!change.append) {
        change.firstIndex = QPersistentModelIndex(index(position, 0));
    }
    _pendingChanges.append(std::move(change));
}

void QmlObjectListModel::_queueRemove(int position, int rows)
{
    PendingChange change;
    change.type = PendingChangeType::Remove;
    change.indexes.reserve(rows);
    for (int row = position; row < (position + rows); ++row) {
        change.indexes.append(QPersistentModelIndex(index(row, 0)));
    }
    _pendingChanges.append(std::move(change));
}

void QmlObjectListModel::_queueMove(int from, int to)
{
    PendingChange change;
    change.type = PendingChangeType::Move;
    change.firstIndex = QPersistentModelIndex(index(from, 0));
    change.secondIndex = QPersistentModelIndex(index(to, 0));
    _pendingChanges.append(std::move(change));
}

void QmlObjectListModel::_applyPendingChanges()
{
    if (_modelChangeInProgress || _applyingPendingChanges) {
        return;
    }

    _applyingPendingChanges = true;
    const QPointer<QmlObjectListModel> self(this);
    while (!_pendingChanges.isEmpty()) {
        const PendingChange change = _pendingChanges.takeFirst();
        switch (change.type) {
            case PendingChangeType::Insert: {
                const int position =
                    change.append
                        ? _objectList.size()
                        : (change.firstIndex.isValid()
                               ? change.firstIndex.row()
                               : std::clamp(change.fallbackPosition, 0, static_cast<int>(_objectList.size())));
                _insert(position, change.objectSlots);
                break;
            }
            case PendingChangeType::Remove: {
                QList<int> rows;
                rows.reserve(change.indexes.size());
                for (const QPersistentModelIndex& persistentIndex : change.indexes) {
                    if (persistentIndex.isValid() && (persistentIndex.model() == this)) {
                        rows.append(persistentIndex.row());
                    }
                }
                std::sort(rows.begin(), rows.end(), std::greater<int>());
                rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
                for (const int row : std::as_const(rows)) {
                    (void) removeRows(row, 1);
                }
                break;
            }
            case PendingChangeType::Move:
                if (change.firstIndex.isValid() && change.secondIndex.isValid() &&
                    (change.firstIndex.model() == this) && (change.secondIndex.model() == this)) {
                    move(change.firstIndex.row(), change.secondIndex.row());
                }
                break;
            case PendingChangeType::Clear:
    clear();
                break;
            case PendingChangeType::ClearAndDelete:
                _clearAndDeleteContents(change.objects);
                break;
}
        if (!self) {
        return;
        }
    }
    _applyingPendingChanges = false;
}

void QmlObjectListModel::_clearAndDeleteContents(const QList<QPointer<QObject>>& additionalObjects)
{
    QSet<QObject*> seenObjects;
    QList<QPointer<QObject>> objects;
    const auto appendUniqueObjects = [&seenObjects, &objects](const QList<QPointer<QObject>>& candidates) {
        for (const QPointer<QObject>& object : candidates) {
            if (object && !seenObjects.contains(object.data())) {
                seenObjects.insert(object.data());
                objects.append(object);
            }
        }
    };
    appendUniqueObjects(additionalObjects);
    QList<QPointer<QObject>> currentObjects;
    currentObjects.reserve(_objectList.size());
    for (const ObjectSlot& slot : std::as_const(_objectList)) {
        currentObjects.append(slot.object);
    }
    appendUniqueObjects(currentObjects);

    const bool wasApplyingPendingChanges = _applyingPendingChanges;
    _applyingPendingChanges = true;
    const QPointer<QmlObjectListModel> self(this);
    clear();
    for (const QPointer<QObject>& object : std::as_const(objects)) {
    if (object) {
            object->deleteLater();
        }
    }
    if (!self) {
        return;
    }
    _applyingPendingChanges = wasApplyingPendingChanges;
    if (!wasApplyingPendingChanges) {
        _applyPendingChanges();
    }
}
