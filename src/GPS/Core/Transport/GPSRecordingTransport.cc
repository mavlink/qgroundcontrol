#include "GPSRecordingTransport.h"

#include <algorithm>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QMetaEnum>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingTransportLog, "GPS.Transport.GPSRecordingTransport")

GPSRecordingTransport::GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, const QString& file,
                                             qint64 maxFileBytes, std::function<void()> stopped)
    : GPSTransport(transport ? transport->cancelToken() : GPSCancelToken{})
    , _transport(std::move(transport))
    , _path(file)
    , _stopped(std::move(stopped))
    , _file(&GPSRecordingTransportLog, "Receiver recording", maxFileBytes)
{
    qCDebug(GPSRecordingTransportLog) << this;
}

GPSRecordingTransport::~GPSRecordingTransport()
{
    if (recording() && !_file.flush()) {
        _notifyStopped();
    }
    _file.close();
    qCDebug(GPSRecordingTransportLog) << this;
}

QString GPSRecordingTransport::sessionFile(const QString& directory, GPSType type, const QDateTime& startedAt)
{
    const char* const name = QMetaEnum::fromType<GPSType>().valueToKey(static_cast<int>(type));
    return QDir(directory).filePath(QStringLiteral("%1-%2.log")
                                        .arg(startedAt.toUTC().toString(QStringLiteral("yyyyMMdd'T'HHmmss.zzz'Z'")),
                                             name ? QString::fromLatin1(name).toLower() : QStringLiteral("gnss")));
}

GPSOpenResult GPSRecordingTransport::open()
{
    if (!_transport) {
        return {GPSOpenStatus::Error, QStringLiteral("No receiver transport to record")};
    }
    GPSOpenResult result = _transport->open();
    if (result.status == GPSOpenStatus::Opened && !std::exchange(_started, true)) {
        _start();
    }
    return result;
}

bool GPSRecordingTransport::fatalError() const
{
    return !_transport || _transport->fatalError();
}

unsigned GPSRecordingTransport::fixedBaudRate() const
{
    return _transport ? _transport->fixedBaudRate() : 0;
}

GPSReadResult GPSRecordingTransport::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    if (!_transport) {
        return {GPSReadStatus::Closed};
    }
    GPSReadResult result = _transport->read(buffer, timeout);
    if (recording() && result.status == GPSReadStatus::Data && result.bytesRead > 0 &&
        static_cast<size_t>(result.bytesRead) <= buffer.size()) {
        _record(RECEIVED,
                QByteArray::fromRawData(reinterpret_cast<const char*>(buffer.data()), result.bytesRead).toHex());
    }
    _flushIfDue();
    return result;
}

std::chrono::milliseconds GPSRecordingTransport::configurationWriteTimeout() const
{
    return _transport ? _transport->configurationWriteTimeout() : GPSTransport::configurationWriteTimeout();
}

bool GPSRecordingTransport::setBaudRate(unsigned baudRate)
{
    if (!_transport || !_transport->setBaudRate(baudRate)) {
        return false;
    }
    _record(BAUD, QByteArray::number(baudRate));
    _flushIfDue();
    return true;
}

GPSWriteResult GPSRecordingTransport::writeData(QByteArrayView bytes, QDeadlineTimer deadline)
{
    if (!_transport) {
        return {GPSWriteStatus::Error};
    }
    GPSWriteResult result = _transport->write(bytes, deadline);
    const QByteArrayView accepted = bytes.first(std::clamp<qsizetype>(result.acceptedBytes, 0, bytes.size()));
    if (recording() && !accepted.isEmpty()) {
        _record(SENT, accepted.toByteArray().toHex());
    }
    _flushIfDue();
    return result;
}

void GPSRecordingTransport::_start()
{
    // Never over an earlier recording: session names differ by their start time alone.
    if (!_file.open(_path)) {
        _notifyStopped();
        return;
    }
    _clock.start();
    _nextFlush.setRemainingTime(FLUSH_INTERVAL, Qt::CoarseTimer);
    qCDebug(GPSRecordingTransportLog) << "Recording receiver data to" << _path;
    _write(QByteArray(FILE_HEADER) + '\n');
}

void GPSRecordingTransport::_record(const char* kind, const QByteArray& value)
{
    if (recording()) {
        _write(QByteArray::number(_clock.nsecsElapsed() / 1000) + ' ' + kind + ' ' + value + '\n');
    }
}

void GPSRecordingTransport::_write(const QByteArray& line)
{
    if (!_file.write(line)) {
        _notifyStopped();
    }
}

void GPSRecordingTransport::_flushIfDue()
{
    if (!recording() || !_nextFlush.hasExpired()) {
        return;
    }
    _nextFlush.setRemainingTime(FLUSH_INTERVAL, Qt::CoarseTimer);
    if (!_file.flush()) {
        _notifyStopped();
    }
}

void GPSRecordingTransport::_notifyStopped()
{
    if (const std::function<void()> stopped = std::exchange(_stopped, {})) {
        stopped();
    }
}
