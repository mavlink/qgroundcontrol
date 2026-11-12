#include "GPSRecordingTransport.h"

#include <algorithm>
#include <utility>
#include <variant>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QLocale>
#include <QtCore/QMetaEnum>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingTransportLog, "GPS.Transport.GPSRecordingTransport")

namespace {

/// @a type in lower case, as a recording's file name and settings have it.
QByteArray recordedTypeName(GPSType type)
{
    const char* const name = QMetaEnum::fromType<GPSType>().valueToKey(static_cast<int>(type));
    return name ? QByteArray(name).toLower() : QByteArrayLiteral("gnss");
}

/// @a value as short as it reads back to the same number.
QByteArray recordedNumber(double value)
{
    return QByteArray::number(value, 'g', QLocale::FloatingPointShortest);
}

}  // namespace

GPSRecordingTransport::GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, const QString& file,
                                             qint64 maxFileBytes, std::function<void()> stopped,
                                             std::optional<Session> session, GPSClock clock)
    : GPSTransport(transport ? transport->cancelToken() : GPSCancelToken{})
    , _transport(std::move(transport))
    , _path(file)
    , _stopped(std::move(stopped))
    , _session(std::move(session))
    , _clock(std::move(clock))
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
    return QDir(directory).filePath(QStringLiteral("%1-%2.log")
                                        .arg(startedAt.toUTC().toString(QStringLiteral("yyyyMMdd'T'HHmmss.zzz'Z'")),
                                             QString::fromLatin1(recordedTypeName(type))));
}

QByteArray GPSRecordingTransport::settingsRecord(const Session& session, unsigned fixedBaudRate)
{
    const GPSReceiverConfig& config = session.config;
    QByteArray record = "type=" + recordedTypeName(session.type) + ",baud=" + QByteArray::number(config.baudRate) +
                        ",fixed-baud=" + QByteArray::number(fixedBaudRate) +
                        ",save=" + QByteArray::number(int(config.allowPersistentChanges)) +
                        ",compact=" + QByteArray::number(int(config.base.compactObservations));
    // Only where a base leaves constellations out of its corrections, so every other record reads as before.
    if (config.base.omittedConstellations()) {
        const auto sent = config.base.correctionConstellations & GPS_ALL_CORRECTION_CONSTELLATIONS;
        record += ",constellations=" + QByteArray::number(sent.toInt());
    }
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode)) {
        record += ",base=fixed,latitude=" + recordedNumber(fixed->position.latitudeDegrees) +
                  ",longitude=" + recordedNumber(fixed->position.longitudeDegrees) +
                  ",altitude=" + recordedNumber(fixed->position.altitudeMeters) +
                  ",accuracy=" + recordedNumber(fixed->accuracyMeters);
    } else if (const auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&config.base.mode)) {
        record += ",base=averaging,duration=" + QByteArray::number(qlonglong(averaging->maximumDuration.count()));
    } else {
        const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode);
        record += ",base=survey,duration=" + QByteArray::number(qlonglong(survey.duration.count())) +
                  ",accuracy=" + recordedNumber(survey.accuracyMeters);
    }
    return record;
}

const char* GPSRecordingTransport::refusalName(GPSWriteStatus answer)
{
    switch (answer) {
        case GPSWriteStatus::Completed:
            return "completed";
        case GPSWriteStatus::TimedOut:
            return "timed-out";
        case GPSWriteStatus::Error:
            return "error";
        case GPSWriteStatus::Unsupported:
            return "unsupported";
        case GPSWriteStatus::Cancelled:
            break;
    }
    return nullptr;
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
    if (!_transport) {
        return false;
    }
    const bool changed = _transport->setBaudRate(baudRate);
    if (changed || !isCancelled()) {
        _record(changed ? BAUD : BAUD_REFUSED, QByteArray::number(baudRate));
    }
    _flushIfDue();
    return changed;
}

GPSWriteResult GPSRecordingTransport::writeData(QByteArrayView bytes, QDeadlineTimer deadline)
{
    if (!_transport) {
        return {GPSWriteStatus::Error};
    }
    GPSWriteResult result = _transport->write(bytes, deadline);
    const QByteArrayView accepted = bytes.first(std::clamp<qsizetype>(result.acceptedBytes, 0, bytes.size()));
    if (!recording()) {
        return result;
    }
    if (!accepted.isEmpty()) {
        _record(SENT, accepted.toByteArray().toHex());
    } else if (const char* const answer = refusalName(result.status)) {
        _record(SENT_REFUSED, QByteArray(answer) + ' ' + bytes.toByteArray().toHex());
    }
    _flushIfDue();
    return result;
}

GPSWriteResult GPSRecordingTransport::writeExpired(QByteArrayView bytes)
{
    // The link answers, so that the attempt is recorded as any write that it refuses.
    return writeData(bytes, QDeadlineTimer());
}

void GPSRecordingTransport::_start()
{
    // Never over an earlier recording: session names differ by their start time alone.
    if (!_file.open(_path)) {
        _notifyStopped();
        return;
    }
    _startedAtUs = _clock.nowUs();
    _nextFlush.setRemainingTime(FLUSH_INTERVAL, Qt::CoarseTimer);
    qCDebug(GPSRecordingTransportLog) << "Recording receiver data to" << _path;
    _write(QByteArray(FILE_HEADER) + '\n');
    if (_session) {
        _record(SETTINGS, settingsRecord(*_session, fixedBaudRate()));
    }
}

void GPSRecordingTransport::_record(const char* kind, const QByteArray& value)
{
    if (recording()) {
        _write(QByteArray::number(qulonglong(_clock.nowUs() - _startedAtUs)) + ' ' + kind + ' ' + value + '\n');
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
