#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QString>

#include "GPSTransport.h"
#include "GPSType.h"

class QDateTime;

/// Records a receiver session: every call passes through to the link it wraps, and what crossed the link goes to a
/// text file, which the test tooling replays (ReplayGPSTransport).
/// The file starts with FILE_HEADER on a line of its own. Every other line is one record, in the order the session
/// made the calls: the microseconds from the start of the recording to the return of the call, on a monotonic
/// clock, then one of
/// - RX and the bytes read from the receiver, in hexadecimal;
/// - TX and the bytes the link accepted for writing to the receiver, in hexadecimal;
/// - BAUD and the baud rate the link changed to.
/// The file is created when the link first opens, on the thread that owns the transport, and written through QFile's
/// buffer, which is flushed every FLUSH_INTERVAL and on destruction. It keeps the records that fit in maxFileBytes and
/// then stops growing, which bounds disk use without a retention policy.
/// A file error logs one warning and ends the recording. The link and its results are never affected.
/// A recording that ends before its session, after a file error or at the size limit, calls the stopped callback
/// once, on the thread that uses the transport.
/// Recordings contain the receiver's position.
class GPSRecordingTransport final : public GPSTransport
{
public:
    /// The first line of a recording; the number is the format version.
    static constexpr char FILE_HEADER[] = "QGC GPS receiver recording 1";
    static constexpr char RECEIVED[] = "RX";
    static constexpr char SENT[] = "TX";
    static constexpr char BAUD[] = "BAUD";

    /// About three hours of a 115200 baud link at full load.
    static constexpr qint64 MAX_FILE_BYTES = qint64{256} * 1024 * 1024;
    static constexpr std::chrono::milliseconds FLUSH_INTERVAL{1000};

    GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, const QString& file,
                          qint64 maxFileBytes = MAX_FILE_BYTES, std::function<void()> stopped = {});
    ~GPSRecordingTransport() override;

    /// The recording in @a directory of a session of @a type that starts at @a startedAt:
    /// <yyyyMMdd>T<HHmmss.zzz>Z-<type>.log, in UTC and with the type in lower case. The milliseconds keep sessions
    /// that follow each other at once in files of their own.
    [[nodiscard]] static QString sessionFile(const QString& directory, GPSType type, const QDateTime& startedAt);

    GPSOpenResult open() override;
    bool fatalError() const override;
    unsigned fixedBaudRate() const override;
    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;
    bool setBaudRate(unsigned baudRate) override;

    /// The file is still being written.
    [[nodiscard]] bool recording() const { return _file.isOpen(); }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;

private:
    void _start();
    void _record(const char* kind, const QByteArray& value);
    void _write(const QByteArray& line);
    void _flushIfDue();
    /// Ends the recording after a file error.
    void _fail(const QString& error);
    void _finish();
    void _notifyStopped();

    std::unique_ptr<GPSTransport> _transport;
    const qint64 _maxFileBytes;
    std::function<void()> _stopped;
    QFile _file;
    QElapsedTimer _clock;
    QDeadlineTimer _nextFlush;
    qint64 _bytes = 0;
    bool _started = false;
};
