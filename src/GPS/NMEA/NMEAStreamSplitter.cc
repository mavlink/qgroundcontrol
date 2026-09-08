#include "NMEAStreamSplitter.h"

#include <QtCore/QIODevice>

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEAStreamSplitterLog, "GPS.NMEA.NMEAStreamSplitter")
QGC_LOGGING_CATEGORY(NMEAStreamDeviceLog, "GPS.NMEA.NMEAStreamDevice")

namespace {
constexpr qsizetype kMaxBufferedBytes = 64 * 1024;
}

class NMEAStreamDevice : public QIODevice
{
public:
    NMEAStreamDevice()
    {
        qCDebug(NMEAStreamDeviceLog) << this;
        open(QIODevice::ReadOnly);
    }

    ~NMEAStreamDevice() override;

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return QIODevice::bytesAvailable() + _buffer.size(); }

    bool canReadLine() const override { return QIODevice::canReadLine() || _buffer.contains('\n'); }

    void close() override
    {
        _buffer.clear();
        _discardUntilNewline = false;
        QIODevice::close();
    }

    void append(const QByteArray& data)
    {
        if (!isOpen() || data.isEmpty()) {
            return;
        }
        qsizetype start = 0;
        if (_discardUntilNewline) {
            const qsizetype newline = data.indexOf('\n');
            if (newline < 0) {
                return;
            }
            start = newline + 1;
            _discardUntilNewline = false;
        }
        _buffer.append(data.constData() + start, data.size() - start);
        if (_buffer.size() > kMaxBufferedBytes) {
            const qsizetype newline = _buffer.indexOf('\n', _buffer.size() - kMaxBufferedBytes - 1);
            if (newline < 0) {
                _buffer.clear();
                _discardUntilNewline = true;
            } else {
                _buffer.remove(0, newline + 1);
            }
        }
        if (!_buffer.isEmpty()) {
            emit readyRead();
        }
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        const qint64 size = std::min<qint64>(maxSize, _buffer.size());
        std::copy_n(_buffer.constData(), size, data);
        _buffer.remove(0, size);
        return size;
    }

    qint64 readLineData(char* data, qint64 maxSize) override
    {
        const qint64 newline = _buffer.indexOf('\n');
        return readData(data, newline < 0 ? maxSize : std::min(maxSize, newline + 1));
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray _buffer;
    bool _discardUntilNewline = false;
};

NMEAStreamDevice::~NMEAStreamDevice()
{
    qCDebug(NMEAStreamDeviceLog) << this;
}

NMEAStreamSplitter::NMEAStreamSplitter(QIODevice* source, QObject* parent)
    : QObject(parent)
    , _source(source)
    , _positionDevice(std::make_unique<NMEAStreamDevice>())
    , _satelliteDevice(std::make_unique<NMEAStreamDevice>())
{
    qCDebug(NMEAStreamSplitterLog) << this;
    if (source) {
        connect(source, &QIODevice::readyRead, this, &NMEAStreamSplitter::_readAvailableData);
        connect(source, &QIODevice::aboutToClose, this, &NMEAStreamSplitter::_closeOutputs);
        connect(source, &QObject::destroyed, this, &NMEAStreamSplitter::_closeOutputs);
    }
}

NMEAStreamSplitter::~NMEAStreamSplitter()
{
    qCDebug(NMEAStreamSplitterLog) << this;
}

QIODevice* NMEAStreamSplitter::positionDevice() const
{
    return _positionDevice.get();
}

QIODevice* NMEAStreamSplitter::satelliteDevice() const
{
    return _satelliteDevice.get();
}

void NMEAStreamSplitter::_readAvailableData()
{
    const QPointer<NMEAStreamSplitter> guard(this);
    while (_source && _source->isReadable() && _source->bytesAvailable() > 0) {
        const QByteArray data = _source->read(kMaxBufferedBytes);
        if (data.isEmpty()) {
            return;
        }
        _positionDevice->append(data);
        if (!guard) {
            return;
        }
        _satelliteDevice->append(data);
        if (!guard) {
            return;
        }
    }
}

void NMEAStreamSplitter::_closeOutputs()
{
    const QPointer<NMEAStreamSplitter> guard(this);
    _positionDevice->close();
    if (guard) {
        _satelliteDevice->close();
    }
}
