#include "NMEAStreamSplitter.h"

#include <QtCore/QIODevice>

#include <algorithm>
#include <deque>

#include "QGCLoggingCategory.h"
#include "ReadTimestamp.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NMEAStreamSplitterLog, "GPS.NMEA.NMEAStreamSplitter")
QGC_LOGGING_CATEGORY(NMEAStreamDeviceLog, "GPS.NMEA.NMEAStreamDevice")

namespace {
constexpr qsizetype MAX_BUFFERED_BYTES = 64 * 1024;
constexpr qsizetype MAX_READ_BYTES_PER_TURN = 32 * 1024;
constexpr qsizetype MAX_SENTENCE_BYTES = 1024;
}  // namespace

class NMEAStreamDevice : public QIODevice, public ReadTimestamp, public NMEASentenceProvider
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

    bool canReadLine() const override { return !_sentences.empty() && _sentences.front().remaining().contains('\n'); }

    quint64 lastReadTimestampUs() const override { return _lastReadTimestampUs; }

    std::optional<NMEASentenceEnvelope> lastReadSentence() const override { return _lastSentence; }

    void close() override
    {
        _sentences.clear();
        _size = 0;
        _lastSentence.reset();
        QIODevice::close();
    }

    void append(const NMEASentenceEnvelope& envelope)
    {
        const auto& bytes = envelope.bytes();
        if (!isOpen() || bytes.isEmpty()) {
            return;
        }
        while (!_sentences.empty() && _size + bytes.size() > MAX_BUFFERED_BYTES) {
            _size -= _sentences.front().remaining().size();
            _sentences.pop_front();
        }
        _sentences.push_back({envelope});
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
        const qint64 size = std::min<qint64>(maxSize, sentence.remaining().size());
        std::copy_n(sentence.remaining().data(), size, data);
        _lastReadTimestampUs = sentence.envelope.receivedAtUs();
        _lastSentence = sentence.envelope;
        sentence.offset += size;
        _size -= size;
        if (sentence.remaining().isEmpty()) {
            _sentences.pop_front();
        }
        return size;
    }

    qint64 readLineData(char* data, qint64 maxSize) override { return readData(data, maxSize); }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    struct Sentence
    {
        NMEASentenceEnvelope envelope;
        qsizetype offset = 0;

        QByteArrayView remaining() const { return QByteArrayView(envelope.bytes()).sliced(offset); }
    };

    std::deque<Sentence> _sentences;
    qint64 _size = 0;
    quint64 _lastReadTimestampUs = 0;
    std::optional<NMEASentenceEnvelope> _lastSentence;
};

NMEAStreamDevice::~NMEAStreamDevice()
{
    qCDebug(NMEAStreamDeviceLog) << this;
}

NMEAStreamSplitter::NMEAStreamSplitter(QIODevice* source, QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent), _source(source), _scheduler(scheduler), _positionDevice(std::make_unique<NMEAStreamDevice>())
{
    qCDebug(NMEAStreamSplitterLog) << this;
    if (source) {
        QMetaObject::invokeMethod(this, &NMEAStreamSplitter::_readAvailableData, Qt::QueuedConnection);
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

void NMEAStreamSplitter::_readAvailableData()
{
    if (_closed || _drainPending) {
        return;
    }
    const QPointer<NMEAStreamSplitter> guard(this);
    qsizetype remaining = MAX_READ_BYTES_PER_TURN;
    while (remaining > 0 && _source && _source->isReadable() && _source->bytesAvailable() > 0) {
        const QByteArray data = _source->read(remaining);
        remaining -= data.size();
        if (data.isEmpty()) {
            return;
        }
        const quint64 receivedAtUs =
            ReadTimestamp::from(_source, _scheduler ? _scheduler->nowUs() : ReadTimestamp::nowUs());
        emit dataReceived(receivedAtUs);
        if (!guard || _closed) {
            return;
        }
        QList<NMEASentenceEnvelope> sentences;
        for (const char byte : data) {
            if (byte == '$') {
                _sentence = "$";
                _sentenceTimestampUs = receivedAtUs;
            } else if (!_sentence.isEmpty()) {
                _sentence.append(byte);
                if (byte == '\n') {
                    if (auto sentence = NMEASentenceEnvelope::parse(_sentence, _sentenceTimestampUs)) {
                        sentences.append(std::move(*sentence));
                    }
                    _sentence.clear();
                } else if (_sentence.size() > MAX_SENTENCE_BYTES || (byte != '\r' && (byte < ' ' || byte > '~'))) {
                    _sentence.clear();
                }
            }
        }
        for (const auto& sentence : sentences) {
            _positionDevice->append(sentence);
            if (!guard || _closed) {
                return;
            }
            emit sentenceReceived(sentence);
            if (!guard || _closed) {
                return;
            }
        }
    }
    if (_source && _source->isReadable() && _source->bytesAvailable() > 0) {
        _drainPending = true;
        QMetaObject::invokeMethod(
            this,
            [this]() {
                _drainPending = false;
                _readAvailableData();
            },
            Qt::QueuedConnection);
    }
}

void NMEAStreamSplitter::_closeOutputs()
{
    const QPointer<NMEAStreamSplitter> guard(this);
    if (_closed) {
        return;
    }
    _closed = true;
    _sentence.clear();
    _positionDevice->close();
    if (guard) {
        emit closed();
    }
}
