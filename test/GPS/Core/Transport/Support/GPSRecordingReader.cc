#include "GPSRecordingReader.h"

#include <algorithm>

#include <QtCore/QFile>
#include <QtCore/QIODevice>

#include "GPSRecordingTransport.h"

namespace GPSTest {

namespace {
/// Whole bytes as the recorder writes them, in lower-case hexadecimal.
bool hexadecimal(const QByteArray& value)
{
    return !value.isEmpty() && value.size() % 2 == 0 && std::ranges::all_of(value, [](char digit) {
        return (digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f');
    });
}

/// @a line without its line ending, or nothing when it has none.
std::optional<QByteArray> terminated(QByteArray line)
{
    if (!line.endsWith('\n')) {
        return std::nullopt;
    }
    line.chop(line.endsWith("\r\n") ? 2 : 1);
    return line;
}
}  // namespace

GPSRecordingReader::GPSRecordingReader(QIODevice& device)
    : _device(device)
{}

bool GPSRecordingReader::readHeader()
{
    // The header, a CR and the LF.
    const QByteArray start = _device.peek(sizeof(GPSRecordingTransport::FILE_HEADER) + 1);
    const std::optional<QByteArray> header = terminated(start.first(start.indexOf('\n') + 1));
    if (header != QByteArray(GPSRecordingTransport::FILE_HEADER)) {
        return false;
    }
    (void) _device.skip(start.indexOf('\n') + 1);
    return true;
}

std::optional<GPSRecordingRecord> GPSRecordingReader::next()
{
    if (!_error.isEmpty() || _device.atEnd()) {
        return std::nullopt;
    }
    ++_line;
    const std::optional<QByteArray> line = terminated(_device.readLine());
    if (!line) {
        return _fail(QStringLiteral("the recording ends within a record"));
    }
    const QByteArrayList fields = line->split(' ');
    if (fields.size() != 3) {
        return _fail(QStringLiteral("not a time, a kind and a value"));
    }
    bool timed = false;
    GPSRecordingRecord record;
    record.time = std::chrono::microseconds(fields[0].toLongLong(&timed));
    if (!timed || record.time < _latest) {
        return _fail(QStringLiteral("not a time at or after the time of the record before it"));
    }
    const QByteArray& value = fields[2];
    if (fields[1] == GPSRecordingTransport::BAUD) {
        bool valid = false;
        record.kind = GPSRecordingRecord::Kind::BaudRate;
        record.baudRate = value.toUInt(&valid);
        if (!valid) {
            return _fail(QStringLiteral("not a baud rate"));
        }
    } else if (fields[1] == GPSRecordingTransport::RECEIVED || fields[1] == GPSRecordingTransport::SENT) {
        if (!hexadecimal(value)) {
            return _fail(QStringLiteral("not whole bytes in lower-case hexadecimal"));
        }
        record.kind = fields[1] == GPSRecordingTransport::SENT ? GPSRecordingRecord::Kind::Sent
                                                               : GPSRecordingRecord::Kind::Received;
        record.bytes = QByteArray::fromHex(value);
    } else {
        return _fail(QStringLiteral("unknown record kind"));
    }
    _latest = record.time;
    return record;
}

QList<GPSRecordingRecord> GPSRecordingReader::readFile(const QString& path, QString* error)
{
    QList<GPSRecordingRecord> records;
    QFile file(path);
    GPSRecordingReader reader(file);
    QString failure;
    if (!file.open(QIODevice::ReadOnly)) {
        failure = file.errorString();
    } else if (!reader.readHeader()) {
        failure = QStringLiteral("not a receiver recording");
    } else {
        while (auto record = reader.next()) {
            records.append(std::move(*record));
        }
        failure = file.error() == QFileDevice::NoError ? reader.error() : file.errorString();
    }
    if (error) {
        *error = failure;
    }
    return records;
}

std::optional<GPSRecordingRecord> GPSRecordingReader::_fail(const QString& reason)
{
    _error = QStringLiteral("line %1: %2").arg(_line).arg(reason);
    return std::nullopt;
}

}  // namespace GPSTest
