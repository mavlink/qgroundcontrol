#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>
#include <chrono>
#include <deque>
#include <vector>

#include "LogEntryTableModel.h"

class LogModel : public LogEntryTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(int maxEntries READ maxEntries WRITE setMaxEntries NOTIFY maxEntriesChanged)
    Q_PROPERTY(QStringList categoriesList READ categoriesList NOTIFY categoriesChanged)
    Q_PROPERTY(int filterLevel READ filterLevel WRITE setFilterLevel NOTIFY filterLevelChanged)
    Q_PROPERTY(QString filterCategory READ filterCategory WRITE setFilterCategory NOTIFY filterCategoryChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(bool filterRegex READ filterRegex WRITE setFilterRegex NOTIFY filterRegexChanged)
    Q_PROPERTY(bool filterRegexValid READ filterRegexValid NOTIFY filterRegexValidChanged)

public:
    explicit LogModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int totalCount() const { return static_cast<int>(_entries.size()); }

    int maxEntries() const { return _maxEntries; }

    void setMaxEntries(int max);

    int filterLevel() const { return _filterLevel; }

    void setFilterLevel(int level);

    QString filterCategory() const { return _filterCategory; }

    void setFilterCategory(const QString& category);

    QString filterText() const { return _filterText; }

    void setFilterText(const QString& text);

    bool filterRegex() const { return _filterRegex; }

    bool filterRegexValid() const { return !_filterRegex || _compiledRegex.isValid(); }

    void setFilterRegex(bool enabled);

    QStringList categoriesList() const;
    Q_INVOKABLE void setFilterTextDeferred(const QString& text);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void clear();

    void enqueue(LogEntry entry);

    QList<LogEntry> allEntriesSnapshot() const { return QList<LogEntry>(_entries.begin(), _entries.end()); }

    QList<LogEntry> filteredEntries() const;

signals:
    void totalCountChanged();
    void maxEntriesChanged();
    void categoriesChanged();
    void filterLevelChanged();
    void filterCategoryChanged();
    void filterTextChanged();
    void filterRegexChanged();
    void filterRegexValidChanged();

protected:
    const LogEntry* entryAt(int row) const override;

private slots:
    void _flushPending();

private:
    void _trimExcess();
    void _rebuildFilteredIndices();
    bool _passesFilter(const LogEntry& entry) const;
    bool _hasActiveFilter() const;
    void _recompileRegex();
    void _appendToFiltered(int first, int last);
    void _invalidateCategoryCache();

    std::deque<LogEntry> _entries;
    std::vector<LogEntry> _pendingEntries;
    std::vector<int> _filteredIndices;
    QChronoTimer _batchTimer{std::chrono::milliseconds{kBatchFlushMs}};
    QTimer _filterTextDebounce;
    QString _pendingFilterText;

    int _maxEntries = 100000;
    int _filterLevel = LogEntry::Debug;
    QString _filterCategory;
    QString _filterText;
    bool _filterRegex = false;
    bool _filterBypassed = true; // true when no active filter — avoids identity-map allocation
    QRegularExpression _compiledRegex;

    QSet<QString> _categoriesSet;
    mutable QStringList _categoriesCache;
    mutable bool _categoriesDirty = false;

    static constexpr int kBatchFlushMs = 16;
    static constexpr int kBatchMaxSize = 200;
    static constexpr int kFilterDebounceMs = 200;
};
