#include "RTCMMessageCountModel.h"

#include <algorithm>

RTCMMessageCountModel::RTCMMessageCountModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int RTCMMessageCountModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(_rows.size());
}

QVariant RTCMMessageCountModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= _rows.size()) {
        return {};
    }
    const Row& row = _rows.at(index.row());
    if (role == MessageIdRole) {
        return row.messageId;
    }
    if (role == CountRole) {
        return row.count;
    }
    return {};
}

QHash<int, QByteArray> RTCMMessageCountModel::roleNames() const
{
    return {{MessageIdRole, "messageId"}, {CountRole, "count"}};
}

void RTCMMessageCountModel::record(int messageId)
{
    const auto position = std::ranges::lower_bound(_rows, messageId, {}, &Row::messageId);
    const int row = static_cast<int>(position - _rows.begin());
    if (position != _rows.end() && position->messageId == messageId) {
        ++position->count;
        emit dataChanged(index(row), index(row), {CountRole});
        return;
    }
    beginInsertRows(QModelIndex(), row, row);
    _rows.insert(position, {messageId, 1});
    endInsertRows();
}

void RTCMMessageCountModel::clear()
{
    if (_rows.isEmpty()) {
        return;
    }
    beginResetModel();
    _rows.clear();
    endResetModel();
}
