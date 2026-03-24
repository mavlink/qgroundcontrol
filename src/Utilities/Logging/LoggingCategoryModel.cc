#include "LoggingCategoryModel.h"

#include "QGCLoggingCategory.h"

// ---------------------------------------------------------------------------
// LoggingCategoryFlatModel
// ---------------------------------------------------------------------------

LoggingCategoryFlatModel::LoggingCategoryFlatModel(QObject* parent) : QAbstractListModel(parent) {}

int LoggingCategoryFlatModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : _items.count();
}

QVariant LoggingCategoryFlatModel::data(const QModelIndex& index, int role) const
{
    using enum Roles;

    if (!index.isValid() || index.row() >= _items.count()) {
        return {};
    }

    const auto* item = _items.at(index.row());

    switch (role) {
        case Qt::DisplayRole:
        case static_cast<int>(FullNameRole):
            return item->fullCategory;
        case static_cast<int>(ShortNameRole):
            return item->shortCategory;
        case static_cast<int>(EnabledRole):
            return item->enabled();
        case static_cast<int>(LogLevelRole):
            return item->logLevel();
    }

    return {};
}

bool LoggingCategoryFlatModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    using enum Roles;

    if (!index.isValid() || index.row() >= _items.count()) {
        return false;
    }

    auto* item = _items.at(index.row());

    if (role == static_cast<int>(EnabledRole)) {
        item->setEnabled(value.toBool());
        emit dataChanged(index, index, {static_cast<int>(EnabledRole), static_cast<int>(LogLevelRole)});
        return true;
    }

    if (role == static_cast<int>(LogLevelRole)) {
        item->setLogLevel(value.toInt());
        emit dataChanged(index, index, {static_cast<int>(EnabledRole), static_cast<int>(LogLevelRole)});
        return true;
    }

    return false;
}

Qt::ItemFlags LoggingCategoryFlatModel::flags(const QModelIndex& index) const
{
    auto f = QAbstractListModel::flags(index);
    if (index.isValid()) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}

QHash<int, QByteArray> LoggingCategoryFlatModel::roleNames() const
{
    using enum Roles;
    return {
        {Qt::DisplayRole, "display"},
        {static_cast<int>(ShortNameRole), "shortName"},
        {static_cast<int>(FullNameRole), "fullName"},
        {static_cast<int>(EnabledRole), "enabled"},
        {static_cast<int>(LogLevelRole), "logLevel"},
    };
}

void LoggingCategoryFlatModel::insertSorted(QGCLoggingCategoryItem* item)
{
    using enum Roles;

    int pos = 0;
    while (pos < _items.count() && _items.at(pos)->fullCategory < item->fullCategory) {
        ++pos;
    }
    beginInsertRows(QModelIndex(), pos, pos);
    _items.insert(pos, item);
    endInsertRows();

    connect(item, &QGCLoggingCategoryItem::logLevelChanged, this, [this, item]() {
        const int row = _items.indexOf(item);
        if (row >= 0) {
            const QModelIndex idx = index(row);
            emit dataChanged(idx, idx, {static_cast<int>(EnabledRole), static_cast<int>(LogLevelRole)});
        }
    });
}

QGCLoggingCategoryItem* LoggingCategoryFlatModel::findByFullName(const QString& fullName) const
{
    for (auto* item : _items) {
        if (item->fullCategory == fullName) {
            return item;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// LoggingCategoryTreeModel (QAbstractItemModel for TreeView)
// ---------------------------------------------------------------------------

LoggingCategoryTreeModel::LoggingCategoryTreeModel(QObject* parent) : QAbstractItemModel(parent) {}

LoggingCategoryTreeModel::~LoggingCategoryTreeModel() = default;

LoggingCategoryTreeNode* LoggingCategoryTreeModel::nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return const_cast<LoggingCategoryTreeNode*>(&_root);
    }
    return static_cast<LoggingCategoryTreeNode*>(index.internalPointer());
}

QModelIndex LoggingCategoryTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0) {
        return {};
    }
    auto* parentNode = nodeFromIndex(parent);
    if (row < 0 || row >= parentNode->children.count()) {
        return {};
    }
    return createIndex(row, 0, parentNode->children.at(row));
}

QModelIndex LoggingCategoryTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) {
        return {};
    }
    auto* node = static_cast<LoggingCategoryTreeNode*>(child.internalPointer());
    auto* parentNode = node->parent;
    if (!parentNode || parentNode == &_root) {
        return {};
    }
    auto* grandparent = parentNode->parent ? parentNode->parent : const_cast<LoggingCategoryTreeNode*>(&_root);
    const int row = grandparent->children.indexOf(parentNode);
    return createIndex(row, 0, parentNode);
}

int LoggingCategoryTreeModel::rowCount(const QModelIndex& parent) const
{
    return nodeFromIndex(parent)->children.count();
}

int LoggingCategoryTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool LoggingCategoryTreeModel::hasChildren(const QModelIndex& parent) const
{
    return !nodeFromIndex(parent)->children.isEmpty();
}

QVariant LoggingCategoryTreeModel::data(const QModelIndex& index, int role) const
{
    using enum Roles;

    if (!index.isValid()) {
        return {};
    }

    auto* node = static_cast<LoggingCategoryTreeNode*>(index.internalPointer());
    auto* item = node->item;
    if (!item) {
        return {};
    }

    switch (role) {
        case Qt::DisplayRole:
        case static_cast<int>(ShortNameRole):
            return item->shortCategory;
        case static_cast<int>(FullNameRole):
            return item->fullCategory;
        case static_cast<int>(EnabledRole):
            return item->enabled();
        case static_cast<int>(LogLevelRole):
            return item->logLevel();
    }

    return {};
}

bool LoggingCategoryTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    using enum Roles;

    if (!index.isValid()) {
        return false;
    }

    auto* node = static_cast<LoggingCategoryTreeNode*>(index.internalPointer());
    auto* item = node->item;
    if (!item) {
        return false;
    }

    if (role == static_cast<int>(EnabledRole)) {
        item->setEnabled(value.toBool());
        emit dataChanged(index, index, {static_cast<int>(EnabledRole), static_cast<int>(LogLevelRole)});
        return true;
    }

    if (role == static_cast<int>(LogLevelRole)) {
        item->setLogLevel(value.toInt());
        emit dataChanged(index, index, {static_cast<int>(EnabledRole), static_cast<int>(LogLevelRole)});
        return true;
    }

    return false;
}

Qt::ItemFlags LoggingCategoryTreeModel::flags(const QModelIndex& index) const
{
    auto f = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        f |= Qt::ItemIsEditable;
    }
    return f;
}

QHash<int, QByteArray> LoggingCategoryTreeModel::roleNames() const
{
    using enum Roles;
    return {
        {Qt::DisplayRole, "display"},
        {static_cast<int>(ShortNameRole), "shortName"},
        {static_cast<int>(FullNameRole), "fullName"},
        {static_cast<int>(EnabledRole), "enabled"},
        {static_cast<int>(LogLevelRole), "logLevel"},
    };
}

int LoggingCategoryTreeModel::insertionIndex(LoggingCategoryTreeNode* parent, const QString& name) const
{
    int pos = 0;
    while (pos < parent->children.count()) {
        auto* existing = parent->children.at(pos)->item;
        if (existing && existing->shortCategory >= name) {
            break;
        }
        ++pos;
    }
    return pos;
}

LoggingCategoryTreeNode* LoggingCategoryTreeModel::findOrCreateIntermediateNode(
    LoggingCategoryTreeNode* parentNode, const QString& segment, const QString& fullPrefix)
{
    for (auto* child : std::as_const(parentNode->children)) {
        if (child->item && child->item->shortCategory == segment) {
            return child;
        }
    }

    // Create intermediate (group) node
    auto* item = new QGCLoggingCategoryItem(segment, fullPrefix, QtWarningMsg, this);
    auto* node = new LoggingCategoryTreeNode;
    node->item = item;
    node->parent = parentNode;

    const QModelIndex parentIndex = (parentNode == &_root) ? QModelIndex() : createIndex(
        parentNode->parent ? parentNode->parent->children.indexOf(parentNode) : 0, 0, parentNode);
    const int pos = insertionIndex(parentNode, segment);

    beginInsertRows(parentIndex, pos, pos);
    parentNode->children.insert(pos, node);
    endInsertRows();

    connect(item, &QGCLoggingCategoryItem::logLevelChanged, this, [this, node]() {
        auto* p = node->parent ? node->parent : &_root;
        const int row = p->children.indexOf(node);
        if (row >= 0) {
            const QModelIndex idx = createIndex(row, 0, node);
            emit dataChanged(idx, idx, {static_cast<int>(Roles::EnabledRole), static_cast<int>(Roles::LogLevelRole)});
        }
    });

    return node;
}

void LoggingCategoryTreeModel::insertCategory(const QStringList& pathSegments, const QString& fullCategory,
                                               QGCLoggingCategoryItem* item)
{
    LoggingCategoryTreeNode* currentParent = &_root;

    // Create/find intermediate nodes for all segments except the last
    QString prefix;
    for (int i = 0; i < pathSegments.size() - 1; ++i) {
        if (!prefix.isEmpty()) {
            prefix += QLatin1Char('.');
        }
        prefix += pathSegments[i];
        currentParent = findOrCreateIntermediateNode(currentParent, pathSegments[i], prefix + QLatin1Char('.'));
    }

    // Insert the leaf node
    auto* node = new LoggingCategoryTreeNode;
    node->item = item;
    node->parent = currentParent;

    const QModelIndex parentIndex = (currentParent == &_root) ? QModelIndex() : createIndex(
        currentParent->parent ? currentParent->parent->children.indexOf(currentParent) : 0, 0, currentParent);
    const int pos = insertionIndex(currentParent, pathSegments.last());

    beginInsertRows(parentIndex, pos, pos);
    currentParent->children.insert(pos, node);
    endInsertRows();

    connect(item, &QGCLoggingCategoryItem::logLevelChanged, this, [this, node]() {
        auto* p = node->parent ? node->parent : &_root;
        const int row = p->children.indexOf(node);
        if (row >= 0) {
            const QModelIndex idx = createIndex(row, 0, node);
            emit dataChanged(idx, idx, {static_cast<int>(Roles::EnabledRole), static_cast<int>(Roles::LogLevelRole)});
        }
    });
}
