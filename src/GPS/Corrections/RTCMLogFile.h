#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "GPSBoundedFile.h"

class QDateTime;

/// Logs RTCM 3 frames to a file, byte for byte and one after the other: a raw RTCM 3 stream, which other tools read.
/// The file is a GPSBoundedFile that keeps the frames that fit in maxFileBytes: reaching that limit or a file error
/// logs one warning and closes the log; the caller sees it in the result of the call and carries on without it.
/// A log of corrections contains the base station's position.
class RTCMLogFile
{
public:
    /// About three days of a correction stream of 1 KB/s.
    static constexpr qint64 MAX_FILE_BYTES = qint64{256} * 1024 * 1024;

    explicit RTCMLogFile(qint64 maxFileBytes = MAX_FILE_BYTES);
    ~RTCMLogFile();
    Q_DISABLE_COPY_MOVE(RTCMLogFile)

    /// The log in @a directory of a stream logged from @a startedAt on: <yyyyMMdd>T<HHmmss.zzz>Z.rtcm3, in UTC. The
    /// milliseconds keep logs that follow each other in files of their own.
    [[nodiscard]] static QString fileName(const QString& directory, const QDateTime& startedAt);

    /// Starts @a file, creating its folder, in place of the log that was open. A file that exists is an earlier log
    /// and is kept, so the log does not start. Returns whether it is open.
    bool open(const QString& file);

    /// Appends @a frame, whole or not at all. Returns whether the log is still open.
    bool write(const QByteArray& frame) { return _file.write(frame); }

    /// Writes the buffered frames out. Returns whether the log is still open.
    bool flush() { return _file.flush(); }

    void close() { _file.close(); }

    [[nodiscard]] bool isOpen() const { return _file.isOpen(); }

    /// The path of the open log; empty while none is open.
    [[nodiscard]] QString file() const { return _file.file(); }

private:
    GPSBoundedFile _file;
};
