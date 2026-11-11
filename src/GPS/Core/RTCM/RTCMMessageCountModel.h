#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QVarLengthArray>

/// Frames received per RTCM message ID, ascending by ID, and how often each ID arrives; ID 0 counts unidentified
/// frames. Rows change in place: a new ID inserts its row and a repeated one changes only that row's count and
/// interval, so a view keeps its delegates.
class RTCMMessageCountModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        MessageIdRole = Qt::UserRole,
        CountRole,
        /// Seconds between frames of the ID, averaged over its latest frames; zero while it is unknown.
        IntervalRole,
    };

    /// The latest frames of an ID its interval is averaged over.
    static constexpr int INTERVAL_WINDOW = 8;

    explicit RTCMMessageCountModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Counts one frame of @a messageId, received at @a receivedAtMs on a clock that never goes back.
    void record(int messageId, qint64 receivedAtMs);
    /// Forgets the interval of each ID whose next frame is overdue at @a nowMs: it no longer arrives that often.
    void expireIntervals(qint64 nowMs);
    void clear();

private:
    struct Row
    {
        int messageId = 0;
        quint64 count = 0;
        /// When the latest frames arrived, oldest first.
        QVarLengthArray<qint64, INTERVAL_WINDOW> receipts;

        /// Milliseconds between frames; zero until a frame follows the oldest one. Frames that arrive together, as
        /// several epochs in one network read do, share the time since the frames before them.
        double intervalMs() const;
    };

    QList<Row> _rows;
};
