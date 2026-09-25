#pragma once

#include <algorithm>
#include <utility>

#include <QtCore/QRangeModel>
#include <QtCore/QSet>

#include "GPSCorrectionDiagnostics.h"

template <>
struct QRangeModel::RowOptions<GPSCorrectionSourceDiagnostic>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

template <>
struct QRangeModel::RowOptions<GPSCorrectionDestinationDiagnostic>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

/// Diagnostics rows with one role per row property. setRows() updates rows by key in place, so views keep their
/// delegates between refreshes.
template <typename Row>
class GPSCorrectionDiagnosticsModel final : public QRangeModel
{
public:
    // The base only records the address of the range; it reads rows after construction.
    explicit GPSCorrectionDiagnosticsModel(QObject* parent = nullptr)
        : QRangeModel(&_rows, parent)
    {}

    const QList<Row>& rows() const { return _rows; }

    // Rows change only through the owner's update functions.
    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        return QRangeModel::flags(index) & ~Qt::ItemIsEditable;
    }

    bool setData(const QModelIndex&, const QVariant&, int) override { return false; }

    bool setItemData(const QModelIndex&, const QMap<int, QVariant>&) override { return false; }

    bool clearItemData(const QModelIndex&) override { return false; }

    /// Removes stale rows, inserts or moves new ones, and emits dataChanged once for the span of changed rows.
    void setRows(QList<Row> next)
    {
        if (_rows.isEmpty() || next.isEmpty()) {
            if (!_rows.isEmpty()) {
                beginRemoveRows({}, 0, static_cast<int>(_rows.size() - 1));
                _rows.clear();
                endRemoveRows();
            }
            if (!next.isEmpty()) {
                beginInsertRows({}, 0, static_cast<int>(next.size() - 1));
                _rows = std::move(next);
                endInsertRows();
            }
            return;
        }
        QSet<QString> nextKeys;
        for (const auto& row : next) {
            nextKeys.insert(row.key());
        }
        for (qsizetype row = _rows.size() - 1; row >= 0; --row) {
            if (!nextKeys.contains(_rows.at(row).key())) {
                beginRemoveRows({}, static_cast<int>(row), static_cast<int>(row));
                _rows.removeAt(row);
                endRemoveRows();
            }
        }
        qsizetype firstChanged = -1;
        qsizetype lastChanged = -1;
        for (qsizetype row = 0; row < next.size(); ++row) {
            const QString key = next.at(row).key();
            const auto match = std::find_if(_rows.cbegin() + (std::min) (row, _rows.size()), _rows.cend(),
                                            [&key](const Row& existing) { return existing.key() == key; });
            if (match == _rows.cend()) {
                beginInsertRows({}, static_cast<int>(row), static_cast<int>(row));
                _rows.insert(row, next.at(row));
                endInsertRows();
                continue;
            }
            if (const qsizetype from = std::distance(_rows.cbegin(), match); from != row) {
                beginMoveRows({}, static_cast<int>(from), static_cast<int>(from), {}, static_cast<int>(row));
                _rows.move(from, row);
                endMoveRows();
            }
            if (!(_rows.at(row) == next.at(row))) {
                _rows[row] = next.at(row);
                firstChanged = firstChanged < 0 ? row : firstChanged;
                lastChanged = row;
            }
        }
        if (_rows.size() > next.size()) {
            beginRemoveRows({}, static_cast<int>(next.size()), static_cast<int>(_rows.size() - 1));
            _rows.resize(next.size());
            endRemoveRows();
        }
        if (firstChanged >= 0) {
            emit dataChanged(index(static_cast<int>(firstChanged), 0), index(static_cast<int>(lastChanged), 0));
        }
    }

private:
    QList<Row> _rows;
};
