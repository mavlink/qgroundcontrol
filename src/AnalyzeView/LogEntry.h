/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QBitArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(LogEntryLog)

//-----------------------------------------------------------------------------
class QGCLogEntry : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(uint         id          READ id                             CONSTANT)
    Q_PROPERTY(QDateTime    time        READ time                           NOTIFY timeChanged)
    Q_PROPERTY(uint         size        READ size                           NOTIFY sizeChanged)
    Q_PROPERTY(QString      sizeStr     READ sizeStr                        NOTIFY sizeChanged)
    Q_PROPERTY(bool         received    READ received                       NOTIFY receivedChanged)
    Q_PROPERTY(bool         selected    READ selected   WRITE setSelected   NOTIFY selectedChanged)
    Q_PROPERTY(QString      status      READ status                         NOTIFY statusChanged)

public:
    QGCLogEntry(uint logId, const QDateTime& dateTime = QDateTime(), uint logSize = 0, bool received = false);

    uint        id          () const { return _logID; }
    uint        size        () const { return _logSize; }
    QString     sizeStr     () const;
    QDateTime   time        () const { return _logTimeUTC; }
    bool        received    () const { return _received; }
    bool        selected    () const { return _selected; }
    QString     status      () const { return _status; }

    void        setId       (uint id_)          { _logID = id_; }
    void        setSize     (uint size_)        { _logSize = size_;     emit sizeChanged(); }
    void        setTime     (QDateTime date_)   { _logTimeUTC = date_;  emit timeChanged(); }
    void        setReceived (bool rec_)         { _received = rec_;     emit receivedChanged(); }
    void        setSelected (bool sel_)         { _selected = sel_;     emit selectedChanged(); }
    void        setStatus   (QString stat_)     { _status = stat_;      emit statusChanged(); }

signals:
    void        idChanged       ();
    void        timeChanged     ();
    void        sizeChanged     ();
    void        receivedChanged ();
    void        selectedChanged ();
    void        statusChanged   ();

private:
    uint        _logID;
    uint        _logSize;
    QDateTime   _logTimeUTC;
    bool        _received;
    bool        _selected;
    QString     _status;
};

struct LogDownloadData {
    LogDownloadData(QGCLogEntry* entry);

    QBitArray     chunk_table;
    uint32_t      current_chunk;
    QFile         file;
    QString       filename;
    uint          ID;
    QGCLogEntry*  entry;
    uint          written;
    size_t        rate_bytes;
    qreal         rate_avg;
    QElapsedTimer elapsed;

    void advanceChunk();
    uint32_t chunkBins() const;
    uint32_t numChunks() const;
    bool chunkEquals(const bool val) const;
};
