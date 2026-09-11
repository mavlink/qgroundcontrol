#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QIODevice>
#include <QtPositioning/QNmeaPositionInfoSource>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"

namespace {
class Input : public QIODevice
{
public:
    Input() { open(ReadOnly | Unbuffered); }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return _bytes.size() + QIODevice::bytesAvailable(); }

    void append(QByteArrayView bytes)
    {
        _bytes.append(bytes.data(), bytes.size());
        emit readyRead();
    }

protected:
    qint64 readData(char* data, qint64 maximum) override
    {
        const auto size = std::min(maximum, qint64(_bytes.size()));
        std::memcpy(data, _bytes.constData(), static_cast<size_t>(size));
        _bytes.remove(0, size);
        return size;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray _bytes;
};

class Decoder : public QNmeaPositionInfoSource
{
public:
    Decoder()
        : QNmeaPositionInfoSource(RealTimeMode)
    {}

    void decode(const QByteArray& line)
    {
        QGeoPositionInfo position;
        bool fix = false;
        // This is the same Qt parsing entrypoint used by NMEAPositionSource.
        (void) parsePosInfoFromNmeaData(line.constData(), line.size(), &position, &fix);
    }
};
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* bytes, size_t size)
{
    if (size > 65536) {
        return 0;
    }
    Input input;
    NMEAStreamSplitter splitter(&input);
    Decoder decoder;
    QObject::connect(&splitter, &NMEAStreamSplitter::sentenceReceived, &splitter,
                     [](const NMEASentenceEnvelope& sentence) {
                         if (!NMEAUtils::verifyChecksum(sentence.bytes()) || sentence.bytes().size() > 1026) {
                             std::abort();
                         }
                     });
    const auto drain = [&]() {
        auto* output = splitter.positionDevice();
        if (output->bytesAvailable() > 65536) {
            std::abort();
        }
        while (output->canReadLine()) {
            const auto line = output->readLine();
            if (!NMEAUtils::verifyChecksum(line) || line.size() > 1026) {
                std::abort();
            }
            decoder.decode(line);
        }
    };
    const size_t fragment = size ? 1 + bytes[size - 1] % 127 : 1;
    for (size_t offset = 0; offset < size; offset += fragment) {
        input.append(QByteArrayView(reinterpret_cast<const char*>(bytes + offset), std::min(fragment, size - offset)));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        drain();
    }
    input.close();
    return 0;
}
