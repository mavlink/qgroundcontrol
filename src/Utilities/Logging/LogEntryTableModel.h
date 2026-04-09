#pragma once

#include <QtCore/QAbstractTableModel>
#include <QtQmlIntegration/QtQmlIntegration>

#include "LogEntry.h"

/// Base class for table models that display LogEntry data.
/// Provides shared data(), headerData(), roleNames(), and columnCount()
/// implementations. Subclasses supply entries via entryAt().
class LogEntryTableModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    using QAbstractTableModel::QAbstractTableModel;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void multiData(const QModelIndex& index, QModelRoleDataSpan roleDataSpan) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

protected:
    /// Return entry at visible row, or nullptr if out of range.
    [[nodiscard]] virtual const LogEntry* entryAt(int row) const = 0;
};
