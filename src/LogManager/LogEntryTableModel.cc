#include "LogEntryTableModel.h"

int LogEntryTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(LogEntry::ColumnCount);
}

QVariant LogEntryTableModel::data(const QModelIndex& index, int role) const
{
    const LogEntry* entry = entryAt(index.row());
    if (!entry) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        return entry->columnDisplayData(index.column());
    }

    return entry->roleData(role);
}

void LogEntryTableModel::multiData(const QModelIndex& index, QModelRoleDataSpan roleDataSpan) const
{
    const LogEntry* entry = entryAt(index.row());
    if (!entry) {
        return;
    }

    for (auto& roleData : roleDataSpan) {
        const int role = roleData.role();
        if (role == Qt::DisplayRole) {
            roleData.setData(entry->columnDisplayData(index.column()));
        } else {
            roleData.setData(entry->roleData(role));
        }
    }
}

QVariant LogEntryTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    return LogEntry::columnHeaderData(section);
}

QHash<int, QByteArray> LogEntryTableModel::roleNames() const
{
    return LogEntry::roleNames();
}
