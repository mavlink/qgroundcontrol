#pragma once

#include "QGCLogEntry.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QSet>

/// Model for displaying log entries in QML.
/// Thread-safe for appending entries from any thread.
/// Supports filtering by level, category, and search text.
class LogModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(int filterLevel READ filterLevel WRITE setFilterLevel NOTIFY filterLevelChanged)
    Q_PROPERTY(QString filterCategory READ filterCategory WRITE setFilterCategory NOTIFY filterCategoryChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        LevelRole,
        CategoryRole,
        MessageRole,
        FormattedRole
    };
    Q_ENUM(Roles)

    explicit LogModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Appends an entry. Thread-safe, but actual model update happens on main thread.
    void append(const QGCLogEntry& entry);

    /// Clears all entries. Must be called from main thread.
    Q_INVOKABLE void clear();

    /// Returns all entries (unfiltered) as formatted strings. Thread-safe.
    QStringList allFormatted() const;

    /// Returns filtered entries as formatted strings. Thread-safe.
    QStringList filteredFormatted() const;

    /// Returns all entries (unfiltered). Thread-safe.
    QList<QGCLogEntry> allEntries() const;

    /// Returns filtered entries. Thread-safe.
    QList<QGCLogEntry> filteredEntries() const;

    /// Total count of unfiltered entries.
    int totalCount() const;

    /// Maximum entries to keep in memory.
    void setMaxEntries(int max);
    int maxEntries() const { return _maxEntries; }

    /// Filter by minimum log level (Debug=0, Info=1, Warning=2, Critical=3, Fatal=4).
    /// Set to -1 to show all levels.
    void setFilterLevel(int level);
    int filterLevel() const { return _filterLevel; }

    /// Filter by category (exact match or prefix with "*").
    /// Empty string shows all categories.
    void setFilterCategory(const QString& category);
    QString filterCategory() const { return _filterCategory; }

    /// Filter by search text (case-insensitive substring match in message).
    /// Empty string shows all messages.
    void setFilterText(const QString& text);
    QString filterText() const { return _filterText; }

    /// Clears all filters.
    Q_INVOKABLE void clearFilters();

    /// Returns unique categories in the log.
    Q_INVOKABLE QStringList categories() const;

signals:
    void countChanged();
    void totalCountChanged();
    void filterLevelChanged();
    void filterCategoryChanged();
    void filterTextChanged();
    void entryAdded(const QGCLogEntry& entry);

private slots:
    void _appendInternal(const QGCLogEntry& entry);

private:
    void _trimExcess();
    void _rebuildFilteredIndices();
    bool _matchesFilter(const QGCLogEntry& entry) const;

    mutable QMutex _mutex;
    QList<QGCLogEntry> _entries;
    QList<int> _filteredIndices;  // Indices into _entries that match current filter
    QSet<QString> _categories;
    int _maxEntries = 100000;

    // Filters
    int _filterLevel = -1;        // -1 = show all
    QString _filterCategory;
    QString _filterText;
};
