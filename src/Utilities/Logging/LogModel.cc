#include "LogModel.h"

#include <QtCore/QMetaObject>
#include <QtCore/QThread>

LogModel::LogModel(QObject* parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<QGCLogEntry>("QGCLogEntry");
}

int LogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    QMutexLocker locker(&_mutex);
    return _filteredIndices.size();
}

int LogModel::totalCount() const
{
    QMutexLocker locker(&_mutex);
    return _entries.size();
}

QVariant LogModel::data(const QModelIndex& index, int role) const
{
    QMutexLocker locker(&_mutex);

    if (!index.isValid() || index.row() < 0 || index.row() >= _filteredIndices.size()) {
        return {};
    }

    const int entryIndex = _filteredIndices.at(index.row());
    if (entryIndex < 0 || entryIndex >= _entries.size()) {
        return {};
    }

    const QGCLogEntry& entry = _entries.at(entryIndex);

    switch (role) {
    case Qt::DisplayRole:
    case FormattedRole:
        return entry.toString();
    case TimestampRole:
        return entry.timestamp;
    case LevelRole:
        return entry.level;
    case CategoryRole:
        return entry.category;
    case MessageRole:
        return entry.message;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {LevelRole, "level"},
        {CategoryRole, "category"},
        {MessageRole, "message"},
        {FormattedRole, "formatted"}
    };
}

void LogModel::append(const QGCLogEntry& entry)
{
    if (QThread::currentThread() == thread()) {
        _appendInternal(entry);
    } else {
        QMetaObject::invokeMethod(this, [this, entry]() {
            _appendInternal(entry);
        }, Qt::QueuedConnection);
    }
}

void LogModel::_appendInternal(const QGCLogEntry& entry)
{
    const int entryIndex = _entries.size();
    const bool matchesFilter = _matchesFilter(entry);

    // Add to categories set
    if (!entry.category.isEmpty()) {
        _categories.insert(entry.category);
    }

    // Determine filtered row before insertion
    int filteredRow = -1;
    if (matchesFilter) {
        QMutexLocker locker(&_mutex);
        filteredRow = _filteredIndices.size();
    }

    // Insert into filtered view if matches
    if (matchesFilter) {
        beginInsertRows(QModelIndex(), filteredRow, filteredRow);
        {
            QMutexLocker locker(&_mutex);
            _entries.append(entry);
            _filteredIndices.append(entryIndex);
        }
        endInsertRows();
        emit countChanged();
    } else {
        QMutexLocker locker(&_mutex);
        _entries.append(entry);
    }

    emit totalCountChanged();
    _trimExcess();
    emit entryAdded(entry);
}

void LogModel::_trimExcess()
{
    int removeCount;
    {
        QMutexLocker locker(&_mutex);
        if (_entries.size() <= _maxEntries) {
            return;
        }
        removeCount = _entries.size() - _maxEntries;
    }

    // Remove from main list and rebuild filter
    {
        QMutexLocker locker(&_mutex);
        _entries.remove(0, removeCount);

        // Update categories (rebuild from remaining entries)
        _categories.clear();
        for (const QGCLogEntry& entry : _entries) {
            if (!entry.category.isEmpty()) {
                _categories.insert(entry.category);
            }
        }
    }

    // Rebuild filtered indices (model reset is simpler than tracking individual removals)
    beginResetModel();
    _rebuildFilteredIndices();
    endResetModel();

    emit countChanged();
    emit totalCountChanged();
}

void LogModel::clear()
{
    {
        QMutexLocker locker(&_mutex);
        if (_entries.isEmpty()) {
            return;
        }
    }

    beginResetModel();
    {
        QMutexLocker locker(&_mutex);
        _entries.clear();
        _filteredIndices.clear();
        _categories.clear();
    }
    endResetModel();

    emit countChanged();
    emit totalCountChanged();
}

QStringList LogModel::allFormatted() const
{
    QMutexLocker locker(&_mutex);

    QStringList result;
    result.reserve(_entries.size());
    for (const QGCLogEntry& entry : _entries) {
        result.append(entry.toString());
    }
    return result;
}

QStringList LogModel::filteredFormatted() const
{
    QMutexLocker locker(&_mutex);

    QStringList result;
    result.reserve(_filteredIndices.size());
    for (int idx : _filteredIndices) {
        if (idx >= 0 && idx < _entries.size()) {
            result.append(_entries.at(idx).toString());
        }
    }
    return result;
}

QList<QGCLogEntry> LogModel::allEntries() const
{
    QMutexLocker locker(&_mutex);
    return _entries;
}

QList<QGCLogEntry> LogModel::filteredEntries() const
{
    QMutexLocker locker(&_mutex);

    QList<QGCLogEntry> result;
    result.reserve(_filteredIndices.size());
    for (int idx : _filteredIndices) {
        if (idx >= 0 && idx < _entries.size()) {
            result.append(_entries.at(idx));
        }
    }
    return result;
}

void LogModel::setMaxEntries(int max)
{
    if (max > 0 && max != _maxEntries) {
        _maxEntries = max;
        _trimExcess();
    }
}

void LogModel::setFilterLevel(int level)
{
    if (_filterLevel == level) {
        return;
    }

    _filterLevel = level;

    beginResetModel();
    _rebuildFilteredIndices();
    endResetModel();

    emit filterLevelChanged();
    emit countChanged();
}

void LogModel::setFilterCategory(const QString& category)
{
    if (_filterCategory == category) {
        return;
    }

    _filterCategory = category;

    beginResetModel();
    _rebuildFilteredIndices();
    endResetModel();

    emit filterCategoryChanged();
    emit countChanged();
}

void LogModel::setFilterText(const QString& text)
{
    if (_filterText == text) {
        return;
    }

    _filterText = text;

    beginResetModel();
    _rebuildFilteredIndices();
    endResetModel();

    emit filterTextChanged();
    emit countChanged();
}

void LogModel::clearFilters()
{
    if (_filterLevel == -1 && _filterCategory.isEmpty() && _filterText.isEmpty()) {
        return;
    }

    _filterLevel = -1;
    _filterCategory.clear();
    _filterText.clear();

    beginResetModel();
    _rebuildFilteredIndices();
    endResetModel();

    emit filterLevelChanged();
    emit filterCategoryChanged();
    emit filterTextChanged();
    emit countChanged();
}

QStringList LogModel::categories() const
{
    QMutexLocker locker(&_mutex);
    return _categories.values();
}

void LogModel::_rebuildFilteredIndices()
{
    QMutexLocker locker(&_mutex);

    _filteredIndices.clear();
    _filteredIndices.reserve(_entries.size());

    for (int i = 0; i < _entries.size(); ++i) {
        if (_matchesFilter(_entries.at(i))) {
            _filteredIndices.append(i);
        }
    }
}

bool LogModel::_matchesFilter(const QGCLogEntry& entry) const
{
    // Level filter
    if (_filterLevel >= 0 && static_cast<int>(entry.level) < _filterLevel) {
        return false;
    }

    // Category filter
    if (!_filterCategory.isEmpty()) {
        if (_filterCategory.endsWith('*')) {
            // Prefix match
            const QString prefix = _filterCategory.left(_filterCategory.size() - 1);
            if (!entry.category.startsWith(prefix, Qt::CaseInsensitive)) {
                return false;
            }
        } else {
            // Exact match
            if (entry.category.compare(_filterCategory, Qt::CaseInsensitive) != 0) {
                return false;
            }
        }
    }

    // Text filter (substring match in message)
    if (!_filterText.isEmpty()) {
        if (!entry.message.contains(_filterText, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}
