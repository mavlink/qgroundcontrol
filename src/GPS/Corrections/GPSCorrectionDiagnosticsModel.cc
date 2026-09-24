#include "GPSCorrectionDiagnosticsModel.h"

#include <algorithm>
#include <utility>

#include <QtCore/QSet>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionDiagnosticsModelLog, "GPS.Corrections.GPSCorrectionDiagnosticsModel")

GPSCorrectionDiagnosticsModel::GPSCorrectionDiagnosticsModel(QString keyField, QObject* parent)
    : QAbstractListModel(parent)
    , _keyField(std::move(keyField))
{
    qCDebug(GPSCorrectionDiagnosticsModelLog) << this;
}

GPSCorrectionDiagnosticsModel::~GPSCorrectionDiagnosticsModel()
{
    qCDebug(GPSCorrectionDiagnosticsModelLog) << this;
}

int GPSCorrectionDiagnosticsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(_rows.size());
}

QVariant GPSCorrectionDiagnosticsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0 || index.row() < 0 ||
        index.row() >= _rows.size() || role != RowRole) {
        return {};
    }
    return _rows.at(index.row());
}

QHash<int, QByteArray> GPSCorrectionDiagnosticsModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{{RowRole, QByteArrayLiteral("row")}};
    return roles;
}

void GPSCorrectionDiagnosticsModel::setRows(const QVariantList& rows)
{
    QList<QVariantMap> next;
    next.reserve(rows.size());
    QSet<QString> nextKeys;
    for (const auto& row : rows) {
        next.append(row.toMap());
        nextKeys.insert(_key(next.constLast()));
    }

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

    for (qsizetype row = _rows.size() - 1; row >= 0; --row) {
        if (!nextKeys.contains(_key(_rows.at(row)))) {
            beginRemoveRows({}, static_cast<int>(row), static_cast<int>(row));
            _rows.removeAt(row);
            endRemoveRows();
        }
    }

    qsizetype firstChanged = -1;
    qsizetype lastChanged = -1;
    for (qsizetype row = 0; row < next.size(); ++row) {
        const QString key = _key(next.at(row));
        const auto match = std::find_if(_rows.cbegin() + (std::min) (row, _rows.size()), _rows.cend(),
                                        [this, &key](const QVariantMap& existing) { return _key(existing) == key; });
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
        if (_rows.at(row) != next.at(row)) {
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
        emit dataChanged(index(static_cast<int>(firstChanged)), index(static_cast<int>(lastChanged)), {RowRole});
    }
}
