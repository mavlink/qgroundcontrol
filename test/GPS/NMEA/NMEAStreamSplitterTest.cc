#include "NMEAStreamSplitterTest.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>

#include <algorithm>

#include "GPSByteStream.h"
#include "GPSObservation.h"
#include "NMEAPositionSource.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"

namespace {
class StreamInput : public QIODevice
{
public:
    StreamInput() { open(ReadOnly); }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override { return QIODevice::bytesAvailable() + _data.size(); }

    void feed(const QByteArray& data)
    {
        _data.append(data);
        emit readyRead();
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        const qint64 size = std::min<qint64>(_data.size(), maxSize);
        std::copy_n(_data.constData(), size, data);
        _data.remove(0, size);
        return size;
    }

    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QByteArray _data;
};
}  // namespace

void NMEAStreamSplitterTest::_rawPartialReadOverflow()
{
    TimestampedByteBuffer buffer;
    const auto received = GPSObservation::monotonicNowUs();
    const QByteArray shared(50000, 'a');
    QVERIFY(buffer.append(shared, received));
    QByteArray output(10000, '\0');
    const auto read = [&] { return buffer.read(output.data(), output.size(), received, 5000000); };
    auto result = read();
    QCOMPARE(result.bytes, 10000);
    QCOMPARE(output, QByteArray(10000, 'a'));
    QCOMPARE(result.receivedAtUs, received);
    QCOMPARE(buffer.size(), 40000);
    buffer.append(QByteArray(30000, 'b'), received);
    QCOMPARE(buffer.size(), 30000);
    QVERIFY(buffer.gapPending());
    result = read();
    QVERIFY(result.gap);
    QCOMPARE(result.bytes, 0);
    QCOMPARE(buffer.size(), 30000);
    for (int i = 0; i < 3; ++i) {
        result = read();
        QCOMPARE(result.bytes, 10000);
        QCOMPARE(output, QByteArray(10000, 'b'));
        QCOMPARE(result.receivedAtUs, received);
    }
    QCOMPARE(buffer.size(), 0);
    QCOMPARE(shared, QByteArray(50000, 'a'));
}

void NMEAStreamSplitterTest::_independentReads()
{
    StreamInput input;
    {
        NMEAStreamSplitter stream(&input);
        auto* position = stream.positionDevice();
        QList<NMEASentenceEnvelope> sentences;
        connect(&stream, &NMEAStreamSplitter::sentenceReceived, this,
                [&](const NMEASentenceEnvelope& sentence) { sentences.append(sentence); });
        const QByteArray first = NMEAUtils::repairChecksum("$GNTXT,first");
        const QByteArray next = NMEAUtils::repairChecksum("$GNTXT,next");
        input.feed(first.first(5));
        QCOMPARE(position->bytesAvailable(), 0);
        QVERIFY(sentences.isEmpty());
        input.feed(first.mid(5) + next);
        QCOMPARE(position->read(2), first.first(2));
        QCOMPARE(position->readAll(), first.mid(2) + next);
        QCOMPARE(sentences.size(), 2);
        QCOMPARE(sentences.first().bytes(), first);
        QCOMPARE(sentences.last().bytes(), next);
        QCOMPARE(sentences.first().sentence().fields[1], std::string_view("first"));
        input.feed(first);
        position->close();
        QVERIFY(position->open(QIODevice::ReadOnly));
        QVERIFY(position->readAll().isEmpty());
        QCOMPARE(sentences.last().bytes(), first);
    }
    QVERIFY(input.isOpen());
}

void NMEAStreamSplitterTest::_slowConsumerIsBounded()
{
    StreamInput input;
    NMEAStreamSplitter stream(&input);
    QByteArray received;
    connect(&stream, &NMEAStreamSplitter::sentenceReceived, this,
            [&](const NMEASentenceEnvelope& sentence) { received += sentence.bytes(); });
    const QByteArray line = NMEAUtils::repairChecksum("$GNTXT,buffer");
    const QByteArray lines = line.repeated(8000);
    input.feed(lines);
    QVERIFY(received.size() < lines.size());
    QTRY_COMPARE_WITH_TIMEOUT(received, lines, TestTimeout::mediumMs());
    QVERIFY(stream.positionDevice()->bytesAvailable() <= 64 * 1024);
    const QByteArray buffered = stream.positionDevice()->readAll();
    QVERIFY(!buffered.isEmpty());
    QVERIFY(lines.endsWith(buffered));
    QVERIFY(buffered.startsWith(line));
    input.feed("$" + QByteArray(80 * 1024, 'x'));
    QVERIFY(stream.positionDevice()->readAll().isEmpty());
    input.feed("rest\n" + line);
    QTRY_VERIFY_WITH_TIMEOUT(stream.positionDevice()->canReadLine(), TestTimeout::mediumMs());
    QCOMPARE(stream.positionDevice()->readAll(), line);
}

void NMEAStreamSplitterTest::_queuedSentenceOwnsItsBytes()
{
    QList<NMEASentenceEnvelope> received;
    {
        StreamInput input;
        NMEAStreamSplitter stream(&input);
        connect(
            &stream, &NMEAStreamSplitter::sentenceReceived, this,
            [&](const NMEASentenceEnvelope& sentence) { received.append(sentence); }, Qt::QueuedConnection);
        input.feed(NMEAUtils::repairChecksum("$GNTXT,retained"));
        QVERIFY(received.isEmpty());
        stream.positionDevice()->readAll();
    }
    QTRY_COMPARE_WITH_TIMEOUT(received.size(), 1, TestTimeout::shortMs());
    QCOMPARE(received.first().sentence().fields[1], std::string_view("retained"));
    QCOMPARE(received.first().bytes(), NMEAUtils::repairChecksum("$GNTXT,retained"));
    QVERIFY(received.first().receivedAtUs() > 0);
}

void NMEAStreamSplitterTest::_sourceDestructionClosesOutputs()
{
    auto input = std::make_unique<StreamInput>();
    NMEAStreamSplitter stream(input.get());
    input->feed(NMEAUtils::repairChecksum("$GNTXT,pending"));
    input.reset();
    QVERIFY(!stream.positionDevice()->isOpen());
    QCOMPARE(stream.positionDevice()->bytesAvailable(), 0);
}

void NMEAStreamSplitterTest::_destructionDuringDelivery()
{
    StreamInput input;
    auto stream = std::make_unique<NMEAStreamSplitter>(&input);
    const QPointer<QIODevice> position(stream->positionDevice());
    connect(stream->positionDevice(), &QIODevice::readyRead, this, [&]() { stream.reset(); });
    input.feed(NMEAUtils::repairChecksum("$GNTXT,one") + NMEAUtils::repairChecksum("$GNTXT,two"));
    QVERIFY(!stream);
    QVERIFY(!position);
    QVERIFY(input.isOpen());
}

void NMEAStreamSplitterTest::_mixedBinaryAndFragmentedSentences()
{
    StreamInput input;
    NMEAStreamSplitter stream(&input);
    NMEAPositionSource position(stream.positionDevice());
    QByteArray sentences;
    connect(&stream, &NMEAStreamSplitter::sentenceReceived, this,
            [&](const NMEASentenceEnvelope& sentence) { sentences += sentence.bytes(); });
    QSignalSpy fixes(&position, &QGeoPositionInfoSource::positionUpdated);
    position.startUpdates();
    const QByteArray rmc = NMEAUtils::repairChecksum("$GNRMC,120000.00,A,3724.000,N,07918.000,W,0.0,0.0,090926,,,A");
    const QByteArray gga = NMEAUtils::repairChecksum("$GNGGA,120000.00,3724.000,N,07918.000,W,1,12,0.8,100,M,0,M,,");
    const QByteArray garbage = QByteArray("$GNRMC,broken*00\r\n") + QByteArray::fromHex("b56201070c000001242aff00");
    for (const char byte : garbage + rmc + gga) {
        input.feed(QByteArray(1, byte));
    }
    QCOMPARE(sentences, rmc + gga);
    QTRY_VERIFY_WITH_TIMEOUT(!fixes.isEmpty(), TestTimeout::shortMs());
    QVERIFY(fixes.first().first().value<QGeoPositionInfo>().isValid());
}

UT_REGISTER_TEST(NMEAStreamSplitterTest, TestLabel::Unit)
