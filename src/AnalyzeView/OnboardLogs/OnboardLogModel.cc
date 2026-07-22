#include "OnboardLogModel.h"

#include <QtCore/QSet>
#include <algorithm>
#include <utility>

#include "OnboardLogEntry.h"

namespace {

constexpr int kMinimumTargetedNullRanges = 4;
constexpr int kMaximumTargetedNullRanges = 32;
constexpr int kEntriesPerTargetedNullRange = 100;
constexpr int kDenseNullEntryDivisor = 4;

}  // namespace

OnboardLogModel::OnboardLogModel(QObject* parent) : QAbstractListModel(parent) {}

OnboardLogModel::~OnboardLogModel() = default;

int OnboardLogModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : _entries.size();
}

QVariant OnboardLogModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || (index.column() != 0)) {
        return {};
    }
    OnboardLogEntry* const entry = at(index.row());
    if (!entry) {
        return {};
    }

    switch (role) {
        case ObjectRole:
            return QVariant::fromValue(static_cast<QObject*>(entry));
        case IdRole:
            return entry->id();
        case TimeRole:
            return entry->time();
        case SizeRole:
            return entry->size();
        case SizeStringRole:
            return entry->sizeStr();
        case ReceivedRole:
            return entry->received();
        case SelectedRole:
            return entry->selected();
        case StateRole:
            return QVariant::fromValue(entry->state());
        case StatusRole:
            return entry->status();
        case ErrorMessageRole:
            return entry->errorMessage();
        default:
            return {};
    }
}

QHash<int, QByteArray> OnboardLogModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        {ObjectRole, "object"},      {IdRole, "logId"},
        {TimeRole, "time"},          {SizeRole, "size"},
        {SizeStringRole, "sizeStr"}, {ReceivedRole, "received"},
        {SelectedRole, "selected"},  {StateRole, "state"},
        {StatusRole, "status"},      {ErrorMessageRole, "errorMessage"},
    };
    return roles;
}

OnboardLogEntry* OnboardLogModel::at(int index) const
{
    return ((index >= 0) && (index < _entries.size())) ? _entries.at(index).data() : nullptr;
}

void OnboardLogModel::append(const QList<OnboardLogEntry*>& entries)
{
    if (_modelChangeInProgress) {
        _queuePendingChange(PendingChangeType::Append, entries);
        return;
    }

    QList<QPointer<OnboardLogEntry>> validEntries;
    validEntries.reserve(entries.size());
    QSet<const OnboardLogEntry*> seenEntries;
    seenEntries.reserve(entries.size());
    for (OnboardLogEntry* const entry : entries) {
        if (entry && !_rowByEntry.contains(entry) && !seenEntries.contains(entry)) {
            validEntries.append(entry);
            seenEntries.insert(entry);
        }
    }

    if (validEntries.isEmpty()) {
        return;
    }

    const int first = _entries.size();
    for (const QPointer<OnboardLogEntry>& entry : std::as_const(validEntries)) {
        if (entry) {
            _connectEntry(entry);
        }
    }

    _modelChangeInProgress = true;
    const QPointer<OnboardLogModel> self(this);
    beginInsertRows({}, first, first + validEntries.size() - 1);
    if (!self) {
        return;
    }
    for (const QPointer<OnboardLogEntry>& entry : std::as_const(validEntries)) {
        _entries.append(entry);
        if (entry) {
            _rowByEntry.insert(entry.data(), _entries.size() - 1);
        }
    }
    endInsertRows();
    if (!self) {
        return;
    }
    (void) _finishMutation(true);
}

OnboardLogEntry* OnboardLogModel::removeOne(const OnboardLogEntry* entry)
{
    if (_modelChangeInProgress) {
        _queuePendingChange(PendingChangeType::Remove, {const_cast<OnboardLogEntry*>(entry)});
        return nullptr;
    }

    const auto rowIt = _rowByEntry.constFind(entry);
    if (rowIt == _rowByEntry.cend()) {
        return nullptr;
    }
    const int row = rowIt.value();

    QPointer<OnboardLogEntry> removed;
    _modelChangeInProgress = true;
    const QPointer<OnboardLogModel> self(this);
    beginRemoveRows({}, row, row);
    if (!self) {
        return nullptr;
    }
    removed = _entries.takeAt(row);
    _rowByEntry.remove(entry);
    if (removed) {
        disconnect(removed, nullptr, this, nullptr);
    }
    _rebuildRowLookup(row);
    endRemoveRows();
    if (!self) {
        return removed.data();
    }
    (void) _finishMutation(true);
    return removed.data();
}

void OnboardLogModel::clear()
{
    if (_modelChangeInProgress) {
        _queuePendingChange(PendingChangeType::Clear);
        return;
    }
    if (_entries.isEmpty()) {
        return;
    }

    _modelChangeInProgress = true;
    const QPointer<OnboardLogModel> self(this);
    beginResetModel();
    if (!self) {
        return;
    }
    for (const QPointer<OnboardLogEntry>& entry : std::as_const(_entries)) {
        if (entry) {
            disconnect(entry, nullptr, this, nullptr);
        }
    }
    _entries.clear();
    _rowByEntry.clear();
    endResetModel();
    if (!self) {
        return;
    }
    _nullCleanupPending = false;
    (void) _finishMutation(true);
}

void OnboardLogModel::clearAndDeleteContents()
{
    if (_modelChangeInProgress) {
        _queuePendingChange(PendingChangeType::ClearAndDelete);
        return;
    }

    _clearAndDeleteContents({});
}

void OnboardLogModel::_clearAndDeleteContents(const QList<QPointer<OnboardLogEntry>>& additionalEntries)
{
    QList<QPointer<OnboardLogEntry>> entries;
    entries.reserve(additionalEntries.size() + _entries.size());
    QSet<const OnboardLogEntry*> seenEntries;
    const auto appendUniqueEntries = [&entries, &seenEntries](const QList<QPointer<OnboardLogEntry>>& candidates) {
        for (const QPointer<OnboardLogEntry>& entry : candidates) {
            if (entry && !seenEntries.contains(entry.data())) {
                seenEntries.insert(entry.data());
                entries.append(entry);
            }
        }
    };
    appendUniqueEntries(additionalEntries);
    appendUniqueEntries(_entries);

    const bool wasApplyingPendingChanges = _applyingPendingChanges;
    _applyingPendingChanges = true;
    const QPointer<OnboardLogModel> self(this);
    clear();
    for (const QPointer<OnboardLogEntry>& entry : entries) {
        if (entry) {
            entry->deleteLater();
        }
    }
    if (!self) {
        return;
    }
    _applyingPendingChanges = wasApplyingPendingChanges;
    if (!wasApplyingPendingChanges) {
        _applyPendingChanges();
    }
}

void OnboardLogModel::_connectEntry(OnboardLogEntry* entry)
{
    connect(entry, &OnboardLogEntry::timeChanged, this, [this, entry]() { _emitEntryChanged(entry, {TimeRole}); });
    connect(entry, &OnboardLogEntry::sizeChanged, this,
            [this, entry]() { _emitEntryChanged(entry, {SizeRole, SizeStringRole}); });
    connect(entry, &OnboardLogEntry::receivedChanged, this,
            [this, entry]() { _emitEntryChanged(entry, {ReceivedRole}); });
    connect(entry, &OnboardLogEntry::selectedChanged, this,
            [this, entry]() { _emitEntryChanged(entry, {SelectedRole}); });
    connect(entry, &OnboardLogEntry::stateChanged, this, [this, entry]() { _emitEntryChanged(entry, {StateRole}); });
    connect(entry, &OnboardLogEntry::statusChanged, this, [this, entry]() { _emitEntryChanged(entry, {StatusRole}); });
    connect(entry, &OnboardLogEntry::errorMessageChanged, this,
            [this, entry]() { _emitEntryChanged(entry, {ErrorMessageRole}); });
    connect(entry, &QObject::destroyed, this, [this, entry]() {
        _pendingEntryChanges.remove(entry);
        if (_modelChangeInProgress) {
            _rowByEntry.remove(entry);
            _nullCleanupPending = true;
        } else {
            (void) removeOne(entry);
        }
    });
}

void OnboardLogModel::_emitEntryChanged(OnboardLogEntry* entry, const QList<int>& roles)
{
    const auto rowIt = _rowByEntry.constFind(entry);
    if (rowIt == _rowByEntry.cend()) {
        return;
    }

    if (_modelChangeInProgress) {
        QSet<int>& pendingRoles = _pendingEntryChanges[entry];
        for (const int role : roles) {
            pendingRoles.insert(role);
        }
        return;
    }

    const QModelIndex modelIndex = index(rowIt.value(), 0);
    emit dataChanged(modelIndex, modelIndex, roles);
}

bool OnboardLogModel::_emitPendingEntryChanges()
{
    const QHash<OnboardLogEntry*, QSet<int>> pendingChanges = std::exchange(_pendingEntryChanges, {});
    const QPointer<OnboardLogModel> self(this);
    _modelChangeInProgress = true;
    for (auto iterator = pendingChanges.cbegin(); iterator != pendingChanges.cend(); ++iterator) {
        const auto rowIt = _rowByEntry.constFind(iterator.key());
        if (rowIt == _rowByEntry.cend()) {
            continue;
        }

        QList<int> roles(iterator.value().cbegin(), iterator.value().cend());
        std::sort(roles.begin(), roles.end());
        const QModelIndex modelIndex = index(rowIt.value(), 0);
        emit dataChanged(modelIndex, modelIndex, roles);
        if (!self) {
            return false;
        }
    }
    _modelChangeInProgress = false;
    return true;
}

bool OnboardLogModel::_finishMutation(bool countChangedPending)
{
    const QPointer<OnboardLogModel> self(this);
    if (countChangedPending) {
        emit countChanged();
        if (!self) {
            return false;
        }
    }
    _modelChangeInProgress = false;
    do {
        _removeNullEntries();
        if (!self) {
            return false;
        }
        if (!_pendingEntryChanges.isEmpty() && !_emitPendingEntryChanges()) {
            return false;
        }
    } while (_nullCleanupPending || !_pendingEntryChanges.isEmpty());
    _applyPendingChanges();
    return self;
}

void OnboardLogModel::_rebuildRowLookup(int firstRow)
{
    for (int row = firstRow; row < _entries.size(); ++row) {
        const QPointer<OnboardLogEntry>& entry = _entries.at(row);
        if (entry) {
            _rowByEntry.insert(entry.data(), row);
        }
    }
}

void OnboardLogModel::_removeNullEntries()
{
    if (_modelChangeInProgress || !_nullCleanupPending) {
        return;
    }

    _modelChangeInProgress = true;
    const QPointer<OnboardLogModel> self(this);
    bool countChangedPending = false;
    do {
        while (_nullCleanupPending) {
            _nullCleanupPending = false;
            QList<std::pair<int, int>> nullRanges;
            int nullEntryCount = 0;
            for (int row = _entries.size() - 1; row >= 0;) {
                if (_entries.at(row)) {
                    --row;
                    continue;
                }

                const int last = row;
                while ((row >= 0) && !_entries.at(row)) {
                    --row;
                }
                nullRanges.append(std::make_pair(row + 1, last));
                nullEntryCount += last - row;
            }

            if (nullRanges.isEmpty()) {
                continue;
            }

            countChangedPending = true;
            const int targetedRangeLimit = std::clamp(static_cast<int>(_entries.size()) / kEntriesPerTargetedNullRange,
                                                      kMinimumTargetedNullRanges, kMaximumTargetedNullRanges);
            const bool highlyFragmented = nullRanges.size() > kMaximumTargetedNullRanges;
            const bool denselyFragmented = (nullEntryCount * kDenseNullEntryDivisor) >= _entries.size();
            const bool resetModel = (nullRanges.size() > targetedRangeLimit) && (highlyFragmented || denselyFragmented);
            if (resetModel) {
                beginResetModel();
                if (!self) {
                    return;
                }
                QList<QPointer<OnboardLogEntry>> liveEntries;
                liveEntries.reserve(_entries.size());
                for (const QPointer<OnboardLogEntry>& entry : std::as_const(_entries)) {
                    if (entry) {
                        liveEntries.append(entry);
                    }
                }
                _entries.swap(liveEntries);
                _rowByEntry.clear();
                _rebuildRowLookup();
                endResetModel();
                if (!self) {
                    return;
                }
            } else {
                for (const auto& [first, last] : std::as_const(nullRanges)) {
                    beginRemoveRows({}, first, last);
                    if (!self) {
                        return;
                    }
                    _entries.remove(first, last - first + 1);
                    _rebuildRowLookup(first);
                    endRemoveRows();
                    if (!self) {
                        return;
                    }
                }
            }
        }

        if (countChangedPending) {
            countChangedPending = false;
            emit countChanged();
            if (!self) {
                return;
            }
        }
    } while (_nullCleanupPending);
    _modelChangeInProgress = false;
}

void OnboardLogModel::_queuePendingChange(PendingChangeType type, const QList<OnboardLogEntry*>& entries)
{
    PendingChange change;
    change.type = type;
    if (type == PendingChangeType::ClearAndDelete) {
        change.entries = _entries;
    } else {
        change.entries.reserve(entries.size());
        for (OnboardLogEntry* const entry : entries) {
            if (entry) {
                change.entries.append(entry);
            }
        }
    }
    if ((type != PendingChangeType::Append && type != PendingChangeType::Remove) || !change.entries.isEmpty()) {
        _pendingChanges.append(std::move(change));
    }
}

void OnboardLogModel::_applyPendingChanges()
{
    if (_modelChangeInProgress || _applyingPendingChanges) {
        return;
    }

    _applyingPendingChanges = true;
    const QPointer<OnboardLogModel> self(this);
    while (!_pendingChanges.isEmpty()) {
        const PendingChange change = _pendingChanges.takeFirst();
        switch (change.type) {
            case PendingChangeType::Append: {
                QList<OnboardLogEntry*> entries;
                entries.reserve(change.entries.size());
                for (const QPointer<OnboardLogEntry>& entry : change.entries) {
                    if (entry) {
                        entries.append(entry.data());
                    }
                }
                append(entries);
                break;
            }
            case PendingChangeType::Remove:
                if (!change.entries.isEmpty() && change.entries.constFirst()) {
                    (void) removeOne(change.entries.constFirst());
                }
                break;
            case PendingChangeType::Clear:
                clear();
                break;
            case PendingChangeType::ClearAndDelete:
                _clearAndDeleteContents(change.entries);
                break;
        }
        if (!self) {
            return;
        }
    }
    _applyingPendingChanges = false;
}
