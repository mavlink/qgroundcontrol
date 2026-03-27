#include "NmeaStreamSplitter.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NmeaStreamSplitterLog, "PositionManager.NmeaStreamSplitter")

// -- NmeaPipeDevice ---------------------------------------------------------

NmeaPipeDevice::NmeaPipeDevice(QObject *parent)
    : QIODevice(parent)
{
    open(QIODevice::ReadWrite);
}

bool NmeaPipeDevice::canReadLine() const
{
    return _buffer.canReadLine();
}

qint64 NmeaPipeDevice::bytesAvailable() const
{
    return _buffer.size() + QIODevice::bytesAvailable();
}

void NmeaPipeDevice::feedData(const QByteArray &data)
{
    if (const qsizetype dropped = _buffer.append(data, kMaxBufferSize); dropped > 0) {
        qCWarning(NmeaStreamSplitterLog) << "Buffer overflow, discarded" << dropped << "oldest bytes";
    }
    emit readyRead();
}

qint64 NmeaPipeDevice::readData(char *data, qint64 maxlen)
{
    return _buffer.read(data, maxlen);
}

qint64 NmeaPipeDevice::readLineData(char *data, qint64 maxSize)
{
    return _buffer.readLine(data, maxSize);
}

qint64 NmeaPipeDevice::writeData(const char *data, qint64 len)
{
    if (const qsizetype dropped = _buffer.append(data, len, kMaxBufferSize); dropped > 0) {
        qCWarning(NmeaStreamSplitterLog) << "Buffer overflow, discarded" << dropped << "oldest bytes";
    }
    emit readyRead();
    return len;
}

// -- NmeaStreamSplitter -----------------------------------------------------

NmeaStreamSplitter::NmeaStreamSplitter(QIODevice *source, QObject *parent)
    : QObject(parent)
    , _source(source)
    , _positionPipe(new NmeaPipeDevice(this))
    , _satellitePipe(new NmeaPipeDevice(this))
{
    (void) connect(_source, &QIODevice::readyRead, this, &NmeaStreamSplitter::_onSourceReadyRead);
    qCDebug(NmeaStreamSplitterLog) << "Splitter created for source" << _source;
}

NmeaStreamSplitter::~NmeaStreamSplitter()
{
    qCDebug(NmeaStreamSplitterLog) << "Splitter destroyed";
}

void NmeaStreamSplitter::_onSourceReadyRead()
{
    const QByteArray data = _source->readAll();
    if (data.isEmpty()) {
        return;
    }

    _positionPipe->feedData(data);
    _satellitePipe->feedData(data);
}
