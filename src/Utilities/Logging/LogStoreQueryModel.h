#pragma once

#include <QtCore/QAbstractTableModel>
#include <QtCore/QFutureWatcher>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "LogEntryTableModel.h"

class LogStore;

class LogStoreQueryModel : public LogEntryTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString sessionFilter READ sessionFilter WRITE setSessionFilter NOTIFY sessionFilterChanged)
    Q_PROPERTY(int filterLevel READ filterLevel WRITE setFilterLevel NOTIFY filterLevelChanged)
    Q_PROPERTY(QString filterCategory READ filterCategory WRITE setFilterCategory NOTIFY filterCategoryChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(qint64 totalResults READ totalResults NOTIFY totalResultsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QStringList availableSessions READ availableSessions NOTIFY availableSessionsChanged)

public:
    explicit LogStoreQueryModel(LogStore* store, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QString sessionFilter() const { return _sessionFilter; }

    void setSessionFilter(const QString& session);

    int filterLevel() const { return _filterLevel; }

    void setFilterLevel(int level);

    QString filterCategory() const { return _filterCategory; }

    void setFilterCategory(const QString& category);

    QString filterText() const { return _filterText; }

    void setFilterText(const QString& text);

    qint64 totalResults() const { return _totalResults; }

    bool loading() const { return _loading; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadMore();
    QStringList availableSessions() const;

signals:
    void sessionFilterChanged();
    void filterLevelChanged();
    void filterCategoryChanged();
    void filterTextChanged();
    void totalResultsChanged();
    void loadingChanged();
    void availableSessionsChanged();

protected:
    const LogEntry* entryAt(int row) const override;

private:
    struct QueryResult
    {
        QList<LogEntry> page;
        bool append = false;
        quint64 generation = 0;
    };

    void _executeQuery(bool append);
    void _onQueryFinished();

    LogStore* _store = nullptr;

    QString _sessionFilter;
    int _filterLevel = LogEntry::Debug;
    QString _filterCategory;
    QString _filterText;

    QList<LogEntry> _results;
    qint64 _totalResults = 0;
    bool _loading = false;
    quint64 _queryGeneration = 0;
    QFutureWatcher<QueryResult> _queryWatcher;

    static constexpr int kPageSize = 1000;
};
