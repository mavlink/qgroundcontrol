#include "RTCMMessageCountModel.h"

#include <algorithm>

namespace {
constexpr double MILLISECONDS_PER_SECOND = 1000.;
/// A frame is overdue once this many of its intervals and the margin have passed without it, which tolerates frames
/// that a network delivers late and together.
constexpr double OVERDUE_INTERVALS = 3.;
constexpr double OVERDUE_MARGIN_MS = 1000.;
}  // namespace

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
    if (role == IntervalRole) {
        return row.intervalMs() / MILLISECONDS_PER_SECOND;
    }
    return {};
}

QHash<int, QByteArray> RTCMMessageCountModel::roleNames() const
{
    return {{MessageIdRole, "messageId"}, {CountRole, "count"}, {IntervalRole, "interval"}};
}

double RTCMMessageCountModel::Row::intervalMs() const
{
    if (receipts.isEmpty()) {
        return 0.;
    }
    const auto later = std::ranges::count_if(receipts, [this](qint64 receipt) { return receipt > receipts.first(); });
    return later > 0 ? static_cast<double>(receipts.last() - receipts.first()) / static_cast<double>(later) : 0.;
}

void RTCMMessageCountModel::record(int messageId, qint64 receivedAtMs)
{
    const auto position = std::ranges::lower_bound(_rows, messageId, {}, &Row::messageId);
    const int row = static_cast<int>(position - _rows.begin());
    if (position != _rows.end() && position->messageId == messageId) {
        ++position->count;
        if (!position->receipts.isEmpty() && receivedAtMs < position->receipts.last()) {
            position->receipts.clear();
        } else if (position->receipts.size() == INTERVAL_WINDOW) {
            position->receipts.erase(position->receipts.begin());
        }
        position->receipts.append(receivedAtMs);
        emit dataChanged(index(row), index(row), {CountRole, IntervalRole});
        return;
    }
    beginInsertRows(QModelIndex(), row, row);
    _rows.insert(position, {.messageId = messageId, .count = 1, .receipts = {receivedAtMs}});
    endInsertRows();
}

void RTCMMessageCountModel::expireIntervals(qint64 nowMs)
{
    for (int row = 0; row < _rows.size(); ++row) {
        Row& counted = _rows[row];
        const double interval = counted.intervalMs();
        if (interval > 0. &&
            static_cast<double>(nowMs - counted.receipts.last()) > interval * OVERDUE_INTERVALS + OVERDUE_MARGIN_MS) {
            counted.receipts.clear();
            emit dataChanged(index(row), index(row), {IntervalRole});
        }
    }
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
