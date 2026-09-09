#include "GPSReplayTransport.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMap>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReplayTransportLog, "GPS.Test.ReplayTransport")

bool GPSReplayTrace::load(const QString& filename, GPSReplayTrace& result, QString& error, quint64 streamId)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    constexpr qint64 maxTraceBytes = 4 * 1024 * 1024;
    if (file.size() > maxTraceBytes) {
        error = QStringLiteral("Replay trace exceeds 4 MiB");
        return false;
    }
    return fromJson(file.readAll(), result, error, streamId);
}

bool GPSReplayTrace::fromJson(const QByteArray& json, GPSReplayTrace& result, QString& error, quint64 streamId)
{
    if (json.size() > 4 * 1024 * 1024) {
        error = QStringLiteral("Replay trace exceeds 4 MiB");
        return false;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("Invalid replay JSON: %1").arg(parseError.errorString());
        return false;
    }
    const auto object = document.object();
    if (object.value("version").toInt() != 1 || !object.value("events").isArray()) {
        error = QStringLiteral("Replay requires version 1 and an events array");
        return false;
    }
    const auto events = object.value("events").toArray();
    if (events.size() > 100000) {
        error = QStringLiteral("Replay has too many events");
        return false;
    }
    GPSReplayTrace parsed;
    quint64 previous = 0;
    quint64 selectedStream = streamId;
    bool foundStream = false;
    const QMap<QString, GPSReplayEvent::Kind> kinds = {
        {"open", GPSReplayEvent::Kind::Open},
        {"open_error", GPSReplayEvent::Kind::OpenError},
        {"baud_error", GPSReplayEvent::Kind::BaudError},
        {"rx", GPSReplayEvent::Kind::Rx},
        {"tx", GPSReplayEvent::Kind::Tx},
        {"baud", GPSReplayEvent::Kind::Baud},
        {"timeout", GPSReplayEvent::Kind::Timeout},
        {"read_error", GPSReplayEvent::Kind::ReadError},
        {"write_error", GPSReplayEvent::Kind::WriteError},
        {"disconnect", GPSReplayEvent::Kind::Disconnect},
        {"cancel", GPSReplayEvent::Kind::Cancel},
    };
    for (const auto& value : events) {
        const auto event = value.toObject();
        const double timestamp = event.value("at_us").toDouble(-1);
        const QString kindName = event.value("kind").toString();
        const bool metadata = kindName == QStringLiteral("session") || kindName == QStringLiteral("close") ||
                              kindName == QStringLiteral("configuration_started") ||
                              kindName == QStringLiteral("configuration_finished");
        const auto kind = kinds.constFind(kindName);
        if (!std::isfinite(timestamp) || timestamp < 0 || timestamp > 9e15 || std::floor(timestamp) != timestamp ||
            static_cast<quint64>(timestamp) < previous || (kind == kinds.cend() && !metadata)) {
            error = QStringLiteral("Invalid or out-of-order replay event %1").arg(parsed.events.size());
            return false;
        }
        previous = static_cast<quint64>(timestamp);
        const auto streamValue = event.value("stream").toInteger();
        if (streamValue < 0) {
            error = QStringLiteral("Invalid recording stream identifier");
            return false;
        }
        const auto currentStream = static_cast<quint64>(streamValue);
        if (selectedStream == 0) {
            selectedStream = currentStream;
        }
        if (currentStream != selectedStream) {
            continue;
        }
        foundStream = true;
        if (metadata) {
            if (kindName == QStringLiteral("session")) {
                parsed.profile = event.value("profile").toObject();
            }
            continue;
        }
        GPSReplayEvent item;
        item.atUs = static_cast<quint64>(timestamp);
        item.kind = kind.value();
        item.value = event.value("value").toInt();
        const auto hex = event.value("hex").toString().toLatin1();
        item.bytes = QByteArray::fromHex(hex);
        if (item.bytes.toHex() != hex.toLower() ||
            ((item.kind == GPSReplayEvent::Kind::Rx || item.kind == GPSReplayEvent::Kind::Tx) &&
             item.bytes.isEmpty())) {
            error = QStringLiteral("Invalid replay hex at event %1").arg(parsed.events.size());
            return false;
        }
        previous = item.atUs;
        parsed.events.append(std::move(item));
    }
    if (streamId && !foundStream) {
        error = QStringLiteral("Requested recording stream was not found");
        return false;
    }
    parsed.streamId = selectedStream;
    result = std::move(parsed);
    error.clear();
    return true;
}

GPSReplayTransport::GPSReplayTransport(GPSReplayClock& clock, std::atomic_bool& requestStop, GPSReplayTrace trace,
                                       int maximumRead)
    : GPSTransport(requestStop),
      _clock(clock),
      _stop(requestStop),
      _trace(std::move(trace)),
      _maximumRead(qMax(maximumRead, 1))
{
    qCDebug(GPSReplayTransportLog) << this;
}

GPSReplayTransport::~GPSReplayTransport()
{
    qCDebug(GPSReplayTransportLog) << this;
}

int GPSReplayTransport::_fail(const QString& message)
{
    if (_failure.isEmpty()) {
        _failure = QStringLiteral("Event %1 at %2 us: %3").arg(_index).arg(_clock.nowUs()).arg(message);
    }
    _fatal = true;
    return -EIO;
}

bool GPSReplayTransport::open()
{
    if (isCancelled()) {
        return false;
    }
    if (_index < _trace.events.size() && _trace.events[_index].kind == GPSReplayEvent::Kind::OpenError) {
        _clock.advanceTo(_trace.events[_index++].atUs);
        _fatal = true;
        _opened = false;
        return false;
    }
    if (_index >= _trace.events.size() || _trace.events[_index].kind != GPSReplayEvent::Kind::Open) {
        _fail(QStringLiteral("Unexpected open"));
        return false;
    }
    _clock.advanceTo(_trace.events[_index++].atUs);
    _fatal = false;
    _opened = true;
    return true;
}

int GPSReplayTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    ++_readCount;
    if (isCancelled()) {
        return -ECANCELED;
    }
    if (!_opened || _fatal || !buffer || length <= 0) {
        return -EIO;
    }
    const auto deadline = _clock.nowUs() + quint64(qMax(timeoutMs, 0)) * 1000;
    if (_index >= _trace.events.size() ||
        _trace.events[_index].atUs > deadline + (_trace.events[_index].kind == GPSReplayEvent::Kind::Timeout ? 1 : 0)) {
        _clock.advanceTo(deadline + (timeoutMs > 0 ? 1 : 0));
        return 0;
    }
    auto& event = _trace.events[_index];
    _clock.advanceTo(event.atUs);
    if (event.kind == GPSReplayEvent::Kind::Rx) {
        const auto count = std::min({qsizetype(length), qsizetype(_maximumRead), event.bytes.size() - _offset});
        std::memcpy(buffer, event.bytes.constData() + _offset, static_cast<size_t>(count));
        _lastReadTimestampUs = event.atUs;
        _offset += count;
        if (_offset == event.bytes.size()) {
            _offset = 0;
            ++_index;
        }
        return static_cast<int>(count);
    }
    const auto kind = event.kind;
    const int error = event.value < 0 ? event.value : -EIO;
    if (kind == GPSReplayEvent::Kind::Timeout) {
        ++_index;
        return 0;
    }
    if (kind == GPSReplayEvent::Kind::Cancel) {
        ++_index;
        _stop = true;
        return -ECANCELED;
    }
    if (kind == GPSReplayEvent::Kind::Disconnect || kind == GPSReplayEvent::Kind::ReadError) {
        ++_index;
        _fatal = true;
        _opened = false;
        return error;
    }
    return _fail(QStringLiteral("Read encountered an expected write, baud change, or open"));
}

int GPSReplayTransport::write(const uint8_t* buffer, int length)
{
    if (isCancelled()) {
        return -ECANCELED;
    }
    if (!_opened || _fatal || !buffer || length <= 0 || _index >= _trace.events.size()) {
        return _fail(QStringLiteral("Unexpected write"));
    }
    if (_trace.events[_index].kind == GPSReplayEvent::Kind::WriteError) {
        const auto event = _trace.events[_index++];
        if (!event.bytes.isEmpty() && event.bytes != QByteArrayView(reinterpret_cast<const char*>(buffer), length)) {
            return _fail(QStringLiteral("Failed TX bytes differ from trace"));
        }
        _clock.advanceTo(event.atUs);
        _fatal = true;
        return event.value < length ? event.value : -EIO;
    }
    int consumed = 0;
    while (consumed < length) {
        if (_index >= _trace.events.size() || _trace.events[_index].kind != GPSReplayEvent::Kind::Tx) {
            return _fail(QStringLiteral("Write exceeded expected TX bytes"));
        }
        const auto& event = _trace.events[_index];
        const auto count = std::min(qsizetype(length - consumed), event.bytes.size() - _offset);
        if (std::memcmp(buffer + consumed, event.bytes.constData() + _offset, static_cast<size_t>(count)) != 0) {
            return _fail(QStringLiteral("TX bytes differ from trace"));
        }
        _clock.advanceTo(event.atUs);
        consumed += static_cast<int>(count);
        _offset += count;
        if (_offset == event.bytes.size()) {
            _offset = 0;
            ++_index;
        }
    }
    return consumed;
}

bool GPSReplayTransport::setBaudrate(unsigned baudrate)
{
    if (isCancelled()) {
        return false;
    }
    if (_index < _trace.events.size() && _trace.events[_index].kind == GPSReplayEvent::Kind::BaudError &&
        _trace.events[_index].value == static_cast<int>(baudrate)) {
        _clock.advanceTo(_trace.events[_index++].atUs);
        return false;
    }
    if (_index >= _trace.events.size() || _trace.events[_index].kind != GPSReplayEvent::Kind::Baud ||
        _trace.events[_index].value != static_cast<int>(baudrate)) {
        _fail(QStringLiteral("Unexpected baud rate %1").arg(baudrate));
        return false;
    }
    _clock.advanceTo(_trace.events[_index++].atUs);
    return true;
}
