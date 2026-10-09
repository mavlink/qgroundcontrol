#include "GPSRecordingTransport.h"

#include <algorithm>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaEnum>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingTransportLog, "Test.GPS.Transport.GPSRecordingTransport")

namespace GPSTest {

GPSRecordingTransport::GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, Files files, qint64 maxFileBytes)
    : GPSTransport(transport ? transport->cancelToken() : GPSCancelToken{})
    , _transport(std::move(transport))
    , _maxFileBytes(std::max<qint64>(maxFileBytes, 0))
{
    _received.file.setFileName(files.received);
    _sent.file.setFileName(files.sent);
    qCDebug(GPSRecordingTransportLog) << this;
}

GPSRecordingTransport::~GPSRecordingTransport()
{
    _finish(_received);
    _finish(_sent);
    qCDebug(GPSRecordingTransportLog) << this;
}

GPSRecordingTransport::Files GPSRecordingTransport::sessionFiles(const QString& directory, GPSType type,
                                                                 const QDateTime& startedAt)
{
    const QDir folder(directory);
    const char* const family = QMetaEnum::fromType<GPSType>().valueToKey(static_cast<int>(type));
    const QString stem = QStringLiteral("gps-%1-%2")
                             .arg(family ? QLatin1StringView(family) : QLatin1StringView("gnss"),
                                  startedAt.toString(QStringLiteral("yyyyMMdd-HHmmss")));
    for (int session = 1;; ++session) {
        const QString name = session == 1 ? stem : QStringLiteral("%1-%2").arg(stem).arg(session);
        Files files{folder.filePath(QStringLiteral("%1-rx.%2").arg(name, receivedExtension(type))),
                    folder.filePath(QStringLiteral("%1-tx.bin").arg(name))};
        if (!QFileInfo::exists(files.received) && !QFileInfo::exists(files.sent)) {
            return files;
        }
    }
}

QString GPSRecordingTransport::receivedExtension(GPSType type)
{
    switch (type) {
        case GPSType::ublox:
            return QStringLiteral("ubx");
        case GPSType::septentrio:
            return QStringLiteral("sbf");
        case GPSType::trimble:
        case GPSType::quectel:
        case GPSType::passive:
            return QStringLiteral("nmea");
        case GPSType::femto:
        case GPSType::unicore:
        case GPSType::automatic:
            break;
    }
    return QStringLiteral("bin");
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
        _record(_received, buffer.first(static_cast<size_t>(result.bytesRead)));
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
    return _transport && _transport->setBaudrate(baudrate);
}

bool GPSRecordingTransport::recording() const
{
    return _received.active || _sent.active;
}

GPSWriteResult GPSRecordingTransport::writeData(QByteArrayView bytes, QDeadlineTimer deadline)
{
    if (!_transport) {
        return {GPSWriteStatus::Error};
    }
    GPSWriteResult result = _transport->write(bytes, deadline);
    _record(_sent, bytes.first(std::clamp<qsizetype>(result.acceptedBytes, 0, bytes.size())));
    _flushIfDue();
    return result;
}

void GPSRecordingTransport::_start()
{
    for (Capture* const capture : {&_received, &_sent}) {
        const QString directory = QFileInfo(capture->file.fileName()).absolutePath();
        if (!QDir().mkpath(directory)) {
            _fail(directory, QStringLiteral("Cannot create the folder"));
            return;
        }
        if (!capture->file.open(QIODevice::WriteOnly)) {
            _fail(capture->file.fileName(), capture->file.errorString());
            return;
        }
        capture->active = true;
    }
    _nextFlush.setRemainingTime(FLUSH_INTERVAL, Qt::CoarseTimer);
    qCDebug(GPSRecordingTransportLog) << "Recording receiver data to" << _received.file.fileName() << "and"
                                      << _sent.file.fileName();
}

void GPSRecordingTransport::_record(Capture& capture, QByteArrayView bytes)
{
    if (!capture.active || bytes.isEmpty()) {
        return;
    }
    const qint64 count = (std::min) (qint64(bytes.size()), _maxFileBytes - capture.bytes);
    if (count > 0 && capture.file.write(bytes.data(), count) != count) {
        _fail(capture.file.fileName(), capture.file.errorString());
        return;
    }
    capture.bytes += count;
    if (capture.bytes >= _maxFileBytes) {
        qCWarning(GPSRecordingTransportLog) << "Receiver recording reached its size limit of" << _maxFileBytes
                                            << "bytes and stopped:" << capture.file.fileName();
        _finish(capture);
    }
}

void GPSRecordingTransport::_flushIfDue()
{
    if (!recording() || !_nextFlush.hasExpired()) {
        return;
    }
    _nextFlush.setRemainingTime(FLUSH_INTERVAL, Qt::CoarseTimer);
    for (Capture* const capture : {&_received, &_sent}) {
        if (capture->active && !capture->file.flush()) {
            _fail(capture->file.fileName(), capture->file.errorString());
            return;
        }
    }
}

void GPSRecordingTransport::_fail(const QString& path, const QString& error)
{
    qCWarning(GPSRecordingTransportLog) << "Receiver recording stopped after a file error:" << path << error;
    for (Capture* const capture : {&_received, &_sent}) {
        capture->active = false;
        capture->file.close();
    }
}

void GPSRecordingTransport::_finish(Capture& capture)
{
    if (!capture.active) {
        return;
    }
    if (!capture.file.flush()) {
        _fail(capture.file.fileName(), capture.file.errorString());
        return;
    }
    capture.active = false;
    capture.file.close();
}

}  // namespace GPSTest
