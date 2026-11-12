#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

class QIODevice;

namespace GPSTest {

/// One record of a receiver recording (GPSRecordingTransport): what crossed the link, in which direction, and when.
struct GPSRecordingRecord
{
    enum class Kind : uint8_t
    {
        Received,  ///< Bytes the session read from the receiver.
        Sent,      ///< Bytes the link accepted from the session for the receiver.
        BaudRate,  ///< The baud rate the session changed the link to.
    };

    /// From the start of the recording to the return of the recorded call.
    std::chrono::microseconds time{0};
    Kind kind = Kind::Received;
    /// Of a Received or Sent record.
    QByteArray bytes;
    /// Of a BaudRate record.
    unsigned baudRate = 0;

    bool operator==(const GPSRecordingRecord&) const = default;
};

/// The reader of the receiver recording format, which the replay transport and the tests share. It reads a device
/// that is open for reading record by record, so a recording is never held whole, and returns both directions and
/// the baud rate changes in file order with their times, which never decrease.
/// A line that is not a record ends the reading with an error that names the line. So does a last line without its
/// line ending: the recorder buffers its file, so the recording of a session that was killed can end within a
/// record. Lines may end in CR LF, as after a transfer that converted them.
class GPSRecordingReader
{
public:
    explicit GPSRecordingReader(QIODevice& device);

    /// Whether the device holds a recording, whose header line this consumes. Nothing is consumed otherwise, as of a
    /// file of raw receiver output.
    [[nodiscard]] bool readHeader();

    /// The next record; nothing at the end of the recording and after an error.
    [[nodiscard]] std::optional<GPSRecordingRecord> next();

    /// What ended the reading before the end of the recording; empty otherwise.
    [[nodiscard]] const QString& error() const { return _error; }

    /// The records of the recording at @a path up to its end or its first error, which @a error receives: a line
    /// that is not a record, or a file that cannot be read or is not a recording.
    [[nodiscard]] static QList<GPSRecordingRecord> readFile(const QString& path, QString* error = nullptr);

private:
    std::optional<GPSRecordingRecord> _fail(const QString& reason);

    QIODevice& _device;
    QString _error;
    /// The header is line 1.
    qint64 _line = 1;
    std::chrono::microseconds _latest{0};
};

}  // namespace GPSTest
