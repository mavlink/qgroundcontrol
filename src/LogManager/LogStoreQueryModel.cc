#include "LogStoreQueryModel.h"

#include <QtConcurrent/QtConcurrentRun>

#include "LogStore.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LogStoreQueryModelLog, "Utilities.LogStoreQueryModel")

LogStoreQueryModel::LogStoreQueryModel(LogStore* store, QObject* parent) : LogEntryTableModel(parent), _store(store)
{
    if (_store) {
        (void)connect(_store, &LogStore::entryCountChanged, this, &LogStoreQueryModel::availableSessionsChanged);
    }
    (void)connect(&_queryWatcher, &QFutureWatcher<QueryResult>::finished, this, &LogStoreQueryModel::_onQueryFinished);
}

int LogStoreQueryModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : _results.size();
}

const LogEntry* LogStoreQueryModel::entryAt(int row) const
{
    if (row < 0 || row >= _results.size()) {
        return nullptr;
    }
    return &_results.at(row);
}

void LogStoreQueryModel::setSessionFilter(const QString& session)
{
    if (_sessionFilter != session) {
        _sessionFilter = session;
        emit sessionFilterChanged();
        refresh();
    }
}

void LogStoreQueryModel::setFilterLevel(int level)
{
    if (_filterLevel != level) {
        _filterLevel = level;
        emit filterLevelChanged();
        refresh();
    }
}

void LogStoreQueryModel::setFilterCategory(const QString& category)
{
    if (_filterCategory != category) {
        _filterCategory = category;
        emit filterCategoryChanged();
        refresh();
    }
}

void LogStoreQueryModel::setFilterText(const QString& text)
{
    if (_filterText != text) {
        _filterText = text;
        emit filterTextChanged();
        refresh();
    }
}

void LogStoreQueryModel::refresh()
{
    _executeQuery(false);
}

void LogStoreQueryModel::loadMore()
{
    if (_results.size() < _totalResults) {
        _executeQuery(true);
    }
}

QStringList LogStoreQueryModel::availableSessions() const
{
    return _store ? _store->sessions() : QStringList();
}

void LogStoreQueryModel::_executeQuery(bool append)
{
    if (!_store || !_store->isOpen()) {
        if (!_results.isEmpty()) {
            beginResetModel();
            _results.clear();
            endResetModel();
            _totalResults = 0;
            emit totalResultsChanged();
        }
        return;
    }

    _loading = true;
    emit loadingChanged();

    const quint64 gen = ++_queryGeneration;

    LogStore::QueryParams params;
    params.sessionId = _sessionFilter;
    params.minLevel = _filterLevel;
    params.category = _filterCategory;
    params.textFilter = _filterText;
    params.limit = kPageSize;
    params.offset = append ? _results.size() : 0;

    LogStore* store = _store;
    _queryWatcher.setFuture(QtConcurrent::run([store, params, append, gen]() -> QueryResult {
        return {store->query(params), append, gen};
    }));
}

void LogStoreQueryModel::_onQueryFinished()
{
    const QueryResult result = _queryWatcher.result();

    if (result.generation != _queryGeneration) {
        return;
    }

    const auto& page = result.page;

    if (result.append && !page.isEmpty()) {
        const int first = _results.size();
        beginInsertRows(QModelIndex(), first, first + page.size() - 1);
        _results.append(page);
        endInsertRows();
    } else if (!result.append) {
        beginResetModel();
        _results = page;
        endResetModel();
    }

    if (page.size() < kPageSize) {
        _totalResults = _results.size();
    } else if (!_sessionFilter.isEmpty()) {
        _totalResults = _store->sessionEntryCount(_sessionFilter);
    } else {
        _totalResults = _results.size() + kPageSize;
    }
    emit totalResultsChanged();

    _loading = false;
    emit loadingChanged();
}
