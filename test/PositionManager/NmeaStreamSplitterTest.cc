#include "NmeaStreamSplitterTest.h"
#include "NmeaStreamSplitter.h"

#include <QtCore/QBuffer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

static const QByteArray kSentence1 = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
static const QByteArray kSentence2 = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n";


void NmeaStreamSplitterTest::testPipesCreated()
{
    QBuffer source;
    source.open(QIODevice::ReadOnly);
    NmeaStreamSplitter splitter(&source);

    QVERIFY(splitter.positionPipe() != nullptr);
    QVERIFY(splitter.satellitePipe() != nullptr);
    QVERIFY(splitter.positionPipe() != splitter.satellitePipe());
}

void NmeaStreamSplitterTest::testDataReachesPositionPipe()
{
    QBuffer source;
    source.open(QIODevice::ReadWrite);
    NmeaStreamSplitter splitter(&source);

    source.write(kSentence1);
    source.seek(0);  // reset read position so readAll() returns the data
    emit source.readyRead();

    const QByteArray received = splitter.positionPipe()->readAll();
    QCOMPARE(received, kSentence1);
}

void NmeaStreamSplitterTest::testDataReachesSatellitePipe()
{
    QBuffer source;
    source.open(QIODevice::ReadWrite);
    NmeaStreamSplitter splitter(&source);

    source.write(kSentence1);
    source.seek(0);
    emit source.readyRead();

    const QByteArray received = splitter.satellitePipe()->readAll();
    QCOMPARE(received, kSentence1);
}

void NmeaStreamSplitterTest::testReadyReadEmittedOnFeed()
{
    QBuffer source;
    source.open(QIODevice::ReadWrite);
    NmeaStreamSplitter splitter(&source);

    QSignalSpy posSpy(splitter.positionPipe(), &QIODevice::readyRead);
    QSignalSpy satSpy(splitter.satellitePipe(), &QIODevice::readyRead);

    source.write(kSentence1);
    source.seek(0);
    emit source.readyRead();

    QCOMPARE(posSpy.count(), 1);
    QCOMPARE(satSpy.count(), 1);
}

void NmeaStreamSplitterTest::testCanReadLineWithNewline()
{
    NmeaPipeDevice pipe;
    pipe.feedData(kSentence1);
    QVERIFY(pipe.canReadLine());
}

void NmeaStreamSplitterTest::testCanReadLineWithoutNewline()
{
    NmeaPipeDevice pipe;
    pipe.feedData("$GPGGA,123519,partial");
    QVERIFY(!pipe.canReadLine());
}

void NmeaStreamSplitterTest::testReadLineSingleSentence()
{
    NmeaPipeDevice pipe;
    pipe.feedData(kSentence1);

    const QByteArray line = pipe.readLine();
    QCOMPARE(line, kSentence1);
}

void NmeaStreamSplitterTest::testMultipleSentencesSplitCorrectly()
{
    NmeaPipeDevice pipe;
    pipe.feedData(kSentence1 + kSentence2);

    const QByteArray first = pipe.readLine();
    const QByteArray second = pipe.readLine();

    QCOMPARE(first, kSentence1);
    QCOMPARE(second, kSentence2);
    QCOMPARE(pipe.bytesAvailable(), qint64(0));
}

void NmeaStreamSplitterTest::testPartialThenCompleteSentence()
{
    NmeaPipeDevice pipe;
    const QByteArray part1 = kSentence1.left(20);
    const QByteArray part2 = kSentence1.mid(20);

    pipe.feedData(part1);
    QVERIFY(!pipe.canReadLine());

    pipe.feedData(part2);
    QVERIFY(pipe.canReadLine());

    const QByteArray line = pipe.readLine();
    QCOMPARE(line, kSentence1);
}

void NmeaStreamSplitterTest::testEmptyDataDoesNotCrash()
{
    QBuffer source;
    source.open(QIODevice::ReadWrite);
    NmeaStreamSplitter splitter(&source);

    // Write nothing, then signal readyRead — splitter must not crash
    emit source.readyRead();

    QCOMPARE(splitter.positionPipe()->bytesAvailable(), qint64(0));
    QCOMPARE(splitter.satellitePipe()->bytesAvailable(), qint64(0));
}

void NmeaStreamSplitterTest::testBytesAvailableReflectsBuffer()
{
    NmeaPipeDevice pipe;
    QCOMPARE(pipe.bytesAvailable(), qint64(0));

    pipe.feedData(kSentence1);
    QCOMPARE(pipe.bytesAvailable(), qint64(kSentence1.size()));

    (void) pipe.readAll();
    QCOMPARE(pipe.bytesAvailable(), qint64(0));
}

UT_REGISTER_TEST(NmeaStreamSplitterTest, TestLabel::Unit)
