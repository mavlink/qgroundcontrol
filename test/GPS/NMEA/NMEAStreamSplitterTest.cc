#include "NMEAStreamSplitterTest.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>

#include "GPSObservation.h"
#include "ManualScheduler.h"
#include "MonotonicClock.h"
#include "NMEAPositionSource.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"
#include "SequentialTestDevice.h"

void NMEAStreamSplitterTest::_independentReads()
{
    SequentialTestDevice input;
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
    SequentialTestDevice input;
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
        SequentialTestDevice input;
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
    auto input = std::make_unique<SequentialTestDevice>();
    NMEAStreamSplitter stream(input.get());
    input->feed(NMEAUtils::repairChecksum("$GNTXT,pending"));
    input.reset();
    QVERIFY(!stream.positionDevice()->isOpen());
    QCOMPARE(stream.positionDevice()->bytesAvailable(), 0);
}

void NMEAStreamSplitterTest::_destructionDuringDelivery()
{
    SequentialTestDevice input;
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
    SequentialTestDevice input;
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

void NMEAStreamSplitterTest::_preloadedInput()
{
    SequentialTestDevice input;
    input.feed(NMEAUtils::repairChecksum("$GPRMC,000001.001,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A") +
               NMEAUtils::repairChecksum("$GPGGA,000001.001,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,"));
    NMEAStreamSplitter stream(&input);
    QSignalSpy sentences(&stream, &NMEAStreamSplitter::sentenceReceived);
    NMEAPositionSource source(stream.positionDevice());
    QSignalSpy fixes(&source, &QGeoPositionInfoSource::positionUpdated);
    source.startUpdates();
    QTRY_COMPARE_WITH_TIMEOUT(sentences.size(), 2, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(fixes.size(), 1, TestTimeout::shortMs());
    const auto observation = source.lastObservation();
    QCOMPARE(observation.position.timestamp().date(), QDate(2011, 5, 28));
    QCOMPARE(observation.position.timestamp().time(), QTime(0, 0, 1, 1));
    QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::Unknown);
    QCOMPARE(observation.satellitesUsed, std::optional<unsigned>(8));
}

void NMEAStreamSplitterTest::_closeDuringDelivery()
{
    SequentialTestDevice input;
    NMEAStreamSplitter stream(&input);
    QSignalSpy sentences(&stream, &NMEAStreamSplitter::sentenceReceived);
    QSignalSpy closed(&stream, &NMEAStreamSplitter::closed);
    connect(stream.positionDevice(), &QIODevice::readyRead, &input, &QIODevice::close);
    input.feed(NMEAUtils::repairChecksum("$GNTXT,one") + NMEAUtils::repairChecksum("$GNTXT,two"));
    QCOMPARE(closed.size(), 1);
    QCOMPARE(sentences.size(), 0);
    QVERIFY(!stream.positionDevice()->isOpen());
}

void NMEAStreamSplitterTest::_delayedChunksPreserveReceipts()
{
    ManualScheduler scheduler;
    SequentialTestDevice input(&scheduler);
    const auto firstReceipt = scheduler.nowUs();
    const QByteArray first = NMEAUtils::repairChecksum("$GNTXT,first");
    input.feed(first.first(5), false);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    const auto secondReceipt = scheduler.nowUs();
    input.feed(first.mid(5) + NMEAUtils::repairChecksum("$GNTXT,second"), false);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    NMEAStreamSplitter splitter(&input, nullptr, &scheduler);
    QList<quint64> receipts;
    connect(&splitter, &NMEAStreamSplitter::sentenceReceived, this,
            [&](const auto& sentence) { receipts.append(sentence.receivedAtUs()); });
    QTRY_COMPARE_WITH_TIMEOUT(receipts.size(), 2, TestTimeout::shortMs());
    QCOMPARE(receipts, QList<quint64>({firstReceipt, secondReceipt}));
}
