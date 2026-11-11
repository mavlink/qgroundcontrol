#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>

/// Frames received per RTCM message ID, ascending by ID; ID 0 counts unidentified frames. Rows change in place: a new
/// ID inserts its row and a repeated one changes only that row's count, so a view keeps its delegates.
class RTCMMessageCountModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        MessageIdRole = Qt::UserRole,
        CountRole,
    };

    explicit RTCMMessageCountModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Counts one frame of @a messageId.
    void record(int messageId);
    void clear();

private:
    struct Row
    {
        int messageId = 0;
        quint64 count = 0;
    };

    QList<Row> _rows;
};
