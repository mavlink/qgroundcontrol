#include "LogModel.h"

#include <QtCore/QMetaObject>
#include <QtCore/QThread>

LogModel::LogModel(QObject* parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<LogEntry>("LogEntry");
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

    const LogEntry& entry = _entries.at(entryIndex);

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
    case FileRole:
        return entry.file;
    case FunctionRole:
        return entry.function;
    case LineRole:
        return entry.line;
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
        {FormattedRole, "formatted"},
        {FileRole, "file"},
        {FunctionRole, "function"},
        {LineRole, "line"}
    };
}

void LogModel::append(const LogEntry& entry)
{
    if (QThread::currentThread() == thread()) {
        _appendInternal(entry);
    } else {
        QMetaObject::invokeMethod(this, [this, entry]() {
            _appendInternal(entry);
        }, Qt::QueuedConnection);
    }
}

void LogModel::_appendInternal(const LogEntry& entry)
{
    // Trim before inserting to avoid beginInsertRows followed by beginResetModel
    const bool trimmed = _trimExcess();

    const bool matchesFilter = _matchesFilter(entry);

    // QRecursiveMutex allows data() to re-lock during beginInsertRows/endInsertRows
    if (matchesFilter) {
        QMutexLocker locker(&_mutex);
        if (!entry.category.isEmpty()) {
            _categoryCounts[entry.category]++;
        }
        const int filteredRow = _filteredIndices.size();
        const int entryIndex = _entries.size();
        beginInsertRows(QModelIndex(), filteredRow, filteredRow);
        _entries.append(entry);
        _filteredIndices.append(entryIndex);
        endInsertRows();
    } else {
        QMutexLocker locker(&_mutex);
        if (!entry.category.isEmpty()) {
            _categoryCounts[entry.category]++;
        }
        _entries.append(entry);
    }

    emit totalCountChanged();
    if (matchesFilter || trimmed) {
        emit countChanged();
    }
    emit entryAdded(entry);
}

bool LogModel::_trimExcess()
{
    QMutexLocker locker(&_mutex);
    if (_entries.size() < _maxEntries) {
        return false;
    }

    const int removeCount = _entries.size() - _maxEntries + 1;

    for (int i = 0; i < removeCount; ++i) {
        const QString& cat = _entries.at(i).category;
        if (!cat.isEmpty()) {
            auto it = _categoryCounts.find(cat);
            if (it != _categoryCounts.end()) {
                if (--(*it) <= 0) {
                    _categoryCounts.erase(it);
                }
            }
        }
    }

    beginResetModel();
    _entries.remove(0, removeCount);
    _rebuildFilteredIndices();
    endResetModel();

    return true;
}

void LogModel::clear()
{
    QMutexLocker locker(&_mutex);
    if (_entries.isEmpty()) {
        return;
    }

    beginResetModel();
    _entries.clear();
    _filteredIndices.clear();
    _categoryCounts.clear();
    endResetModel();

    emit countChanged();
    emit totalCountChanged();
}

QStringList LogModel::allFormatted() const
{
    QMutexLocker locker(&_mutex);

    QStringList result;
    result.reserve(_entries.size());
    for (const LogEntry& entry : _entries) {
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

QList<LogEntry> LogModel::allEntries() const
{
    QMutexLocker locker(&_mutex);
    return _entries;
}

QList<LogEntry> LogModel::filteredEntries() const
{
    QMutexLocker locker(&_mutex);

    QList<LogEntry> result;
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
        emit maxEntriesChanged();
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
    return _categoryCounts.keys();
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

bool LogModel::_matchesFilter(const LogEntry& entry) const
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
