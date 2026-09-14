#include "NMEASentenceTest.h"

#include <QtCore/QTime>
#include <QtPositioning/QGeoCoordinate>

#include "NMEASentence.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"
#include "SequentialTestDevice.h"

void NMEASentenceTest::_utcMilliseconds_data()
{
    QTest::addColumn<QByteArray>("text");
    QTest::addColumn<int>("expected");
    QTest::newRow("midnight") << QByteArray("000000") << 0;
    QTest::newRow("tenths") << QByteArray("000001.1") << 1100;
    QTest::newRow("hundredths") << QByteArray("000001.01") << 1010;
    QTest::newRow("milliseconds") << QByteArray("000001.001") << 1001;
    QTest::newRow("sub-milliseconds") << QByteArray("000001.0019") << 1001;
    QTest::newRow("end-of-day") << QByteArray("235959.9999") << 86399999;
    for (const auto& text : {"", "00000", "0000000", "000000.", "240000", "006000", "000060", "+10000", "0000-1",
                             "000001.1e2", "000001.001x", "000001. 1", "000001.1.2"}) {
        QTest::newRow(text) << QByteArray(text) << -1;
    }
}

void NMEASentenceTest::_utcMilliseconds()
{
    QFETCH(QByteArray, text);
    QFETCH(int, expected);
    QCOMPARE(NMEA::utcMilliseconds(std::string_view(text.constData(), text.size())).value_or(-1), expected);
}

void NMEASentenceTest::_qtTimestampEquivalence()
{
    // Every millisecond in one minute includes the floating-point truncation regressions.
    for (int milliseconds = 0; milliseconds < 60000; ++milliseconds) {
        const auto text = QTime::fromMSecsSinceStartOfDay(milliseconds).toString(u"hhmmss.zzz").toLatin1();
        const auto expected = QTime::fromString(QString::fromLatin1(text), u"hhmmss.z");
        const auto actual = NMEA::utcMilliseconds(std::string_view(text.constData(), text.size()));
        QVERIFY2(actual && *actual == expected.msecsSinceStartOfDay(), text.constData());
    }
}

void NMEASentenceTest::_makeGga_data()
{
    QTest::addColumn<QGeoCoordinate>("coordinate");
    QTest::addColumn<double>("altitude");
    QTest::addColumn<QByteArray>("coordinateFields");
    QTest::addColumn<QByteArray>("altitudeField");
    QTest::newRow("precision") << QGeoCoordinate(47.3977, 8.5456) << 450.0 << QByteArray("4723.8620,N,00832.7360,E")
                               << QByteArray("450.0");
    QTest::newRow("tokyo") << QGeoCoordinate(35.6762, 139.6503) << 40.0 << QByteArray("3540.5720,N,13939.0180,E")
                           << QByteArray("40.0");
    QTest::newRow("southern-western") << QGeoCoordinate(-33.8688, -151.2093) << 10.0
                                      << QByteArray("3352.1280,S,15112.5580,W") << QByteArray("10.0");
    QTest::newRow("equator") << QGeoCoordinate(0.0, 0.0) << 0.0 << QByteArray("0000.0000,N,00000.0000,E")
                             << QByteArray("0.0");
    QTest::newRow("date-line-east") << QGeoCoordinate(17.7134, 178.065) << 5.0 << QByteArray("1742.8040,N,17803.9000,E")
                                    << QByteArray("5.0");
    QTest::newRow("date-line-west") << QGeoCoordinate(17.7134, -179.5) << 5.0 << QByteArray("1742.8040,N,17930.0000,W")
                                    << QByteArray("5.0");
    QTest::newRow("zero-altitude") << QGeoCoordinate(51.5074, -0.1278) << 0.0 << QByteArray("5130.4440,N,00007.6680,W")
                                   << QByteArray("0.0");
    QTest::newRow("high-altitude") << QGeoCoordinate(27.9881, 86.9250) << 8848.9
                                   << QByteArray("2759.2860,N,08655.5000,E") << QByteArray("8848.9");
    QTest::newRow("negative-altitude") << QGeoCoordinate(31.5, 35.5) << -430.5 << QByteArray("3130.0000,N,03530.0000,E")
                                       << QByteArray("-430.5");
}

void NMEASentenceTest::_makeGga()
{
    QFETCH(QGeoCoordinate, coordinate);
    QFETCH(double, altitude);
    QFETCH(QByteArray, coordinateFields);
    QFETCH(QByteArray, altitudeField);
    const auto sentence = NMEAUtils::makeGGA(coordinate, altitude);
    const auto star = sentence.indexOf('*');
    QVERIFY(star > 0);
    QCOMPARE(sentence.size(), star + 5);
    QVERIFY(sentence.endsWith("\r\n"));
    QVERIFY(NMEAUtils::verifyChecksum(sentence));
    const auto fields = sentence.sliced(1, star - 1).split(',');
    QCOMPARE(fields.size(), 15);
    QCOMPARE(fields[0], QByteArray("GPGGA"));
    QCOMPARE(fields[1].size(), 6);
    for (const char digit : fields[1]) {
        QVERIFY(digit >= '0' && digit <= '9');
    }
    QVERIFY(QTime::fromString(QString::fromLatin1(fields[1]), u"hhmmss").isValid());
    QCOMPARE(fields.sliced(2, 4).join(','), coordinateFields);
    QCOMPARE(fields.sliced(6).join(','), "1,12,1.0," + altitudeField + ",M,0.0,M,,");
}

void NMEASentenceTest::_repairChecksum_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("expected");
    const QByteArray body = "$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,";
    const QByteArray expected = body + "*77\r\n";
    QTest::newRow("correct") << body + "*77" << expected;
    QTest::newRow("wrong") << body + "*FF" << expected;
    QTest::newRow("missing") << body << expected;
    QTest::newRow("terminated") << expected << expected;
    QTest::newRow("missing-crlf") << body + "\r\n" << expected;
    QTest::newRow("missing-lf") << body + "\n" << expected;
    QTest::newRow("valid-lf") << body + "*77\n" << expected;
    QTest::newRow("valid-with-suffix") << body + "*77garbage" << expected;
    QTest::newRow("multiple-stars") << body + "*00*77" << expected;
    QTest::newRow("truncated") << QByteArray("$GPGGA,120000,0000.0000,N,00000.0000,E,1,12,1.0,0.0,M,0.0,M,,*")
                               << QByteArray("$GPGGA,120000,0000.0000,N,00000.0000,E,1,12,1.0,0.0,M,0.0,M,,*73\r\n");
    QTest::newRow("repair-and-terminate")
        << QByteArray("$GPGGA,000000,0000.0000,N,00000.0000,E,1,12,1.0,0.0,M,0.0,M,,*00")
        << QByteArray("$GPGGA,000000,0000.0000,N,00000.0000,E,1,12,1.0,0.0,M,0.0,M,,*70\r\n");
    QTest::newRow("short") << QByteArray("$GP") << QByteArray("$GP\r\n");
    QTest::newRow("empty") << QByteArray() << QByteArray("\r\n");
}

void NMEASentenceTest::_repairChecksum()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, expected);
    QCOMPARE(NMEAUtils::repairChecksum(input), expected);
}

UT_REGISTER_TEST(NMEASentenceTest, TestLabel::Unit)

void NMEASentenceTest::_frameValidation_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<bool>("valid");
    QTest::newRow("plain") << QByteArray("$GNTXT,p*0D") << true;
    QTest::newRow("lowercase") << QByteArray("$GNTXT,p*0d") << true;
    QTest::newRow("lf") << QByteArray("$GNTXT,p*0D\n") << true;
    QTest::newRow("crlf") << QByteArray("$GNTXT,p*0D\r\n") << true;
    for (const auto* text :
         {"$GNTXT,p*+D", "$GNTXT,p*-D", "$GNTXT,p*0Dgarbage", "$GNTXT,p*0D ", "$GNTXT,p*D", "$GNTXT,p*00D",
          "$GNTXT,p*00*0D", "$GNTXT,p*GG", "$GNTXT,p*00", "$GNTXT,p", "$GNTXT,p*0D\rx"}) {
        QTest::newRow(text) << QByteArray(text) << false;
    }
}

void NMEASentenceTest::_frameValidation()
{
    QFETCH(QByteArray, input);
    QFETCH(bool, valid);
    const std::string_view view(input.constData(), input.size());
    const auto frame = NMEA::frame(view);
    QCOMPARE(frame && frame->hasValidChecksum(), valid);
    QCOMPARE(NMEA::sentence(view).has_value(), valid);
    QCOMPARE(NMEAUtils::verifyChecksum(input), valid);
    QCOMPARE(NMEASentenceEnvelope::parse(input, 1).has_value(), valid);
    SequentialTestDevice device;
    NMEAStreamSplitter splitter(&device);
    int received = 0;
    connect(&splitter, &NMEAStreamSplitter::sentenceReceived, this, [&](const auto&) { ++received; });
    device.feed(input.endsWith('\n') ? input : input + '\n');
    QCOMPARE(received, valid ? 1 : 0);
}

void NMEASentenceTest::_borrowedBytesAreOwned()
{
    std::optional<NMEASentenceEnvelope> retained;
    {
        QByteArray storage("$GNTXT,p*0D\r\n");
        retained = NMEASentenceEnvelope::parse(QByteArray::fromRawData(storage.constData(), storage.size()), 42);
        QVERIFY(retained);
        storage.fill('x');
    }
    QCOMPARE(retained->bytes(), QByteArray("$GNTXT,p*0D\r\n"));
    QCOMPARE(retained->sentence().fields[1], std::string_view("p"));
    QCOMPARE(retained->receivedAtUs(), quint64(42));
}
