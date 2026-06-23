/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QmlObjectTreeModel.h"

#include <QtCore/QMetaMethod>
#include <QtQml/QQmlEngine>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QmlObjectTreeModelLog, "API.QmlObjectTreeModel")

namespace {

constexpr const char* kDirtyChangedSignature = "dirtyChanged(bool)";
constexpr const char* kChildDirtyChangedSlotSignature = "_childDirtyChanged(bool)";

QMetaMethod childDirtyChangedSlot()
{
    const QMetaObject* metaObject = &QmlObjectTreeModel::staticMetaObject;
    const int slotIndex = metaObject->indexOfSlot(kChildDirtyChangedSlotSignature);
    Q_ASSERT_X(slotIndex >= 0, "childDirtyChangedSlot", "slot signature mismatch — update kChildDirtyChangedSlotSignature");
    return (slotIndex >= 0) ? metaObject->method(slotIndex) : QMetaMethod();
}

QMetaMethod dirtyChangedSignal(const QObject* object)
{
    if (!object) {
        return QMetaMethod();
    }

    const int signalIndex = object->metaObject()->indexOfSignal(kDirtyChangedSignature);
    return (signalIndex >= 0) ? object->metaObject()->method(signalIndex) : QMetaMethod();
}

} // namespace

//-----------------------------------------------------------------------------
// TreeNode
//-----------------------------------------------------------------------------

int QmlObjectTreeModel::TreeNode::row() const
{
    if (parentNode) {
        return parentNode->children.indexOf(const_cast<TreeNode*>(this));
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Construction / Destruction
//-----------------------------------------------------------------------------

QmlObjectTreeModel::QmlObjectTreeModel(QObject* parent)
    : ObjectItemModelBase(parent)
{
}

QmlObjectTreeModel::~QmlObjectTreeModel()
{
    // Skip disconnect — objects may already be destroyed during application shutdown.
    // Just delete the tree nodes.
    _deleteSubtree(&_rootNode, false);
}

//-----------------------------------------------------------------------------
// QAbstractItemModel overrides
//-----------------------------------------------------------------------------

QModelIndex QmlObjectTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0) {
        return {};
    }

    const TreeNode* parentNode = parent.isValid() ? _nodeFromIndex(parent) : &_rootNode;
    if (!parentNode || row < 0 || row >= parentNode->children.count()) {
        return {};
    }

    return createIndex(row, 0, parentNode->children.at(row));
}

QModelIndex QmlObjectTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) {
        return {};
    }

    const TreeNode* node = _nodeFromIndex(child);
    if (!node || !node->parentNode || node->parentNode == &_rootNode) {
        return {};
    }

    return _indexForNode(node->parentNode);
}

int QmlObjectTreeModel::rowCount(const QModelIndex& parent) const
{
    const TreeNode* node = parent.isValid() ? _nodeFromIndex(parent) : &_rootNode;
    return node ? node->children.count() : 0;
}

int QmlObjectTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool QmlObjectTreeModel::hasChildren(const QModelIndex& parent) const
{
    const TreeNode* node = parent.isValid() ? _nodeFromIndex(parent) : &_rootNode;
    return node && !node->children.isEmpty();
}

QVariant QmlObjectTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const TreeNode* node = _nodeFromIndex(index);
    if (!node) {
        return {};
    }

    switch (role) {
    case ObjectRole:
        return node->object ? QVariant::fromValue(node->object) : QVariant{};
    case TextRole:
        return node->object ? QVariant::fromValue(node->object->objectName()) : QVariant{};
    case NodeTypeRole:
        return QVariant::fromValue(node->nodeType);
    case SeparatorRole: {
        if (!node->children.isEmpty()) {
            return false;
        }
        const TreeNode* parentNode = node->parentNode;
        if (!parentNode) {
            return false;
        }
        return node->row() < parentNode->children.size() - 1;
    }
    default:
        return {};
    }
}

bool QmlObjectTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != ObjectRole) {
        return false;
    }

    TreeNode* node = _nodeFromIndex(index);
    if (!node) {
        return false;
    }

    if (node->object) {
        _disconnectDirtyChanged(node->object);
    }

    node->object = value.value<QObject*>();

    if (node->object) {
        QQmlEngine::setObjectOwnership(node->object, QQmlEngine::CppOwnership);
        _connectDirtyChanged(node->object);
    }

    emit dataChanged(index, index);
    return true;
}

bool QmlObjectTreeModel::insertRows(int /*row*/, int /*count*/, const QModelIndex& /*parent*/)
{
    qCWarning(QmlObjectTreeModelLog) << "insertRows() not supported — use insertItem()";
    return false;
}

bool QmlObjectTreeModel::removeRows(int /*row*/, int /*count*/, const QModelIndex& /*parent*/)
{
    qCWarning(QmlObjectTreeModelLog) << "removeRows() not supported — use removeItem()";
    return false;
}

QHash<int, QByteArray> QmlObjectTreeModel::roleNames() const
{
    auto roles = ObjectItemModelBase::roleNames();
    roles[NodeTypeRole]  = "nodeType";
    roles[SeparatorRole] = "separator";
    return roles;
}

//-----------------------------------------------------------------------------
// Properties
//-----------------------------------------------------------------------------

int QmlObjectTreeModel::count() const
{
    return _totalCount;
}

void QmlObjectTreeModel::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

//-----------------------------------------------------------------------------
// QML-accessible tree operations
//-----------------------------------------------------------------------------

QObject* QmlObjectTreeModel::getObject(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    const TreeNode* node = _nodeFromIndex(index);
    return node ? node->object : nullptr;
}

QModelIndex QmlObjectTreeModel::appendItem(QObject* object, const QModelIndex& parentIndex)
{
    TreeNode* parentNode = parentIndex.isValid() ? _nodeFromIndex(parentIndex) : &_rootNode;
    if (!parentNode) {
        qCWarning(QmlObjectTreeModelLog) << "appendItem: invalid parent index";
        return {};
    }

    return insertItem(parentNode->children.count(), object, parentIndex);
}

QModelIndex QmlObjectTreeModel::insertItem(int row, QObject* object, const QModelIndex& parentIndex)
{
    return insertItem(row, object, parentIndex, QString());
}

QModelIndex QmlObjectTreeModel::insertItem(int row, QObject* object, const QModelIndex& parentIndex, const QString& nodeType)
{
    TreeNode* parentNode = parentIndex.isValid() ? _nodeFromIndex(parentIndex) : &_rootNode;
    if (!parentNode) {
        qCWarning(QmlObjectTreeModelLog) << "insertItem: invalid parent index";
        return {};
    }

    if (row < 0 || row > parentNode->children.count()) {
        qCWarning(QmlObjectTreeModelLog) << "insertItem: invalid row" << row << "count:" << parentNode->children.count();
        return {};
    }

    auto* node = new TreeNode;
    node->object = object;
    node->parentNode = parentNode;
    node->nodeType = nodeType;

    if (object) {
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
        _connectDirtyChanged(object);
    }

    if (_resetModelNestingCount > 0) {
        // During a batch reset we just accumulate nodes silently
        parentNode->children.insert(row, node);
    } else {
        beginInsertRows(parentIndex, row, row);
        parentNode->children.insert(row, node);
        endInsertRows();
        _emitSeparatorChanged(parentIndex, row > 0 ? row - 1 : 0);
        _signalCountChangedIfNotNested();
    }

    _totalCount++;
    setDirty(true);

    return createIndex(row, 0, node);
}

QModelIndex QmlObjectTreeModel::appendItem(QObject* object, const QModelIndex& parentIndex, const QString& nodeType)
{
    TreeNode* parentNode = parentIndex.isValid() ? _nodeFromIndex(parentIndex) : &_rootNode;
    if (!parentNode) {
        qCWarning(QmlObjectTreeModelLog) << "appendItem: invalid parent index";
        return {};
    }
    return insertItem(parentNode->children.count(), object, parentIndex, nodeType);
}

QObject* QmlObjectTreeModel::removeItem(const QModelIndex& index)
{
    if (!index.isValid()) {
        qCWarning(QmlObjectTreeModelLog) << "removeItem: invalid index";
        return nullptr;
    }

    TreeNode* node = _nodeFromIndex(index);
    if (!node || !node->parentNode) {
        qCWarning(QmlObjectTreeModelLog) << "removeItem: node not found or is the root";
        return nullptr;
    }

    QObject* object = node->object;
    TreeNode* parentNode = node->parentNode;
    const int row = node->row();
    const QModelIndex parentIdx = _indexForNode(parentNode);

    _disconnectSubtree(node);

    // Count the subtree nodes being removed (the node itself + all descendants)
    const int removedCount = 1 + _subtreeCount(node);

    if (_resetModelNestingCount == 0) {
        beginRemoveRows(parentIdx, row, row);
    }
    parentNode->children.removeAt(row);
    if (_resetModelNestingCount == 0) {
        endRemoveRows();
    }

    // Free the subtree's TreeNode objects but NOT the QObjects
    _deleteSubtree(node, false);
    delete node;

    _totalCount -= removedCount;
    if (_resetModelNestingCount == 0 && !parentNode->children.isEmpty()) {
        _emitSeparatorChanged(parentIdx, parentNode->children.count() - 1);
    }
    _signalCountChangedIfNotNested();
    setDirty(true);

    return object;
}

QModelIndex QmlObjectTreeModel::indexForObject(QObject* object) const
{
    if (!object) {
        return {};
    }

    const TreeNode* node = _findNode(&_rootNode, object);
    return node ? _indexForNode(node) : QModelIndex();
}

int QmlObjectTreeModel::depth(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return -1;
    }

    int d = 0;
    const TreeNode* node = _nodeFromIndex(index);
    while (node && node->parentNode && node->parentNode != &_rootNode) {
        d++;
        node = node->parentNode;
    }
    return d;
}

//-----------------------------------------------------------------------------
// C++ convenience API
//-----------------------------------------------------------------------------

void QmlObjectTreeModel::appendRootItem(QObject* object)
{
    appendItem(object);
}

void QmlObjectTreeModel::appendChild(const QModelIndex& parentIndex, QObject* object)
{
    appendItem(object, parentIndex);
}

QObject* QmlObjectTreeModel::removeAt(const QModelIndex& parentIndex, int row)
{
    return removeItem(index(row, 0, parentIndex));
}

void QmlObjectTreeModel::removeChildren(const QModelIndex& parentIndex)
{
    TreeNode* parentNode = parentIndex.isValid() ? _nodeFromIndex(parentIndex) : &_rootNode;
    if (!parentNode || parentNode->children.isEmpty()) {
        return;
    }

    const int childCount = parentNode->children.count();

    // Count all nodes being removed (direct children + their subtrees)
    int removedCount = childCount;
    for (const TreeNode* child : parentNode->children) {
        removedCount += _subtreeCount(child);
    }

    if (_resetModelNestingCount == 0) {
        beginRemoveRows(parentIndex, 0, childCount - 1);
    }

    // Disconnect signals but don't free nodes yet — views may still access them
    for (TreeNode* child : parentNode->children) {
        _disconnectSubtree(child);
    }

    // Detach from parent
    QList<TreeNode*> orphans = parentNode->children;
    parentNode->children.clear();
    _totalCount -= removedCount;

    if (_resetModelNestingCount == 0) {
        endRemoveRows();
        _signalCountChangedIfNotNested();
    }

    // Now safe to free the TreeNode structs
    for (TreeNode* child : orphans) {
        _deleteSubtree(child, false);
        delete child;
    }
}

void QmlObjectTreeModel::clear()
{
    if (_rootNode.children.isEmpty()) {
        return;
    }

    beginResetModel();
    _disconnectSubtree(&_rootNode);
    _deleteSubtree(&_rootNode, false);
    _totalCount = 0;
    endResetModel();
}

void QmlObjectTreeModel::clearAndDeleteContents()
{
    if (_rootNode.children.isEmpty()) {
        return;
    }

    beginResetModel();
    _disconnectSubtree(&_rootNode);
    _deleteSubtree(&_rootNode, true);
    _totalCount = 0;
    endResetModel();
}

bool QmlObjectTreeModel::contains(QObject* object) const
{
    return _findNode(&_rootNode, object) != nullptr;
}

//-----------------------------------------------------------------------------
// Private helpers
//-----------------------------------------------------------------------------

QmlObjectTreeModel::TreeNode* QmlObjectTreeModel::_nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return nullptr;
    }
    return static_cast<TreeNode*>(index.internalPointer());
}

QModelIndex QmlObjectTreeModel::_indexForNode(const TreeNode* node) const
{
    if (!node || node == &_rootNode) {
        return {};
    }
    return createIndex(node->row(), 0, const_cast<TreeNode*>(node));
}

QmlObjectTreeModel::TreeNode* QmlObjectTreeModel::_findNode(const TreeNode* root, const QObject* object) const
{
    for (TreeNode* child : root->children) {
        if (child->object == object) {
            return child;
        }
        TreeNode* found = _findNode(child, object);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

int QmlObjectTreeModel::_subtreeCount(const TreeNode* node)
{
    int result = node->children.count();
    for (const TreeNode* child : node->children) {
        result += _subtreeCount(child);
    }
    return result;
}

void QmlObjectTreeModel::_disconnectSubtree(TreeNode* node)
{
    if (node != &_rootNode && node->object) {
        _disconnectDirtyChanged(node->object);
    }
    for (TreeNode* child : node->children) {
        _disconnectSubtree(child);
    }
}

void QmlObjectTreeModel::_deleteSubtree(TreeNode* node, bool deleteObjects)
{
    for (TreeNode* child : node->children) {
        _deleteSubtree(child, deleteObjects);
        if (deleteObjects && child->object) {
            child->object->deleteLater();
        }
        delete child;
    }
    node->children.clear();
}

void QmlObjectTreeModel::_connectDirtyChanged(QObject* object)
{
    const QMetaMethod signal = dirtyChangedSignal(object);
    const QMetaMethod slot = childDirtyChangedSlot();
    if (signal.isValid() && slot.isValid()) {
        connect(object, signal, this, slot);
    }
}

void QmlObjectTreeModel::_disconnectDirtyChanged(QObject* object)
{
    const QMetaMethod signal = dirtyChangedSignal(object);
    const QMetaMethod slot = childDirtyChangedSlot();
    if (signal.isValid() && slot.isValid()) {
        disconnect(object, signal, this, slot);
    }
}

void QmlObjectTreeModel::_emitSeparatorChanged(const QModelIndex& parentIdx, int fromRow)
{
    const TreeNode* parentNode = parentIdx.isValid() ? _nodeFromIndex(parentIdx) : &_rootNode;
    if (!parentNode) {
        return;
    }
    const int last = parentNode->children.count() - 1;
    if (fromRow > last) {
        fromRow = last;
    }
    if (fromRow < 0) {
        return;
    }
    const QModelIndex first = index(fromRow, 0, parentIdx);
    const QModelIndex end = index(last, 0, parentIdx);
    emit dataChanged(first, end, {SeparatorRole});
}
