/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "ObjectItemModelBase.h"

#include <QtQmlIntegration/QtQmlIntegration>

/// A tree model for QObject* items, usable from both C++ and QML.
/// Works like QmlObjectListModel but supports hierarchical parent/child relationships.
/// Compatible with Qt 6 TreeView.
///
/// Top-level items are children of an invisible root node. The root is represented
/// by an invalid QModelIndex (the default).
class QmlObjectTreeModel : public ObjectItemModelBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit QmlObjectTreeModel(QObject* parent = nullptr);
    ~QmlObjectTreeModel() override;

    // -- ObjectItemModelBase overrides --
    int count() const override;
    void setDirty(bool dirty) override;
    void clear() override;

    // -- QAbstractItemModel overrides --
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;

    // -- QML-accessible tree operations --

    /// Returns the QObject* stored at @p index, or nullptr if invalid
    Q_INVOKABLE QObject* getObject(const QModelIndex& index) const;

    /// Appends @p object as the last child of @p parentIndex (root if invalid). Returns the new item's index.
    Q_INVOKABLE QModelIndex appendItem(QObject* object, const QModelIndex& parentIndex = QModelIndex());

    /// Same as above but also sets a nodeType tag on the created tree node.
    Q_INVOKABLE QModelIndex appendItem(QObject* object, const QModelIndex& parentIndex, const QString& nodeType);

    /// Inserts @p object at @p row under @p parentIndex. Returns the new item's index.
    Q_INVOKABLE QModelIndex insertItem(int row, QObject* object, const QModelIndex& parentIndex = QModelIndex());

    /// Same as above but also sets a nodeType tag on the created tree node.
    Q_INVOKABLE QModelIndex insertItem(int row, QObject* object, const QModelIndex& parentIndex, const QString& nodeType);

    /// Removes the item (and its entire subtree) at @p index. Returns the removed QObject* (caller takes ownership).
    /// Child QObjects are NOT deleted; only the internal tree nodes are freed.
    Q_INVOKABLE QObject* removeItem(const QModelIndex& index);

    /// Number of direct children under @p parentIndex
    Q_INVOKABLE int childCount(const QModelIndex& parentIndex = QModelIndex()) const { return rowCount(parentIndex); }

    /// Returns the QModelIndex for the child at @p row under @p parentIndex
    Q_INVOKABLE QModelIndex childIndex(int row, const QModelIndex& parentIndex = QModelIndex()) const { return index(row, 0, parentIndex); }

    /// Searches the entire tree for @p object and returns its QModelIndex (invalid if not found)
    Q_INVOKABLE QModelIndex indexForObject(QObject* object) const;

    /// Convenience wrapper around parent()
    Q_INVOKABLE QModelIndex parentIndex(const QModelIndex& index) const { return parent(index); }

    /// Returns the depth of @p index (0 = root-level item, -1 = invalid index)
    Q_INVOKABLE int depth(const QModelIndex& index) const;

    // -- C++ convenience API --
    void    appendRootItem(QObject* object);
    void    appendChild(const QModelIndex& parentIndex, QObject* object);
    QObject* removeAt(const QModelIndex& parentIndex, int row);
    void    removeChildren(const QModelIndex& parentIndex); ///< Removes all children of parentIndex without removing the parent itself
    void    clearAndDeleteContents(); ///< Clears the tree and calls deleteLater on every QObject
    bool    contains(QObject* object) const;

    static constexpr int NodeTypeRole  = Qt::UserRole + 2;
    static constexpr int SeparatorRole = Qt::UserRole + 3;

private:
    struct TreeNode {
        QObject* object = nullptr;
        TreeNode* parentNode = nullptr;
        QList<TreeNode*> children;
        QString nodeType;   ///< Discriminator tag for delegates (e.g. "missionGroup", "fenceEditor")

        /// Row of this node within its parent's children list
        int row() const;
    };

    TreeNode*   _nodeFromIndex(const QModelIndex& index) const;
    QModelIndex _indexForNode(const TreeNode* node) const;
    TreeNode*   _findNode(const TreeNode* root, const QObject* object) const;
    static int  _subtreeCount(const TreeNode* node);
    void        _disconnectSubtree(TreeNode* node);
    void        _deleteSubtree(TreeNode* node, bool deleteObjects);
    void        _connectDirtyChanged(QObject* object);
    void        _disconnectDirtyChanged(QObject* object);
    void        _emitSeparatorChanged(const QModelIndex& parentIdx, int fromRow);

    TreeNode _rootNode; ///< Invisible root; top-level items are its children
    int      _totalCount = 0; ///< Cached total node count (all nodes in tree)
};
