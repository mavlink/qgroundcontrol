#include "GPSRecordingTransport.h"

#include <algorithm>
#include <cerrno>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingTransportLog, "GPS.Recording.GPSRecordingTransport")

GPSRecordingTransport::GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, const std::atomic_bool& stop,
                                             std::shared_ptr<GPSRecordingStream> recording)
    : GPSTransport(stop), _transport(std::move(transport)), _recording(std::move(recording))
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

GPSTransport::OpenResult GPSRecordingTransport::open()
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const auto result = _transport ? _transport->open() : OpenResult{.status = OpenStatus::Error};
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

GPSTransport::ReadResult GPSRecordingTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const auto result =
        _transport ? _transport->read(buffer, length, timeoutMs) : ReadResult{.status = ReadStatus::Error};
    if (_recording) {
        const auto bytes =
            result.status == ReadStatus::Data && buffer && length > 0 && result.bytesRead > 0
                ? QByteArrayView(reinterpret_cast<const char*>(buffer), std::min(result.bytesRead, length))
                : QByteArrayView();
        _recording->recordRead(bytes, result, started);
    }
    return result;
}

GPSTransport::WriteResult GPSRecordingTransport::write(const uint8_t* buffer, int length)
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const auto result = _transport ? _transport->write(buffer, length) : WriteResult{.status = WriteStatus::Error};
    if (_recording) {
        const auto bytes =
            buffer && length > 0 ? QByteArrayView(reinterpret_cast<const char*>(buffer), length) : QByteArrayView();
        _recording->recordWrite(bytes, result, started, fatalError());
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

std::chrono::milliseconds GPSRecordingTransport::configurationWriteTimeout() const
{
    return _transport ? _transport->configurationWriteTimeout() : GPSTransport::configurationWriteTimeout();
}
