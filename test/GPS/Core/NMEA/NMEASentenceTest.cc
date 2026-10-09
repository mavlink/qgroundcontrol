#include "NMEASentenceTest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <QtCore/QByteArray>
#include <QtCore/QTime>

#include "NMEAFramer.h"
#include "NMEANavigationEpoch.h"
#include "NMEASatellites.h"
#include "NMEASentence.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/fixtures/GPSFixtureExpectations.h"

using GPSTest::checksumValid;

namespace {
/// Owns the bytes that a parsed sentence's field views reference.
struct OwnedSentence
{
    QByteArray bytes;
    NMEA::Sentence parsed;

    const NMEA::Sentence& sentence() const { return parsed; }
};

std::unique_ptr<OwnedSentence> ownedSentence(const QByteArray& input)
{
    auto result = std::make_unique<OwnedSentence>();
    result->bytes = input;
    result->bytes.detach();
    const auto sentence = NMEA::sentence({result->bytes.constData(), static_cast<size_t>(result->bytes.size())});
    if (!sentence) {
        return nullptr;
    }
    result->parsed = *sentence;
    return result;
}
}  // namespace

void NMEASentenceTest::_fixQuality_data()
{
    QTest::addColumn<unsigned>("quality");
    QTest::addColumn<GPSFixQuality>("autonomous");
    QTest::addColumn<GPSFixQuality>("expected");
    const std::array qualities{
        GPSFixQuality::NoFix,        GPSFixQuality::Unknown,  GPSFixQuality::Differential,
        GPSFixQuality::Unknown,      GPSFixQuality::RTKFixed, GPSFixQuality::RTKFloat,
        GPSFixQuality::Extrapolated, GPSFixQuality::Fix3D,    GPSFixQuality::Unknown,
    };
    for (unsigned quality = 0; quality < qualities.size(); ++quality) {
        QTest::addRow("%u", quality) << quality << GPSFixQuality::Fix3D
                                     << (quality == NMEA::GgaQuality::GPS ? GPSFixQuality::Fix3D : qualities[quality]);
    }
    // Only an autonomous fix takes the dimension the receiver reports elsewhere.
    QTest::newRow("gps-2d") << unsigned(NMEA::GgaQuality::GPS) << GPSFixQuality::Fix2D << GPSFixQuality::Fix2D;
    QTest::newRow("gps-unknown") << unsigned(NMEA::GgaQuality::GPS) << GPSFixQuality::Unknown << GPSFixQuality::Unknown;
    QTest::newRow("unsupported") << 255u << GPSFixQuality::Fix3D << GPSFixQuality::Unknown;
}

void NMEASentenceTest::_fixQuality()
{
    QFETCH(unsigned, quality);
    QFETCH(GPSFixQuality, autonomous);
    QFETCH(GPSFixQuality, expected);
    QCOMPARE(NMEA::fixQuality(quality, autonomous), expected);
    QCOMPARE(gpsFixQualityFromValue(7), GPSFixQuality::Unknown);
    QCOMPARE(gpsFixQualityFromValue(8), GPSFixQuality::Extrapolated);
}

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

void NMEASentenceTest::_lineFraming_data()
{
    QTest::addColumn<QList<QByteArray>>("chunks");
    QTest::addColumn<int>("maxLine");
    QTest::addColumn<QList<QByteArray>>("expectedLines");
    QTest::addColumn<QList<QByteArray>>("expectedParsed");

    const QByteArray valid("$GNTXT,p*0D");
    const int exactMaxLine = static_cast<int>(valid.size());
    QTest::newRow("overlong-resync") << QList<QByteArray>{QByteArray("$") + QByteArray(12, 'A') + "\n" + valid + "\n"}
                                     << exactMaxLine << QList<QByteArray>{valid} << QList<QByteArray>{valid};
    QTest::newRow("exact-boundary") << QList<QByteArray>{valid + "\n"} << exactMaxLine << QList<QByteArray>{valid}
                                    << QList<QByteArray>{valid};
    QTest::newRow("control-discards-line") << QList<QByteArray>{QByteArray("$GNTXT,p\x01*0D\n") + valid + "\n"} << 64
                                           << QList<QByteArray>{valid} << QList<QByteArray>{valid};
    QTest::newRow("bad-checksum-then-valid") << QList<QByteArray>{QByteArray("$GNTXT,p*00\n") + valid + "\n"} << 64
                                             << QList<QByteArray>{"$GNTXT,p*00", valid} << QList<QByteArray>{valid};
    QTest::newRow("embedded-start-restarts") << QList<QByteArray>{QByteArray("$GNTXT,partial") + valid + "\n"} << 64
                                             << QList<QByteArray>{valid} << QList<QByteArray>{valid};
    QTest::newRow("split-across-reads") << QList<QByteArray>{"$GNT", "XT,p*0", "D\r\n"} << 64
                                        << QList<QByteArray>{valid} << QList<QByteArray>{valid};
    QTest::newRow("crlf-and-lf") << QList<QByteArray>{valid + "\r\n" + valid + "\n"} << 64
                                 << QList<QByteArray>{valid, valid} << QList<QByteArray>{valid, valid};
}

void NMEASentenceTest::_lineFraming()
{
    QFETCH(QList<QByteArray>, chunks);
    QFETCH(int, maxLine);
    QFETCH(QList<QByteArray>, expectedLines);
    QFETCH(QList<QByteArray>, expectedParsed);

    std::array<char, 128> storage{};
    NMEA::LineFramer framer(std::span<char>(storage.data(), static_cast<size_t>(maxLine)));
    QList<QByteArray> lines;
    QList<QByteArray> parsed;
    for (const auto& chunk : chunks) {
        for (const char byte : chunk) {
            const auto framed = framer.addByte(static_cast<uint8_t>(byte));
            if (!framed) {
                continue;
            }
            const auto line = QByteArray(framed->data(), static_cast<qsizetype>(framed->size()));
            lines.append(line);
            if (NMEA::sentence(std::string_view(line.constData(), static_cast<size_t>(line.size())))) {
                parsed.append(line);
            }
        }
    }
    QCOMPARE(lines, expectedLines);
    QCOMPARE(parsed, expectedParsed);
}

void NMEASentenceTest::_navigationFreshnessBoundaries()
{
    const auto rmc =
        ownedSentence(NMEAUtils::repairChecksum("$GPRMC,000000.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A"));
    const auto gga =
        ownedSentence(NMEAUtils::repairChecksum("$GPGGA,000000.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,"));
    const auto gsa = ownedSentence(NMEAUtils::repairChecksum("$GPGSA,A,3,02,,,,,,,,,,,,1.0,1.03,0.6"));
    const auto gst = ownedSentence(NMEAUtils::repairChecksum("$GPGST,000000.000,1,1,1,0,3,4,6"));
    QVERIFY(rmc && gga && gsa && gst);

    constexpr quint64 maxAgeUs = NMEA::NavigationEpochAssembler::METADATA_MAX_AGE.count();
    const auto gsaFresh = [&](quint64 ageUs) {
        NMEA::NavigationEpochAssembler assembler;
        if (!assembler.ingest(rmc->sentence(), 1000000) || !assembler.ingest(gga->sentence(), 1000000)) {
            return std::optional<NMEA::NavigationUpdate>();
        }
        return assembler.ingest(gsa->sentence(), 1000000 + ageUs);
    };
    QVERIFY(gsaFresh(maxAgeUs).has_value());
    QVERIFY(!gsaFresh(maxAgeUs + 1).has_value());

    const auto gstFresh = [&](quint64 ageUs) {
        NMEA::NavigationEpochAssembler assembler;
        if (!assembler.ingest(rmc->sentence(), 1000000)) {
            return false;
        }
        assembler.ingest(gst->sentence(), 1000000);
        const auto update = assembler.ingest(gga->sentence(), 1000000 + ageUs);
        return update && update->epoch.horizontalAccuracyMeters.has_value();
    };
    QVERIFY(gstFresh(maxAgeUs - 1));
    QVERIFY(!gstFresh(maxAgeUs + 1));
}

void NMEASentenceTest::_navigationFixLoss_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::newRow("gga-quality-zero") << QByteArray("$GPGGA,092751.000,,,,,0,0,,,,,,,");
    QTest::newRow("rmc-status-void") << QByteArray("$GPRMC,092751.000,V,,,,,,,280511,,,N");
}

void NMEASentenceTest::_navigationFixLoss()
{
    QFETCH(QByteArray, body);
    const auto sentence = ownedSentence(NMEAUtils::repairChecksum(body));
    QVERIFY(sentence);
    NMEA::NavigationEpochAssembler assembler;
    const auto update = assembler.ingest(sentence->sentence(), 42);
    QVERIFY(update);
    QVERIFY(update->type == NMEA::NavigationUpdate::Type::FixLoss);
    QCOMPARE(update->epoch.fixQuality, GPSFixQuality::NoFix);
    QCOMPARE(update->epoch.receivedAtUs, quint64(42));
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
    QVERIFY(checksumValid(sentence));
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

void NMEASentenceTest::_ggaHdopValidation_data()
{
    QTest::addColumn<QByteArray>("hdop");
    QTest::addColumn<double>("expected");
    QTest::newRow("valid") << QByteArray("1.03") << 1.03;
    QTest::newRow("zero") << QByteArray("0.0") << 0.0;
    QTest::newRow("negative") << QByteArray("-1.03") << qQNaN();
    QTest::newRow("empty") << QByteArray() << qQNaN();
}

void NMEASentenceTest::_ggaHdopValidation()
{
    QFETCH(QByteArray, hdop);
    QFETCH(double, expected);
    const auto sentence = ownedSentence(
        NMEAUtils::repairChecksum("$GPGGA,000000.000,5321.6802,N,00630.3372,W,1,8," + hdop + ",61.7,M,55.2,M,,"));
    QVERIFY(sentence);
    const auto fix = NMEA::gga(sentence->sentence());
    QVERIFY(fix);
    if (qIsNaN(expected)) {
        QVERIFY(qIsNaN(fix->hdop));
    } else {
        QCOMPARE(fix->hdop, expected);
    }
}

void NMEASentenceTest::_gstFieldCounts_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("parsed");
    QTest::addColumn<double>("vertical");
    QTest::newRow("standard") << QByteArray("$GPGST,000000.000,1,1,1,0,3,4,6") << true << 6.0;
    QTest::newRow("no-altitude") << QByteArray("$GPGST,000000.000,1,1,1,0,3,4") << true << qQNaN();
    QTest::newRow("proprietary-suffix") << QByteArray("$GPGST,000000.000,1,1,1,0,3,4,6,9") << true << 6.0;
    QTest::newRow("missing-longitude") << QByteArray("$GPGST,000000.000,1,1,1,0,3") << false << qQNaN();
}

void NMEASentenceTest::_gstFieldCounts()
{
    QFETCH(QByteArray, body);
    QFETCH(bool, parsed);
    QFETCH(double, vertical);
    const auto sentence = ownedSentence(NMEAUtils::repairChecksum(body));
    QVERIFY(sentence);
    const auto gst = NMEA::gst(sentence->sentence());
    QCOMPARE(gst.has_value(), parsed);
    if (!gst) {
        return;
    }
    QCOMPARE(gst->horizontalAccuracy, 5.0);
    if (qIsNaN(vertical)) {
        QVERIFY(qIsNaN(gst->verticalAccuracy));
    } else {
        QCOMPARE(gst->verticalAccuracy, vertical);
    }
}

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
}

void NMEASentenceTest::_satelliteIds_data()
{
    QTest::addColumn<int>("constellation");
    QTest::addColumn<int>("wireId");
    QTest::addColumn<int>("expected");

    const struct
    {
        GPSConstellation constellation;
        int wireId;
        int expected;
    } cases[] = {
        {GPSConstellation::GLONASS, 64, 64},   {GPSConstellation::GLONASS, 65, 1},
        {GPSConstellation::GLONASS, 96, 32},   {GPSConstellation::GLONASS, 97, 97},
        {GPSConstellation::Galileo, 300, 300}, {GPSConstellation::Galileo, 301, 1},
        {GPSConstellation::Galileo, 336, 36},  {GPSConstellation::Galileo, 337, 337},
        {GPSConstellation::BeiDou, 400, 400},  {GPSConstellation::BeiDou, 401, 1},
        {GPSConstellation::BeiDou, 463, 63},   {GPSConstellation::BeiDou, 464, 464},
        {GPSConstellation::BeiDou, 200, 200},  {GPSConstellation::BeiDou, 201, 1},
        {GPSConstellation::BeiDou, 202, 2},    {GPSConstellation::BeiDou, 235, 35},
        {GPSConstellation::BeiDou, 236, 236},  {GPSConstellation::QZSS, 192, 192},
        {GPSConstellation::QZSS, 193, 1},      {GPSConstellation::QZSS, 201, 9},
        {GPSConstellation::QZSS, 202, 10},     {GPSConstellation::QZSS, 203, 203},
        {GPSConstellation::SBAS, 32, 32},      {GPSConstellation::SBAS, 33, 120},
        {GPSConstellation::SBAS, 64, 151},     {GPSConstellation::SBAS, 65, 65},
        {GPSConstellation::SBAS, 120, 120},    {GPSConstellation::GPS, 33, 33},
        {GPSConstellation::NavIC, 401, 401},   {GPSConstellation::Unknown, 193, 193},
        {GPSConstellation::Unknown, 999, 999},
    };

    for (const auto& entry : cases) {
        const int constellation = static_cast<int>(entry.constellation);
        QTest::addRow("%d-%d", constellation, entry.wireId) << constellation << entry.wireId << entry.expected;
    }
}

void NMEASentenceTest::_satelliteIds()
{
    QFETCH(int, constellation);
    QFETCH(int, wireId);
    QFETCH(int, expected);
    QCOMPARE(NMEA::satelliteId(static_cast<GPSConstellation>(constellation), wireId), expected);
}

void NMEASentenceTest::_coordinateNumbers_data()
{
    QTest::addColumn<double>("degreesMinutes");
    QTest::addColumn<double>("expected");
    const double nan = (std::numeric_limits<double>::quiet_NaN)();
    const double infinity = (std::numeric_limits<double>::infinity)();

    const struct
    {
        const char* name;
        double degreesMinutes;
        double expected;
    } cases[] = {
        {"zero", 0.0, 0.0},
        {"negative-zero", -0.0, 0.0},
        {"half-degree", 30.0, 0.5},
        {"negative-half-degree", -30.0, -0.5},
        {"fraction", 4807.038, 48.1173},
        {"negative-fraction", -4807.038, -48.1173},
        {"positive-limit", 18000.0, 180.0},
        {"negative-limit", -18000.0, -180.0},
        {"above-limit", 18000.001, nan},
        {"below-limit", -18000.001, nan},
        {"invalid-minutes", 1260.0, nan},
        {"negative-invalid-minutes", -1260.0, nan},
        {"nan", nan, nan},
        {"infinity", infinity, nan},
        {"negative-infinity", -infinity, nan},
    };

    for (const auto& entry : cases) {
        QTest::newRow(entry.name) << entry.degreesMinutes << entry.expected;
    }
}

void NMEASentenceTest::_coordinateNumbers()
{
    QFETCH(double, degreesMinutes);
    QFETCH(double, expected);
    const double actual = NMEA::degreesFromDegreesMinutes(degreesMinutes);
    if (std::isnan(expected)) {
        QVERIFY(std::isnan(actual));
    } else {
        QVERIFY(std::isfinite(actual));
        QVERIFY(std::abs(actual - expected) <= 1e-10);
    }
}

void NMEASentenceTest::_numberFields_data()
{
    QTest::addColumn<QByteArray>("field");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<double>("value");
    QTest::addColumn<bool>("floatAccepted");
    QTest::newRow("decimal") << QByteArray("4807.038") << true << 4807.038 << true;
    QTest::newRow("leading-point") << QByteArray(".5") << true << 0.5 << true;
    QTest::newRow("plus-sign") << QByteArray("+1.5") << true << 1.5 << true;
    QTest::newRow("minus-sign") << QByteArray("-1.5") << true << -1.5 << true;
    QTest::newRow("exponent") << QByteArray("1e5") << true << 1e5 << true;
    QTest::newRow("double-sign") << QByteArray("+-1") << false << 0.0 << false;
    QTest::newRow("sign-only") << QByteArray("+") << false << 0.0 << false;
    QTest::newRow("incomplete-exponent") << QByteArray("1e") << false << 0.0 << false;
    QTest::newRow("hexadecimal") << QByteArray("0x10") << false << 0.0 << false;
    QTest::newRow("comma") << QByteArray("1,5") << false << 0.0 << false;
    QTest::newRow("infinity") << QByteArray("inf") << false << 0.0 << false;
    QTest::newRow("not-a-number") << QByteArray("nan") << false << 0.0 << false;
    QTest::newRow("overflow") << QByteArray("1e400") << false << 0.0 << false;
    QTest::newRow("underflow") << QByteArray("1e-400") << false << 0.0 << false;
    QTest::newRow("float-overflow") << QByteArray("1e39") << true << 1e39 << false;
    QTest::newRow("float-underflow") << QByteArray("1e-310") << true << 1e-310 << false;
    // Qt's number parsing skips the surrounding whitespace that std::from_chars rejects.
    QTest::newRow("leading-space") << QByteArray(" 1") << false << 0.0 << false;
    QTest::newRow("trailing-space") << QByteArray("1 ") << false << 0.0 << false;
    QTest::newRow("leading-tab") << QByteArray("\t1") << false << 0.0 << false;
    QTest::newRow("trailing-carriage-return") << QByteArray("1\r") << false << 0.0 << false;
}

void NMEASentenceTest::_numberFields()
{
    QFETCH(QByteArray, field);
    QFETCH(bool, accepted);
    QFETCH(double, value);
    QFETCH(bool, floatAccepted);
    const std::string_view text(field.constData(), static_cast<size_t>(field.size()));
    const auto parsed = NMEA::number<double>(text);
    QCOMPARE(parsed.has_value(), accepted);
    if (accepted) {
        QCOMPARE(*parsed, value);
    }
    const auto parsedFloat = NMEA::number<float>(text);
    QCOMPARE(parsedFloat.has_value(), floatAccepted);
    if (floatAccepted) {
        QCOMPARE(*parsedFloat, static_cast<float>(value));
    }
}

void NMEASentenceTest::_nmeaWireContract()
{
    const NMEA::GGA fix{
        .latitude = 47.3977,
        .longitude = 8.5456,
        .altitude = 100.0,
        .geoidSeparation = 0.0,
        .hdop = 1.0,
        .quality = NMEA::GgaQuality::GPS,
        .satellitesUsed = 12,
    };
    const auto generated = NMEAUtils::makeGGA(fix, QTime(12, 0, 0, 999));
    QCOMPARE(generated, QByteArray("$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,*77\r\n"));
    QVERIFY(checksumValid(generated));
    QCOMPARE(NMEAUtils::repairChecksum(generated), generated);
    const NMEA::GGA unknownFix{.latitude = 47.3977, .longitude = 8.5456};
    const auto unknown = NMEAUtils::makeGGA(unknownFix, QTime(0, 0));
    QVERIFY(unknown.contains(",E,0,,,,M,,M,,*"));
    QVERIFY(checksumValid(unknown));
    QVERIFY(NMEAUtils::makeGGA(fix, QTime()).isEmpty());
    QVERIFY(NMEAUtils::makeGGA({}, QTime(0, 0)).isEmpty());

    const auto sentence = NMEA::sentence("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47");
    QVERIFY(sentence);
    QVERIFY(NMEA::gga(*sentence));
    QCOMPARE(NMEA::utcMilliseconds(sentence->fields[1]), std::optional<int>{45319000});
    QCOMPARE(NMEA::satelliteConstellation("GP", {}, 1), GPSConstellation::GPS);
    const auto view = NMEA::sentence("$GPGSV,1,1,01,01,40,083,41*43");
    QVERIFY(view);
    NMEA::SatelliteAssembler assembler;
    QVERIFY(assembler.ingest(*view, 1000).accepted);
    const auto epoch = assembler.flush();
    const auto gps = std::find_if(epoch.begin(), epoch.end(),
                                  [](const auto& system) { return system.constellation == GPSConstellation::GPS; });
    QVERIFY(gps != epoch.end());
    QCOMPARE(gps->inViewTimestampUs, uint64_t{1000});
    QCOMPARE(gps->inView, 1);
    assembler.clear();
    QVERIFY(assembler.flush().empty());
}

void NMEASentenceTest::_ggaValidity_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<bool>("coordinates");
    QTest::newRow("no-fix-empty") << QByteArray("GPGGA,120000,,,,,0,0,,,,,,,") << true << false;
    QTest::newRow("no-fix-hemispheres") << QByteArray("GPGGA,120000,,N,,E,0,0,,,,,,,") << true << false;
    QTest::newRow("no-fix-zero-coordinates")
        << QByteArray("GPGGA,120000,0000.0,N,00000.0,E,0,0,,,,,,,") << true << true;
    QTest::newRow("fix-without-coordinates") << QByteArray("GPGGA,120000,,,,,1,0,,,,,,,") << false << false;
    QTest::newRow("invalid-coordinate") << QByteArray("GPGGA,120000,9100.0,N,00000.0,E,0,0,,,,,,,") << false << false;
    QTest::newRow("invalid-hemisphere") << QByteArray("GPGGA,120000,,Q,,E,0,0,,,,,,,") << false << false;
}

void NMEASentenceTest::_ggaValidity()
{
    QFETCH(QByteArray, body);
    QFETCH(bool, accepted);
    QFETCH(bool, coordinates);
    const auto wire = NMEAUtils::repairChecksum('$' + body);
    const auto sentence = NMEA::sentence({wire.constData(), static_cast<size_t>(wire.size())});
    QVERIFY(sentence);
    const auto fix = NMEA::gga(*sentence);
    QCOMPARE(fix.has_value(), accepted);
    if (fix) {
        QCOMPARE(fix->quality, NMEA::GgaQuality::INVALID);
        QCOMPARE(fix->satellitesUsed, std::optional<unsigned>(0));
        QCOMPARE(std::isfinite(fix->latitude) && std::isfinite(fix->longitude), coordinates);
    }
}

void NMEASentenceTest::_navigationStatus_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<int>("valid");
    QTest::addColumn<int>("epoch");
    QTest::newRow("gga-no-fix") << QByteArray("GPGGA,120001,,,,,0,0,,,,,,,") << 0 << 43201000;
    QTest::newRow("gga-declared-fix") << QByteArray("GPGGA,120001,,,,,1,0,,,,,,,") << 1 << 43201000;
    QTest::newRow("gga-unknown-quality") << QByteArray("GPGGA,120001,,,,,9,0,,,,,,,") << -1 << -1;
    QTest::newRow("rmc-valid") << QByteArray("GPRMC,120001,A") << 1 << 43201000;
    QTest::newRow("rmc-invalid") << QByteArray("GPRMC,120001,V") << 0 << 43201000;
    QTest::newRow("rmc-invalid-time") << QByteArray("GPRMC,,V") << 0 << -1;
    QTest::newRow("gll-invalid") << QByteArray("GPGLL,,,,,120001,V") << 0 << 43201000;
    QTest::newRow("gll-valid") << QByteArray("GPGLL,,,,,120001,A") << 1 << 43201000;
    QTest::newRow("gsa-invalid") << QByteArray("GPGSA,A,1") << 0 << -1;
    QTest::newRow("gsa-two-dimensional") << QByteArray("GPGSA,A,2") << 1 << -1;
    QTest::newRow("gsa-three-dimensional") << QByteArray("GPGSA,A,3") << 1 << -1;
    QTest::newRow("gsa-unknown") << QByteArray("GPGSA,A,4") << -1 << -1;
    QTest::newRow("rmc-unknown") << QByteArray("GPRMC,120001,X") << -1 << -1;
}

void NMEASentenceTest::_navigationStatus()
{
    QFETCH(QByteArray, body);
    QFETCH(int, valid);
    QFETCH(int, epoch);
    const auto wire = NMEAUtils::repairChecksum('$' + body);
    const auto sentence = NMEA::sentence({wire.constData(), static_cast<size_t>(wire.size())});
    QVERIFY(sentence);
    const auto status = NMEA::navigationStatus(*sentence);
    QCOMPARE(status.has_value(), valid >= 0);
    if (status) {
        QCOMPARE(status->valid, valid != 0);
        QCOMPARE(status->utcMilliseconds.value_or(-1), epoch);
    }
}

/// The synthetic GGA and GST fixtures against expectations decoded independently of this parser.
void NMEASentenceTest::_fixtureSentences()
{
    const auto ggaBytes = GPSTest::fixtureBytes("synthetic-gga.nmea");
    QVERIFY(ggaBytes);
    const std::string_view ggaText(reinterpret_cast<const char*>(ggaBytes->data()), ggaBytes->size());
    for (const auto& expected : GPSFixture::ggas) {
        const auto text = ggaText.substr(expected.offset, expected.size);
        const auto sentence = NMEA::sentence(text);
        QVERIFY(sentence && sentence->talker() == expected.talker);
        const auto actual = NMEA::gga(*sentence);
        if ((std::isnan(expected.latitude) || std::isnan(expected.longitude)) &&
            expected.quality != NMEA::GgaQuality::INVALID) {
            QVERIFY(!actual);
        } else {
            QVERIFY(actual);
            QVERIFY(GPSTest::matches(actual->latitude, expected.latitude, 1e-9));
            QVERIFY(GPSTest::matches(actual->longitude, expected.longitude, 1e-9));
            QVERIFY(GPSTest::matches(actual->altitude, expected.altitude));
            QVERIFY(GPSTest::matches(actual->geoidSeparation, expected.geoid));
            QVERIFY(GPSTest::matches(actual->hdop, expected.hdop));
            QCOMPARE(actual->quality, expected.quality);
            QCOMPARE(actual->satellitesUsed.has_value(), (expected.satellites >= 0));
            if (expected.satellites >= 0) {
                QCOMPARE(*actual->satellitesUsed, static_cast<unsigned>(expected.satellites));
            }
        }
        QVERIFY(!NMEA::sentence(text.substr(0, text.size() - 3)));
        std::string corrupt(text);
        corrupt[corrupt.size() - 3] = corrupt[corrupt.size() - 3] == '0' ? '1' : '0';
        QVERIFY(!NMEA::sentence(corrupt));
    }
    const auto gstBytes = GPSTest::fixtureBytes("synthetic-gst.nmea");
    QVERIFY(gstBytes);
    const std::string_view gstText(reinterpret_cast<const char*>(gstBytes->data()), gstBytes->size());
    for (const auto& expected : GPSFixture::gsts) {
        const auto sentence = NMEA::sentence(gstText.substr(expected.offset, expected.size));
        QVERIFY(sentence && sentence->talker() == expected.talker);
        const auto actual = NMEA::gst(*sentence);
        QVERIFY(actual);
        QVERIFY(GPSTest::matches(actual->horizontalAccuracy, expected.horizontal));
        QVERIFY(GPSTest::matches(actual->verticalAccuracy, expected.vertical));
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(NMEASentenceTest, TestLabel::Unit)
