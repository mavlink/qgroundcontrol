#include "NMEASentenceTest.h"

#include <array>

#include <QtCore/QTime>

#include "NMEAFramer.h"
#include "NMEASentence.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"
#include "SequentialTestDevice.h"

Q_DECLARE_METATYPE(NMEA::GGA)

void NMEASentenceTest::_incrementalFraming_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QList<QByteArray>>("expected");
    QTest::newRow("no-line-ending") << QByteArray("$A*41") << QList<QByteArray>{"$A*41"};
    QTest::newRow("line-ending") << QByteArray("$A*41\r\n") << QList<QByteArray>{"$A*41"};
    QTest::newRow("adjacent-frames") << QByteArray("$A*41$Z*5A") << QList<QByteArray>{"$A*41", "$Z*5A"};
    QTest::newRow("embedded-sync") << QByteArray("$partial$A*41") << QList<QByteArray>{"$A*41"};
    QTest::newRow("bad-checksum") << QByteArray("noise$A*00$Z*5A") << QList<QByteArray>{"$Z*5A"};
    QTest::newRow("lowercase-checksum") << QByteArray("$Z*5a") << QList<QByteArray>{};
    QTest::newRow("incomplete") << QByteArray("$A*4") << QList<QByteArray>{};
    const QByteArray maximum = "$" + QByteArray(9, 'A') + "*41";
    QTest::newRow("maximum-length") << maximum << QList<QByteArray>{maximum};
    QTest::newRow("overlong-resync") << "$" + QByteArray(10, 'A') + "*00$Z*5A" << QList<QByteArray>{"$Z*5A"};
}

void NMEASentenceTest::_incrementalFraming()
{
    QFETCH(QByteArray, input);
    QFETCH(QList<QByteArray>, expected);
    std::array<uint8_t, 18> storage;
    storage.fill(0xA5);
    auto buffer = std::span(storage).subspan(1, 16);
    NMEA::Framer framer(buffer);
    QList<QByteArray> frames;
    for (const auto byte : input) {
        if (const auto length = framer.addByte(static_cast<uint8_t>(byte)); length > 0) {
            frames.append(QByteArray(reinterpret_cast<const char*>(buffer.data()), static_cast<qsizetype>(length)));
        }
    }
    QCOMPARE(frames, expected);
    QCOMPARE(storage.front(), uint8_t(0xA5));
    QCOMPARE(storage.back(), uint8_t(0xA5));
}

void NMEASentenceTest::_incrementalReset()
{
    std::array<uint8_t, 16> buffer{};
    NMEA::Framer framer(buffer);
    for (const auto byte : QByteArray("$A*")) {
        QCOMPARE(framer.addByte(static_cast<uint8_t>(byte)), size_t(0));
    }
    framer.reset();
    for (const auto byte : QByteArray("41$A*4")) {
        QCOMPARE(framer.addByte(static_cast<uint8_t>(byte)), size_t(0));
    }
    QCOMPARE(framer.addByte('1'), size_t(5));
}

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
    QTest::addColumn<NMEA::GGA>("fix");
    QTest::addColumn<QTime>("utc");
    QTest::addColumn<QByteArray>("body");
    const auto addCoordinate = [](const char* name, double latitude, double longitude, double altitude,
                                  const QByteArray& coordinateFields, const QByteArray& altitudeField) {
        const NMEA::GGA fix{
            .latitude = latitude,
            .longitude = longitude,
            .altitude = altitude,
            .geoidSeparation = 0.0,
            .hdop = 1.0,
            .quality = NMEA::GgaQuality::GPS,
            .satellitesUsed = 12,
        };
        QTest::newRow(name) << fix << QTime(12, 34, 56, 789)
                            << "GPGGA,123456," + coordinateFields + ",1,12,1.0," + altitudeField + ",M,0.0,M,,";
    };
    addCoordinate("precision", 47.3977, 8.5456, 450.0, "4723.8620,N,00832.7360,E", "450.0");
    addCoordinate("tokyo", 35.6762, 139.6503, 40.0, "3540.5720,N,13939.0180,E", "40.0");
    addCoordinate("southern-western", -33.8688, -151.2093, 10.0, "3352.1280,S,15112.5580,W", "10.0");
    addCoordinate("equator", 0.0, 0.0, 0.0, "0000.0000,N,00000.0000,E", "0.0");
    addCoordinate("date-line-east", 17.7134, 178.065, 5.0, "1742.8040,N,17803.9000,E", "5.0");
    addCoordinate("date-line-west", 17.7134, -179.5, 5.0, "1742.8040,N,17930.0000,W", "5.0");
    addCoordinate("zero-altitude", 51.5074, -0.1278, 0.0, "5130.4440,N,00007.6680,W", "0.0");
    addCoordinate("high-altitude", 27.9881, 86.9250, 8848.9, "2759.2860,N,08655.5000,E", "8848.9");
    addCoordinate("negative-altitude", 31.5, 35.5, -430.5, "3130.0000,N,03530.0000,E", "-430.5");
    addCoordinate("rounded-degree", 12.9999994, -179.9999994, 25.0, "1300.0000,N,18000.0000,W", "25.0");
    addCoordinate("coordinate-limits", 90.0, 180.0, 25.0, "9000.0000,N,18000.0000,E", "25.0");

    const NMEA::GGA explicitFix{
        .latitude = 47.3977,
        .longitude = 8.5456,
        .altitude = 0.0,
        .geoidSeparation = -31.5,
        .hdop = 0.7,
        .quality = NMEA::GgaQuality::RTK_FIXED,
        .satellitesUsed = 0,
    };
    QTest::newRow("explicit-metadata") << explicitFix << QTime(0, 0)
                                       << QByteArray("GPGGA,000000,4723.8620,N,00832.7360,E,4,0,0.7,0.0,M,-31.5,M,,");
    const NMEA::GGA unknownFix{.latitude = 47.3977, .longitude = 8.5456};
    QTest::newRow("unknown-metadata") << unknownFix << QTime(23, 59, 59, 999)
                                      << QByteArray("GPGGA,235959,4723.8620,N,00832.7360,E,0,,,,M,,M,,");
    QTest::newRow("invalid-utc") << explicitFix << QTime() << QByteArray();
    const auto invalidField = [&explicitFix](const char* name, double NMEA::GGA::* field, double value) {
        auto fix = explicitFix;
        fix.*field = value;
        QTest::newRow(name) << fix << QTime(12, 0) << QByteArray();
    };
    invalidField("unknown-latitude", &NMEA::GGA::latitude, qQNaN());
    invalidField("latitude-out-of-range", &NMEA::GGA::latitude, 90.1);
    invalidField("infinite-longitude", &NMEA::GGA::longitude, qInf());
    invalidField("longitude-out-of-range", &NMEA::GGA::longitude, -180.1);
    invalidField("infinite-altitude", &NMEA::GGA::altitude, qInf());
    invalidField("infinite-geoid", &NMEA::GGA::geoidSeparation, -qInf());
    invalidField("infinite-hdop", &NMEA::GGA::hdop, qInf());
    invalidField("negative-hdop", &NMEA::GGA::hdop, -0.1);
    auto invalidQuality = explicitFix;
    invalidQuality.quality = NMEA::GgaQuality::MAX_VALUE + 1;
    QTest::newRow("invalid-quality") << invalidQuality << QTime(12, 0) << QByteArray();
}

void NMEASentenceTest::_makeGga()
{
    QFETCH(NMEA::GGA, fix);
    QFETCH(QTime, utc);
    QFETCH(QByteArray, body);
    const auto sentence = NMEAUtils::makeGGA(fix, utc);
    if (body.isEmpty()) {
        QVERIFY(sentence.isEmpty());
        return;
    }
    const auto star = sentence.indexOf('*');
    QVERIFY(star > 0);
    QCOMPARE(sentence.size(), star + 5);
    QVERIFY(sentence.endsWith("\r\n"));
    QVERIFY(NMEAUtils::verifyChecksum(sentence));
    QCOMPARE(sentence.sliced(1, star - 1), body);
    const auto fields = sentence.sliced(1, star - 1).split(',');
    QCOMPARE(fields.size(), 15);
    QCOMPARE(fields[0], QByteArray("GPGGA"));
    QCOMPARE(fields[1], utc.toString(u"hhmmss").toLatin1());
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
