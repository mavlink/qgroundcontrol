#include "OnboardLogModel.h"

#include <utility>

#include "OnboardLogEntry.h"

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
    return {
        {ObjectRole, "object"},      {IdRole, "logId"},
        {TimeRole, "time"},          {SizeRole, "size"},
        {SizeStringRole, "sizeStr"}, {ReceivedRole, "received"},
        {SelectedRole, "selected"},  {StateRole, "state"},
        {StatusRole, "status"},      {ErrorMessageRole, "errorMessage"},
    };
}

OnboardLogEntry* OnboardLogModel::at(int index) const
{
    return ((index >= 0) && (index < _entries.size())) ? _entries.at(index) : nullptr;
}

void OnboardLogModel::append(const QList<OnboardLogEntry*>& entries)
{
    QList<OnboardLogEntry*> validEntries;
    validEntries.reserve(entries.size());
    for (OnboardLogEntry* const entry : entries) {
        if (entry && !_entries.contains(entry) && !validEntries.contains(entry)) {
            validEntries.append(entry);
        }
    }

    if (validEntries.isEmpty()) {
        return;
    }

    const int first = _entries.size();
    beginInsertRows({}, first, first + validEntries.size() - 1);
    for (OnboardLogEntry* const entry : validEntries) {
        _entries.append(entry);
        _connectEntry(entry);
    }
    endInsertRows();
    emit countChanged();
}

OnboardLogEntry* OnboardLogModel::removeOne(const OnboardLogEntry* entry)
{
    const int row = _entries.indexOf(const_cast<OnboardLogEntry*>(entry));
    if (row < 0) {
        return nullptr;
    }

    beginRemoveRows({}, row, row);
    OnboardLogEntry* const removed = _entries.takeAt(row);
    if (removed) {
        disconnect(removed, nullptr, this, nullptr);
    }
    endRemoveRows();
    emit countChanged();
    return removed;
}

void OnboardLogModel::clear()
{
    if (_entries.isEmpty()) {
        return;
    }

    beginResetModel();
    for (OnboardLogEntry* const entry : std::as_const(_entries)) {
        if (entry) {
            disconnect(entry, nullptr, this, nullptr);
        }
    }
    _entries.clear();
    endResetModel();
    emit countChanged();
}

void OnboardLogModel::clearAndDeleteContents()
{
    const QList<OnboardLogEntry*> entries = _entries;
    clear();
    for (OnboardLogEntry* const entry : entries) {
        if (entry) {
            entry->deleteLater();
        }
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
    connect(entry, &QObject::destroyed, this, [this, entry]() { (void) removeOne(entry); });
}

void OnboardLogModel::_emitEntryChanged(OnboardLogEntry* entry, const QList<int>& roles)
{
    const int row = _entries.indexOf(entry);
    if (row >= 0) {
        const QModelIndex modelIndex = index(row, 0);
        emit dataChanged(modelIndex, modelIndex, roles);
    }
}
