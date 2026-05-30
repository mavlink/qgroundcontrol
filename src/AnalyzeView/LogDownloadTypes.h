/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QBitArray>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "MAVLinkLib.h"

class QGCLogEntry : public QObject
{
    Q_OBJECT

public:
    explicit QGCLogEntry(int id, QObject* parent = nullptr)
        : QObject(parent)
        , _id(id)
        , _received(false)
    {}

    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(uint32_t size READ size WRITE setSize)
    Q_PROPERTY(QDateTime time READ time WRITE setTime)
    Q_PROPERTY(bool received READ received WRITE setReceived)

    [[nodiscard]] int id() const { return _id; }
    [[nodiscard]] bool selected() const { return _selected; }
    void setSelected(bool selected);
    [[nodiscard]] QString status() const { return _status; }
    void setStatus(const QString& status);
    [[nodiscard]] uint32_t size() const { return _size; }
    void setSize(uint32_t size) { _size = size; }
    [[nodiscard]] QDateTime time() const { return _time; }
    void setTime(const QDateTime& time) { _time = time; }
    [[nodiscard]] bool received() const { return _received; }
    void setReceived(bool received) { _received = received; }

signals:
    void selectedChanged();
    void statusChanged();

private:
    int _id = 0;
    bool _selected = false;
    QString _status;
    uint32_t _size = 0;
    QDateTime _time;
    bool _received = false;
};

struct LogDownloadData
{
    static constexpr uint32_t kChunkSize = MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN * 8;

    explicit LogDownloadData(QGCLogEntry* entry)
        : entry(entry)
        , ID(entry->id())
    {}

    QGCLogEntry* entry = nullptr;
    int ID = 0;
    uint32_t current_chunk = 0;
    QBitArray chunk_table;
    QString filename;
    QFile file;

    [[nodiscard]] uint32_t chunkBins() const { return kChunkSize / MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN; }
};