#include "GPSRecordingTransport.h"

#include <algorithm>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingTransportLog, "GPS.Transport.GPSRecordingTransport")

GPSRecordingTransport::GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, const QString& file,
                                             qint64 maxFileBytes, std::function<void()> stopped)
    : GPSTransport(transport ? transport->cancelToken() : GPSCancelToken{})
    , _transport(std::move(transport))
    , _maxFileBytes(maxFileBytes)
    , _stopped(std::move(stopped))
    , _file(file)
{
    qCDebug(GPSRecordingTransportLog) << this;
}

GPSRecordingTransport::~GPSRecordingTransport()
{
    _finish();
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

unsigned GPSRecordingTransport::fixedBaudrate() const
{
    return _transport ? _transport->fixedBaudrate() : 0;
}

GPSReadResult GPSRecordingTransport::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    if (!_transport) {
        return {GPSReadStatus::Closed};
    }
    GPSReadResult result = _transport->read(buffer, timeout);
    if (result.status == GPSReadStatus::Data && result.bytesRead > 0 &&
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

bool GPSRecordingTransport::setBaudrate(unsigned baudrate)
{
    if (!_transport || !_transport->setBaudrate(baudrate)) {
        return false;
    }
    _record(BAUD, QByteArray::number(baudrate));
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
    if (!accepted.isEmpty()) {
        _record(SENT, accepted.toByteArray().toHex());
    }
    _flushIfDue();
    return result;
}

void GPSRecordingTransport::_start()
{
    const QString directory = QFileInfo(_file).absolutePath();
    if (!QDir().mkpath(directory)) {
        _fail(QStringLiteral("Cannot create the folder %1").arg(directory));
        return;
    }
    if (!_file.open(QIODevice::WriteOnly)) {
        _fail(_file.errorString());
        return;
    }
    _clock.start();
    _nextFlush.setRemainingTime(FLUSH_INTERVAL, Qt::CoarseTimer);
    qCDebug(GPSRecordingTransportLog) << "Recording receiver data to" << _file.fileName();
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
    // Whole records only, so a recording that reached its limit still ends with a complete line.
    if (_bytes + line.size() > _maxFileBytes) {
        qCWarning(GPSRecordingTransportLog) << "Receiver recording reached its size limit of" << _maxFileBytes
                                            << "bytes and stopped:" << _file.fileName();
        _finish();
        _notifyStopped();
        return;
    }
    if (_file.write(line) != line.size()) {
        _fail(_file.errorString());
        return;
    }
    _bytes += line.size();
}

void GPSRecordingTransport::_flushIfDue()
{
    if (!recording() || !_nextFlush.hasExpired()) {
        return;
    }
    _nextFlush.setRemainingTime(FLUSH_INTERVAL, Qt::CoarseTimer);
    if (!_file.flush()) {
        _fail(_file.errorString());
    }
}

void GPSRecordingTransport::_fail(const QString& error)
{
    qCWarning(GPSRecordingTransportLog) << "Receiver recording stopped after a file error:" << _file.fileName()
                                        << error;
    _file.close();
    _notifyStopped();
}

void GPSRecordingTransport::_finish()
{
    if (!recording()) {
        return;
    }
    if (!_file.flush()) {
        _fail(_file.errorString());
        return;
    }
    _file.close();
}

void GPSRecordingTransport::_notifyStopped()
{
    if (const std::function<void()> stopped = std::exchange(_stopped, {})) {
        stopped();
    }
}
