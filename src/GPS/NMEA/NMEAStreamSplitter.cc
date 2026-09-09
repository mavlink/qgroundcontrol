#include "NMEAStreamSplitter.h"

#include <QtCore/QIODevice>

#include <algorithm>
#include <deque>

#include "GPSReadTimestamp.h"
#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEAStreamSplitterLog, "GPS.NMEA.NMEAStreamSplitter")
QGC_LOGGING_CATEGORY(NMEAStreamDeviceLog, "GPS.NMEA.NMEAStreamDevice")

namespace {
constexpr qsizetype kMaxBufferedBytes = 64 * 1024;
}

class NMEAStreamDevice : public QIODevice, public GPSReadTimestamp
{
public:
    NMEAStreamDevice()
    {
        qCDebug(NMEAStreamDeviceLog) << this;
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }

    ~NMEAStreamDevice() override;

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return QIODevice::bytesAvailable() + _size; }

    bool canReadLine() const override { return !_sentences.empty() && _sentences.front().bytes.contains('\n'); }

    quint64 lastReadTimestampUs() const override { return _lastReadTimestampUs; }

    void close() override
    {
        _sentences.clear();
        _size = 0;
        QIODevice::close();
    }

    void append(const QByteArray& bytes, quint64 receivedAtUs)
    {
        if (!isOpen() || bytes.isEmpty()) {
            return;
        }
        while (!_sentences.empty() && _size + bytes.size() > kMaxBufferedBytes) {
            _size -= _sentences.front().bytes.size();
            _sentences.pop_front();
        }
        _sentences.push_back({bytes, receivedAtUs});
        _size += bytes.size();
        emit readyRead();
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        if (_sentences.empty() || maxSize <= 0) {
            return 0;
        }
        auto& sentence = _sentences.front();
        const qint64 size = std::min<qint64>(maxSize, sentence.bytes.size());
        std::copy_n(sentence.bytes.constData(), size, data);
        _lastReadTimestampUs = sentence.receivedAtUs;
        sentence.bytes.remove(0, size);
        _size -= size;
        if (sentence.bytes.isEmpty()) {
            _sentences.pop_front();
        }
        return size;
    }

    qint64 readLineData(char* data, qint64 maxSize) override { return readData(data, maxSize); }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    struct Sentence
    {
        QByteArray bytes;
        quint64 receivedAtUs = 0;
    };

    std::deque<Sentence> _sentences;
    qint64 _size = 0;
    quint64 _lastReadTimestampUs = 0;
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
        const QByteArray data = _source->read(kMaxBufferedBytes / 2);
        if (data.isEmpty()) {
            return;
        }
        const quint64 receivedAtUs = GPSReadTimestamp::from(_source);
        QList<std::pair<QByteArray, quint64>> sentences;
        for (const char byte : data) {
            if (byte == '$') {
                _sentence = "$";
                _sentenceTimestampUs = receivedAtUs;
            } else if (!_sentence.isEmpty()) {
                _sentence.append(byte);
                if (byte == '\n') {
                    const QByteArray line = _sentence.trimmed();
                    if (line.indexOf('*') == line.size() - 3 && NMEAUtils::verifyChecksum(line)) {
                        sentences.append({line + "\r\n", _sentenceTimestampUs});
                    }
                    _sentence.clear();
                } else if (_sentence.size() > 1024 || (byte != '\r' && (byte < ' ' || byte > '~'))) {
                    _sentence.clear();
                }
            }
        }
        for (const auto& [sentence, timestampUs] : sentences) {
            _positionDevice->append(sentence, timestampUs);
            if (!guard) {
                return;
            }
            _satelliteDevice->append(sentence, timestampUs);
            if (!guard) {
                return;
            }
        }
    }
}

void NMEAStreamSplitter::_closeOutputs()
{
    const QPointer<NMEAStreamSplitter> guard(this);
    _sentence.clear();
    _positionDevice->close();
    if (guard) {
        _satelliteDevice->close();
    }
}
