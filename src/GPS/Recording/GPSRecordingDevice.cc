#include "GPSRecordingDevice.h"

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingDeviceLog, "GPS.Recording.GPSRecordingDevice")

GPSRecordingDevice::GPSRecordingDevice(QIODevice* source, std::shared_ptr<GPSRecordingStream> recording,
                                       QObject* parent)
    : QIODevice(parent), _source(source), _recording(std::move(recording))
{
    qCDebug(GPSRecordingDeviceLog) << this;
    if (!source || !source->isOpen()) {
        return;
    }
    open(source->openMode() | QIODevice::Unbuffered);
    connect(source, &QIODevice::readyRead, this, &QIODevice::readyRead);
    connect(source, &QIODevice::aboutToClose, this, &GPSRecordingDevice::close);
    connect(source, &QObject::destroyed, this, &GPSRecordingDevice::close);
}

GPSRecordingDevice::~GPSRecordingDevice()
{
    qCDebug(GPSRecordingDeviceLog) << this;
    close();
}

bool GPSRecordingDevice::isSequential() const
{
    return true;
}

qint64 GPSRecordingDevice::bytesAvailable() const
{
    return (_source ? _source->bytesAvailable() : 0) + QIODevice::bytesAvailable();
}

quint64 GPSRecordingDevice::lastReadTimestampUs() const
{
    return _lastReadUs;
}

void GPSRecordingDevice::close()
{
    if (_recording) {
        _recording->closed();
    }
    QIODevice::close();
}

qint64 GPSRecordingDevice::readData(char* data, qint64 maximum)
{
    if (!_source) {
        return -1;
    }
    const auto started = _recording ? _recording->nowUs() : 0;
    const qint64 count = _source->read(data, maximum);
    if (count > 0) {
        _lastReadUs = GPSReadTimestamp::from(_source);
        if (_recording) {
            _recording->record(GPSRecordingBuffer::Kind::Rx, QByteArrayView(data, count), 0, started);
        }
    } else if (count < 0 && _recording) {
        _recording->record(GPSRecordingBuffer::Kind::ReadError, {}, static_cast<int>(count), started);
    }
    return count;
}

qint64 GPSRecordingDevice::writeData(const char* data, qint64 length)
{
    const auto started = _recording ? _recording->nowUs() : 0;
    const auto count = _source ? _source->write(data, length) : -1;
    if (_recording) {
        _recording->record(count == length ? GPSRecordingBuffer::Kind::Tx : GPSRecordingBuffer::Kind::WriteError,
                           QByteArrayView(data, length), count == length ? 0 : static_cast<int>(count), started);
    }
    return count;
}
