#include "GPSRecordingDialogue.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "GPSDriver.h"
#include "GPSReceiverWorker.h"
#include "GPSRecordingTransport.h"
#include "Protocols/Support/GPSTestClock.h"

namespace GPSTest {

namespace {
/// Past every interval the drivers measure from a receipt they never had, as a steady clock is.
constexpr uint64_t DIALOGUE_START_US = 1000000000;

constexpr uint64_t microsecondsOf(std::chrono::microseconds duration)
{
    return static_cast<uint64_t>((std::max) (duration, std::chrono::microseconds::zero()).count());
}

QString secondsText(uint64_t microseconds)
{
    return QStringLiteral("%1 s").arg(static_cast<double>(microseconds) / 1e6, 0, 'f', 6);
}

QString bytesText(QByteArrayView bytes)
{
    return QStringLiteral("%1 bytes %2").arg(bytes.size()).arg(QLatin1StringView(bytes.toByteArray().toHex()));
}
}  // namespace

QString GPSDialogueDivergence::text() const
{
    return QStringLiteral("line %1: the recording has %2; the driver %3").arg(line).arg(recorded, driver);
}

GPSDialogueTransport::GPSDialogueTransport(const QString& recording, GPSTestClock& clock)
    : GPSDialogueTransport(recording, clock, GPSCancelSource())
{}

GPSDialogueTransport::GPSDialogueTransport(const QString& recording, GPSTestClock& clock, GPSCancelSource stop)
    : GPSTransport(stop.token())
    , _stop(std::move(stop))
    , _file(recording)
    , _reader(_file)
    , _clock(clock)
{}

GPSDialogueTransport::~GPSDialogueTransport() = default;

GPSOpenResult GPSDialogueTransport::open()
{
    if (_file.isOpen()) {
        return {GPSOpenStatus::Opened};
    }
    if (!_file.open(QIODevice::ReadOnly)) {
        return {GPSOpenStatus::Error, _file.errorString()};
    }
    if (!_reader.readHeader()) {
        _file.close();
        return {GPSOpenStatus::Error, QStringLiteral("not a receiver recording")};
    }
    _startUs = _clock.nowUs();
    _progressUs = _startUs;
    _error = _reader.error();
    if (_error.isEmpty()) {
        _load();
    }
    if (!_error.isEmpty()) {
        return {GPSOpenStatus::Error, _error};
    }
    return {GPSOpenStatus::Opened};
}

bool GPSDialogueTransport::fatalError() const
{
    return !_file.isOpen() || !_error.isEmpty();
}

unsigned GPSDialogueTransport::fixedBaudRate() const
{
    return settings() ? settings()->fixedBaudRate : 0;
}

GPSReadResult GPSDialogueTransport::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (fatalError()) {
        return {_file.isOpen() ? GPSReadStatus::Error : GPSReadStatus::Closed, 0, _error};
    }
    if (buffer.empty()) {
        return {GPSReadStatus::Data};
    }
    if (!_record) {
        cancel();
        return {GPSReadStatus::Cancelled};
    }
    const uint64_t now = _clock.nowUs();
    const uint64_t until = now + microsecondsOf(timeout);
    if (_record->kind != GPSRecordingRecord::Kind::Received) {
        if (now > (std::max) (_dueUs(), _progressUs) + microsecondsOf(MISSING_WRITE_LIMIT)) {
            const bool rate = _record->kind == GPSRecordingRecord::Kind::BaudRate ||
                              _record->kind == GPSRecordingRecord::Kind::RefusedBaudRate;
            _diverge(rate          ? QStringLiteral("has not set that rate")
                     : _offset > 0 ? QStringLiteral("wrote only its first %1 bytes").arg(_offset)
                                   : QStringLiteral("wrote nothing"));
            return {GPSReadStatus::Cancelled};
        }
    } else if (_dueUs() <= until + microsecondsOf(READ_LATENESS)) {
        const qsizetype count = (std::min) (static_cast<qsizetype>(buffer.size()), _record->bytes.size() - _offset);
        std::memcpy(buffer.data(), _record->bytes.constData() + _offset, static_cast<size_t>(count));
        _offset += count;
        _receivedBytes += count;
        _clock.advanceTo(_dueUs());
        if (_offset == _record->bytes.size()) {
            _reproduced();
        }
        return {GPSReadStatus::Data, static_cast<int>(count)};
    }
    _clock.advanceTo(until);
    return {GPSReadStatus::TimedOut};
}

bool GPSDialogueTransport::setBaudRate(unsigned baudRate)
{
    if (isCancelled() || fatalError()) {
        return false;
    }
    const bool refused =
        _record && _record->kind == GPSRecordingRecord::Kind::RefusedBaudRate && _record->baudRate == baudRate;
    // The recorded link refused any rate but its own, which a recording made before refusals were recorded does not
    // say.
    if (!refused && fixedBaudRate() != 0 && baudRate != fixedBaudRate()) {
        return false;
    }
    if (!_record) {
        cancel();
        return false;
    }
    if (!refused && (_record->kind != GPSRecordingRecord::Kind::BaudRate || _record->baudRate != baudRate)) {
        _diverge(QStringLiteral("set the link to %1 baud").arg(baudRate));
        return false;
    }
    _baudRateChanges += refused ? 0 : 1;
    _clock.advanceTo(_dueUs());
    _reproduced();
    return !refused;
}

GPSWriteResult GPSDialogueTransport::writeData(QByteArrayView bytes, QDeadlineTimer)
{
    if (fatalError()) {
        return {GPSWriteStatus::Error, 0, 0, _error};
    }
    qsizetype written = 0;
    while (written < bytes.size()) {
        if (!_record) {
            cancel();
            break;
        }
        const QByteArrayView rest = bytes.sliced(written);
        const bool refused = _record->kind == GPSRecordingRecord::Kind::RefusedWrite;
        if (_record->kind != GPSRecordingRecord::Kind::Sent && !refused) {
            _diverge(QStringLiteral("wrote %1").arg(bytesText(rest)));
            break;
        }
        const QByteArrayView recorded = QByteArrayView(_record->bytes).sliced(_offset);
        const qsizetype count = (std::min) (rest.size(), recorded.size());
        const qsizetype same = std::mismatch(rest.begin(), rest.begin() + count, recorded.begin()).first - rest.begin();
        if (same < count) {
            _diverge(QStringLiteral("wrote %1 there, which differ from byte %2 of the record")
                         .arg(bytesText(rest.first(count)))
                         .arg(_offset + same + 1));
            break;
        }
        if (refused) {
            // The link refused one write whole, so only that write gets its answer.
            if (written > 0 || rest.size() != recorded.size()) {
                _diverge(
                    QStringLiteral("wrote %1 in one write, not the bytes of the record alone").arg(bytesText(bytes)));
                break;
            }
            const GPSWriteStatus answer = _record->answer;
            _clock.advanceTo(_dueUs());
            _reproduced();
            return {answer};
        }
        written += count;
        _offset += count;
        _sentBytes += count;
        if (_offset == _record->bytes.size()) {
            _clock.advanceTo(_dueUs());
            _reproduced();
        }
    }
    const int accepted = static_cast<int>(written);
    return {written == bytes.size() ? GPSWriteStatus::Completed : GPSWriteStatus::Cancelled, accepted, accepted};
}

void GPSDialogueTransport::sessionEnded(const QString& reason)
{
    if (_record) {
        _diverge(QStringLiteral("ended its session (%1)").arg(reason));
    }
}

bool GPSDialogueTransport::finished() const
{
    return _file.isOpen() && !_record && _error.isEmpty() && !_divergence;
}

uint64_t GPSDialogueTransport::_dueUs() const
{
    return _startUs + microsecondsOf(_record->time);
}

void GPSDialogueTransport::_reproduced()
{
    ++_records;
    _progressUs = _clock.nowUs();
    _load();
}

void GPSDialogueTransport::_load()
{
    _offset = 0;
    _record = _reader.next();
    _error = _file.error() != QFileDevice::NoError ? _file.errorString() : _reader.error();
    if (!_error.isEmpty()) {
        cancel();
    }
}

void GPSDialogueTransport::_diverge(const QString& action)
{
    if (!_divergence && _error.isEmpty()) {
        _divergence = {.line = _reader.line(),
                       .recorded = _describeRecord(),
                       .driver = QStringLiteral("%1 at %2").arg(action, _replayTime())};
    }
    cancel();
}

QString GPSDialogueTransport::_describeRecord() const
{
    QString record;
    switch (_record->kind) {
        case GPSRecordingRecord::Kind::Received:
            record = QStringLiteral("RX %1").arg(bytesText(_record->bytes));
            break;
        case GPSRecordingRecord::Kind::Sent:
            record = QStringLiteral("TX %1").arg(bytesText(_record->bytes));
            break;
        case GPSRecordingRecord::Kind::BaudRate:
            record = QStringLiteral("BAUD %1").arg(_record->baudRate);
            break;
        case GPSRecordingRecord::Kind::RefusedWrite:
            record = QStringLiteral("TX-REFUSED %1 %2")
                         .arg(QLatin1StringView(GPSRecordingTransport::refusalName(_record->answer)),
                              bytesText(_record->bytes));
            break;
        case GPSRecordingRecord::Kind::RefusedBaudRate:
            record = QStringLiteral("BAUD-REFUSED %1").arg(_record->baudRate);
            break;
    }
    return QStringLiteral("%1 at %2").arg(record, secondsText(microsecondsOf(_record->time)));
}

QString GPSDialogueTransport::_replayTime() const
{
    return secondsText(_clock.nowUs() - _startUs);
}

GPSDialogueResult replayDialogue(const QString& recording, const std::function<bool()>& interrupted)
{
    GPSDialogueResult result;
    GPSTestClock clock(DIALOGUE_START_US);
    GPSDialogueTransport link(recording, clock);
    if (const GPSOpenResult opened = link.open(); opened.status != GPSOpenStatus::Opened) {
        result.error = opened.detail;
        return result;
    }
    result.settings = link.settings();
    if (!result.settings) {
        result.error = QStringLiteral("the recording does not hold the settings of its session");
        return result;
    }
    {
        GPSDriverSinks sinks;
        if (result.settings->type == GPSType::Automatic) {
            sinks.onReceiverDetected = [&result](GPSType family) { result.detected = family; };
        }
        GPSDriver driver(result.settings->type, link, result.settings->config, std::move(sinks), clock.source());
        QString ended;
        result.configured = driver.configure();
        result.configurationError = driver.configurationError();
        result.receiverIdentity = driver.receiverIdentity();
        if (!result.configured) {
            ended = QStringLiteral("its configuration failed: %1").arg(result.configurationError);
        }
        while (ended.isEmpty() && !link.finished() && !link.isCancelled()) {
            if (interrupted && interrupted()) {
                result.error = QStringLiteral("interrupted");
                link.cancel();
                break;
            }
            const GPSReceiveResult received = driver.receiveOutcome(GPSReceiverWorker::RECEIVE_TIMEOUT);
            if (received.terminal()) {
                ended = QStringLiteral("it lost the receiver: %1").arg(received.detail);
            }
        }
        if (!ended.isEmpty()) {
            link.sessionEnded(ended);
        }
    }
    result.divergence = link.divergence();
    if (!link.error().isEmpty()) {
        result.error = link.error();
    }
    result.records = link.records();
    result.receivedBytes = link.receivedBytes();
    result.sentBytes = link.sentBytes();
    result.baudRateChanges = link.baudRateChanges();
    result.duration = std::chrono::microseconds(clock.nowUs() - DIALOGUE_START_US);
    return result;
}

}  // namespace GPSTest
