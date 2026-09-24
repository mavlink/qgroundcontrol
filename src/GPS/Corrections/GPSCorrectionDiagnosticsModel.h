#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>

/// Keyed diagnostics rows updated in place so views keep their delegates between refreshes.
class GPSCorrectionDiagnosticsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role
    {
        RowRole = Qt::UserRole + 1,
    };
    Q_ENUM(Role)

    explicit GPSCorrectionDiagnosticsModel(QString keyField, QObject* parent = nullptr);
    ~GPSCorrectionDiagnosticsModel() override;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = RowRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Applies @a rows by key: removes stale rows, inserts or moves new ones, and emits dataChanged once
    /// for the span of rows whose contents changed.
    void setRows(const QVariantList& rows);

    const QList<QVariantMap>& rows() const { return _rows; }

private:
    QString _key(const QVariantMap& row) const { return row.value(_keyField).toString(); }

    const QString _keyField;
    QList<QVariantMap> _rows;
};
