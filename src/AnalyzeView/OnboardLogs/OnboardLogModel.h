#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <cstdint>

class OnboardLogEntry;

/// Typed list model for onboard-log entries.
///
/// ObjectRole remains the first user role for compatibility with existing QML
/// delegates. The additional roles make sorting and future delegates independent
/// of QObject extraction.
class OnboardLogModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_DISABLE_COPY_MOVE(OnboardLogModel)

public:
    enum Role : int
    {
        ObjectRole = Qt::UserRole,
        IdRole,
        TimeRole,
        SizeRole,
        SizeStringRole,
        ReceivedRole,
        SelectedRole,
        StateRole,
        StatusRole,
        ErrorMessageRole,
    };
    Q_ENUM(Role)

    explicit OnboardLogModel(QObject* parent = nullptr);
    ~OnboardLogModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return _entries.size(); }

    bool isEmpty() const { return _entries.isEmpty(); }

    OnboardLogEntry* at(int index) const;

    bool contains(const OnboardLogEntry* entry) const { return _rowByEntry.contains(entry); }

    template <typename T>
    T value(int index) const
    {
        return qobject_cast<T>(at(index));
    }

    void append(const QList<OnboardLogEntry*>& entries);

    void append(OnboardLogEntry* entry) { append(QList<OnboardLogEntry*>{entry}); }

    OnboardLogEntry* removeOne(const OnboardLogEntry* entry);
    void clearAndDeleteContents();

public slots:
    void clear();

signals:
    void countChanged();

private:
    enum class PendingChangeType : uint8_t
    {
        Append,
        Remove,
        Clear,
        ClearAndDelete,
    };

    struct PendingChange
    {
        PendingChangeType type = PendingChangeType::Clear;
        QList<QPointer<OnboardLogEntry>> entries;
    };

    void _connectEntry(OnboardLogEntry* entry);
    void _emitEntryChanged(OnboardLogEntry* entry, const QList<int>& roles);
    bool _emitPendingEntryChanges();
    bool _finishMutation(bool countChanged);
    void _rebuildRowLookup(int firstRow = 0);
    void _removeNullEntries();
    void _clearAndDeleteContents(const QList<QPointer<OnboardLogEntry>>& additionalEntries);
    void _queuePendingChange(PendingChangeType type, const QList<OnboardLogEntry*>& entries = {});
    void _applyPendingChanges();

    QList<QPointer<OnboardLogEntry>> _entries;
    QHash<const OnboardLogEntry*, int> _rowByEntry;
    QHash<OnboardLogEntry*, QSet<int>> _pendingEntryChanges;
    QList<PendingChange> _pendingChanges;
    bool _modelChangeInProgress = false;
    bool _nullCleanupPending = false;
    bool _applyingPendingChanges = false;
};
