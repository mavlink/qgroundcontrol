#include "LogModel.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LogModelLog, "Utilities.LogModel")

LogModel::LogModel(QObject* parent) : LogEntryTableModel(parent)
{
    _batchTimer.setSingleShot(true);
    (void)connect(&_batchTimer, &QChronoTimer::timeout, this, &LogModel::_flushPending);

    _filterTextDebounce.setInterval(kFilterDebounceMs);
    _filterTextDebounce.setSingleShot(true);
    connect(&_filterTextDebounce, &QTimer::timeout, this, [this]() {
        if (_filterText != _pendingFilterText) {
            _filterText = _pendingFilterText;
            emit filterTextChanged();
            if (_filterRegex) {
                _recompileRegex();
            }
            _rebuildFilteredIndices();
        }
    });
}

int LogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return _filterBypassed ? static_cast<int>(_entries.size())
                           : static_cast<int>(_filteredIndices.size());
}

const LogEntry* LogModel::entryAt(int row) const
{
    if (row < 0) {
        return nullptr;
    }

    if (_filterBypassed) {
        if (row >= static_cast<int>(_entries.size())) {
            return nullptr;
        }
        return &_entries[row];
    }

    if (row >= static_cast<int>(_filteredIndices.size())) {
        return nullptr;
    }
    return &_entries[_filteredIndices[row]];
}

void LogModel::setMaxEntries(int max)
{
    max = qMax(1000, max);
    if (_maxEntries != max) {
        _maxEntries = max;
        emit maxEntriesChanged();
    }
}

void LogModel::setFilterLevel(int level)
{
    if (_filterLevel != level) {
        _filterLevel = level;
        emit filterLevelChanged();
        _rebuildFilteredIndices();
    }
}

void LogModel::setFilterCategory(const QString& category)
{
    if (_filterCategory != category) {
        _filterCategory = category;
        emit filterCategoryChanged();
        _rebuildFilteredIndices();
    }
}

void LogModel::setFilterText(const QString& text)
{
    if (_filterText != text) {
        _pendingFilterText = text;
        _filterTextDebounce.stop();
        _filterText = text;
        emit filterTextChanged();
        if (_filterRegex) {
            _recompileRegex();
        }
        _rebuildFilteredIndices();
    }
}

void LogModel::setFilterTextDeferred(const QString& text)
{
    if (_pendingFilterText != text) {
        _pendingFilterText = text;
        _filterTextDebounce.start();
    }
}

void LogModel::setFilterRegex(bool enabled)
{
    if (_filterRegex != enabled) {
        _filterRegex = enabled;
        emit filterRegexChanged();
        if (enabled && !_filterText.isEmpty()) {
            _recompileRegex();
        } else {
            emit filterRegexValidChanged();
        }
        _rebuildFilteredIndices();
    }
}

void LogModel::clearFilters()
{
    bool changed = false;
    _filterTextDebounce.stop();
    _pendingFilterText.clear();

    if (_filterLevel != LogEntry::Debug) {
        _filterLevel = LogEntry::Debug;
        emit filterLevelChanged();
        changed = true;
    }
    if (!_filterCategory.isEmpty()) {
        _filterCategory.clear();
        emit filterCategoryChanged();
        changed = true;
    }
    if (!_filterText.isEmpty()) {
        _filterText.clear();
        emit filterTextChanged();
        changed = true;
    }
    if (_filterRegex) {
        _filterRegex = false;
        emit filterRegexChanged();
        changed = true;
    }

    if (changed) {
        _rebuildFilteredIndices();
    }
}

bool LogModel::_hasActiveFilter() const
{
    return _filterLevel > LogEntry::Debug || !_filterCategory.isEmpty() || !_filterText.isEmpty();
}

bool LogModel::_passesFilter(const LogEntry& entry) const
{
    if (static_cast<int>(entry.level) < _filterLevel) {
        return false;
    }
    if (!_filterCategory.isEmpty() && !entry.category.contains(_filterCategory, Qt::CaseInsensitive)) {
        return false;
    }
    if (!_filterText.isEmpty()) {
        if (_filterRegex) {
            if (!_compiledRegex.isValid() || !_compiledRegex.match(entry.message).hasMatch()) {
                return false;
            }
        } else {
            if (!entry.message.contains(_filterText, Qt::CaseInsensitive)) {
                return false;
            }
        }
    }
    return true;
}

void LogModel::_recompileRegex()
{
    _compiledRegex = QRegularExpression(_filterText, QRegularExpression::CaseInsensitiveOption);
    emit filterRegexValidChanged();
}

void LogModel::enqueue(LogEntry entry)
{
    _pendingEntries.push_back(std::move(entry));
    if (static_cast<int>(_pendingEntries.size()) >= kBatchMaxSize) {
        _batchTimer.stop();
        _flushPending();
    } else if (!_batchTimer.isActive()) {
        _batchTimer.start();
    }
}

void LogModel::_flushPending()
{
    _batchTimer.stop();

    if (_pendingEntries.empty()) {
        return;
    }

    _trimExcess();

    bool newCategories = false;
    for (auto& entry : _pendingEntries) {
        if (!entry.category.isEmpty() && !_categoriesSet.contains(entry.category)) {
            _categoriesSet.insert(entry.category);
            newCategories = true;
        }
    }

    const int overflow = static_cast<int>(_entries.size() + _pendingEntries.size()) - _maxEntries;
    if (overflow > 0 && overflow <= static_cast<int>(_pendingEntries.size())) {
        _pendingEntries.erase(_pendingEntries.begin(), _pendingEntries.begin() + overflow);
    }

    const int firstNew = static_cast<int>(_entries.size());
    for (auto& entry : _pendingEntries) {
        _entries.push_back(std::move(entry));
    }
    _pendingEntries.clear();

    const int lastNew = static_cast<int>(_entries.size()) - 1;
    if (firstNew <= lastNew) {
        _appendToFiltered(firstNew, lastNew);
    }

    emit totalCountChanged();

    if (newCategories) {
        _invalidateCategoryCache();
        emit categoriesChanged();
    }
}

void LogModel::_appendToFiltered(int first, int last)
{
    if (_filterBypassed) {
        // Entries already in _entries; just signal the view
        beginInsertRows(QModelIndex(), first, last);
        endInsertRows();
        return;
    }

    std::vector<int> newIndices;
    for (int i = first; i <= last; ++i) {
        if (_passesFilter(_entries[i])) {
            newIndices.push_back(i);
        }
    }

    if (!newIndices.empty()) {
        const int proxyFirst = static_cast<int>(_filteredIndices.size());
        beginInsertRows(QModelIndex(), proxyFirst, proxyFirst + static_cast<int>(newIndices.size()) - 1);
        _filteredIndices.insert(_filteredIndices.end(), newIndices.begin(), newIndices.end());
        endInsertRows();
    }
}

void LogModel::_trimExcess()
{
    const int total = static_cast<int>(_entries.size()) + static_cast<int>(_pendingEntries.size());
    if (total <= _maxEntries) {
        return;
    }

    const int removeCount = total - _maxEntries;
    const int actualRemove = qMin(removeCount, static_cast<int>(_entries.size()));
    if (actualRemove <= 0) {
        return;
    }

    if (_filterBypassed) {
        beginRemoveRows(QModelIndex(), 0, actualRemove - 1);
        _entries.erase(_entries.begin(), _entries.begin() + actualRemove);
        endRemoveRows();
        return;
    }

    int filteredRemoved = 0;
    while (filteredRemoved < static_cast<int>(_filteredIndices.size()) &&
           _filteredIndices[filteredRemoved] < actualRemove) {
        ++filteredRemoved;
    }

    if (filteredRemoved > 0) {
        beginRemoveRows(QModelIndex(), 0, filteredRemoved - 1);
        _filteredIndices.erase(_filteredIndices.begin(), _filteredIndices.begin() + filteredRemoved);
        endRemoveRows();
    }

    _entries.erase(_entries.begin(), _entries.begin() + actualRemove);

    for (auto& idx : _filteredIndices) {
        idx -= actualRemove;
    }
}

void LogModel::_rebuildFilteredIndices()
{
    beginResetModel();
    _filteredIndices.clear();

    if (!_hasActiveFilter()) {
        _filterBypassed = true;
    } else {
        _filterBypassed = false;
        for (int i = 0; i < static_cast<int>(_entries.size()); ++i) {
            if (_passesFilter(_entries[i])) {
                _filteredIndices.push_back(i);
            }
        }
    }

    endResetModel();
}

void LogModel::clear()
{
    _batchTimer.stop();
    _pendingEntries.clear();

    beginResetModel();
    _entries.clear();
    _filteredIndices.clear();
    _filterBypassed = !_hasActiveFilter();
    endResetModel();

    emit totalCountChanged();
    _categoriesSet.clear();
    _invalidateCategoryCache();
    emit categoriesChanged();
}

void LogModel::_invalidateCategoryCache()
{
    _categoriesDirty = true;
}

QStringList LogModel::categoriesList() const
{
    if (_categoriesDirty) {
        _categoriesCache = QStringList(_categoriesSet.begin(), _categoriesSet.end());
        _categoriesCache.sort();
        _categoriesDirty = false;
    }
    return _categoriesCache;
}


QList<LogEntry> LogModel::filteredEntries() const
{
    if (_filterBypassed) {
        return QList<LogEntry>(_entries.begin(), _entries.end());
    }

    QList<LogEntry> result;
    result.reserve(static_cast<int>(_filteredIndices.size()));
    for (const int idx : _filteredIndices) {
        result.append(_entries[idx]);
    }
    return result;
}
