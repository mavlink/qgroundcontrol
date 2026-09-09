#include "GPSRecordingTransport.h"

#include <algorithm>
#include <cerrno>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingTransportLog, "GPS.Recording.GPSRecordingTransport")

GPSRecordingTransport::GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, const std::atomic_bool& stop,
                                             std::shared_ptr<GPSRecordingStream> recording)
    : GPSTransport(stop)
    , _transport(std::move(transport))
    , _recording(std::move(recording))
{
    qCDebug(GPSRecordingTransportLog) << this;
}

GPSRecordingTransport::~GPSRecordingTransport()
{
    qCDebug(GPSRecordingTransportLog) << this;
    _transport.reset();
    if (_recording) {
        _recording->closed(isCancelled() ? -ECANCELED : 0);
    }
}

bool GPSRecordingTransport::open()
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const bool result = _transport && _transport->open();
    if (_recording) {
        _recording->opened(result, started);
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

int GPSRecordingTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const int result = _transport ? _transport->read(buffer, length, timeoutMs) : -EIO;
    if (_recording) {
        using K = GPSRecordingBuffer::Kind;
        if (result > 0 && buffer && length > 0) {
            _recording->record(K::Rx, QByteArrayView(reinterpret_cast<const char*>(buffer), std::min(result, length)),
                               0, started);
        } else {
            const K kind = isCancelled() ? K::Cancel : result < 0 ? K::ReadError : K::Timeout;
            _recording->record(kind, {}, result < 0 ? result : timeoutMs, started);
        }
    }
    return result;
}

int GPSRecordingTransport::write(const uint8_t* buffer, int length)
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const int result = _transport ? _transport->write(buffer, length) : -EIO;
    if (_recording) {
        const auto bytes =
            buffer && length > 0 ? QByteArrayView(reinterpret_cast<const char*>(buffer), length) : QByteArrayView();
        // A failed drain may have transmitted a prefix; preserve the attempted write and reported result.
        _recording->record(
            result == length && result > 0 ? GPSRecordingBuffer::Kind::Tx : GPSRecordingBuffer::Kind::WriteError, bytes,
            result == length ? 0 : result, started);
    }
    return result;
}

bool GPSRecordingTransport::setBaudrate(unsigned baudrate)
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const bool result = _transport && _transport->setBaudrate(baudrate);
    if (_recording) {
        _recording->record(result ? GPSRecordingBuffer::Kind::Baud : GPSRecordingBuffer::Kind::BaudError, {},
                           static_cast<int>(baudrate), started);
    }
    return result;
}

GPSTransport::WriteResult GPSRecordingTransport::writeBounded(const uint8_t* buffer, int length,
                                                              QDeadlineTimer deadline)
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const auto result =
        _transport ? _transport->writeBounded(buffer, length, deadline) : WriteResult{.status = WriteStatus::Error};
    if (_recording) {
        const auto bytes =
            buffer && length > 0 ? QByteArrayView(reinterpret_cast<const char*>(buffer), length) : QByteArrayView();
        _recording->recordWrite(bytes, result, started, fatalError());
    }
    return result;
}

std::chrono::milliseconds GPSRecordingTransport::correctionWriteTimeout(int length) const
{
    return _transport ? _transport->correctionWriteTimeout(length) : GPSTransport::correctionWriteTimeout(length);
}
