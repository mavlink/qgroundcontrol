#include "GPSByteStream.h"

#include <QtCore/QMutexLocker>

#include <algorithm>
#include <cstring>
#include <utility>

#include "GPSSourceHealth.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSByteStreamLog, "GPS.Receiver.GPSByteStream")

GPSByteStream::GPSByteStream(QObject* parent, std::function<quint64()> nowUs)
    : QIODevice(parent)
    , _nowUs(nowUs ? std::move(nowUs) : ReadTimestamp::nowUs)
    , _buffer(std::make_shared<TimestampedByteBuffer>())
{
    qCDebug(GPSByteStreamLog) << this;
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

GPSByteStream::~GPSByteStream()
{
    qCDebug(GPSByteStreamLog) << this;
}

qint64 GPSByteStream::bytesAvailable() const
{
    return _buffer->size() + (_buffer->gapPending() ? 1 : 0) + QIODevice::bytesAvailable();
}

qint64 GPSByteStream::readData(char* data, qint64 length)
{
    const auto result = _buffer->read(data, length, _nowUs(), GPSSourceHealth::FRESHNESS_TIMEOUT_MS * 1000u);
    _lastReadTimestampUs = result.receivedAtUs;
    if (result.gap) {
        // NMEA resynchronizes at a data gap; the generic buffer does not manufacture bytes.
        data[0] = '\0';
        return 1;
    }
    return result.bytes;
}

void GPSByteStream::notifyReadyRead()
{
    if (bytesAvailable() > 0) {
        emit readyRead();
    }
}
