#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkLib.h"

struct GapRange {
    uint32_t offset = 0;
    uint32_t length = 0;
};

class QGCLogEntry;

Q_DECLARE_LOGGING_CATEGORY(LogEntryLog)

struct LogDownloadData
{
    explicit LogDownloadData(QGCLogEntry * const logEntry);
    ~LogDownloadData();

    /// Record a gap in received data
    void recordGap(uint32_t gapOffset, uint32_t gapLength);

    /// Remove or shrink a gap as retried data arrives
    void fillGap(uint32_t dataOffset, uint32_t dataLength);

    /// True if there are any recorded gaps
    bool hasGaps() const { return !gaps.isEmpty(); }

    /// True if all expected data has been received with no gaps
    bool isComplete() const;

    uint ID = 0;
    QGCLogEntry *const entry = nullptr;

    uint32_t expectedOffset = 0;    ///< Next expected byte offset in the stream
    QList<GapRange> gaps;           ///< Missing byte ranges detected during download

    QFile file;
    QString filename;
    uint written = 0;
    uint last_status_written = 0;
    size_t rate_bytes = 0;
    qreal rate_avg = 0.;
    QElapsedTimer elapsed;

    /// Maximum bytes to request: largest multiple of packet size that fits in uint32_t
    static constexpr uint32_t kMaxRequestCount = (UINT32_MAX / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN) * MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
};

/*===========================================================================*/

class QGCLogEntry : public QObject
{
    Q_OBJECT
    // QML_ELEMENT

    Q_PROPERTY(uint         id          READ id                             NOTIFY idChanged)
    Q_PROPERTY(QDateTime    time        READ time                           NOTIFY timeChanged)
    Q_PROPERTY(uint         size        READ size                           NOTIFY sizeChanged)
    Q_PROPERTY(QString      sizeStr     READ sizeStr                        NOTIFY sizeChanged)
    Q_PROPERTY(bool         received    READ received                       NOTIFY receivedChanged)
    Q_PROPERTY(bool         selected    READ selected   WRITE setSelected   NOTIFY selectedChanged)
    Q_PROPERTY(QString      status      READ status                         NOTIFY statusChanged)

public:
    explicit QGCLogEntry(uint logId, const QDateTime &dateTime = QDateTime(), uint logSize = 0, bool received = false, QObject *parent = nullptr);
    ~QGCLogEntry();

    uint id() const { return _logID; }
    uint size() const { return _logSize; }
    QString sizeStr() const;
    QDateTime time() const { return _logTimeUTC; }
    bool received() const { return _received; }
    bool selected() const { return _selected; }
    QString status() const { return _status; }

    void setId(uint id) { if (id != _logID) { _logID = id; emit idChanged(); } }
    void setSize(uint size) { if (size != _logSize) { _logSize = size; emit sizeChanged(); } }
    void setTime(const QDateTime &date) { if (date != _logTimeUTC) {_logTimeUTC = date; emit timeChanged(); } }
    void setReceived(bool rec) { if (rec != _received) { _received = rec; emit receivedChanged(); } }
    void setSelected(bool sel) { if (sel != _selected) { _selected = sel; emit selectedChanged(); } }
    void setStatus(const QString &stat) { if (stat != _status) { _status = stat; emit statusChanged(); } }

signals:
    void idChanged();
    void timeChanged();
    void sizeChanged();
    void receivedChanged();
    void selectedChanged();
    void statusChanged();

private:
    uint _logID = 0;
    uint _logSize = 0;
    QDateTime _logTimeUTC;
    bool _received = false;
    bool _selected = false;
    QString _status = QStringLiteral("Pending");
};
