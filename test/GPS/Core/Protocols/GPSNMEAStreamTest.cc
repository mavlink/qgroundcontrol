#include "GPSNMEAStreamTest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QStringList>

#include "GPSNMEAStream.h"
#include "GPSObservation.h"
#include "GPSProtocolRuntime.h"
#include "NMEASentence.h"
#include "Protocols/Support/GPSEventSummary.h"
#include "Protocols/Support/GPSProtocolTestData.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ProtocolTestPackets.h"

using namespace GPSTest;

namespace {

/// Standard NMEA through the runtime: the composition an ASCII family without vendor sentences uses.
class NMEAStreamFamily final : public GPSFamilyProtocol
{
public:
    bool configure(GPSCommandChannel&, GPSConfig, unsigned&) override { return true; }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        return _nmea.onFrame(frame, context);
    }

    void flush(GPSDecodeContext& context) override { _nmea.flush(context); }

private:
    GPSNMEAStream _nmea;
};

constexpr GPSReceiverFamily NMEA_FAMILY{.type = GPSType::passive, .stream = GPSNMEAStream::STREAM};

QByteArray rtcm(uint16_t message, int size)
{
    std::vector<uint8_t> payload(static_cast<size_t>(size), 0x5a);
    payload[0] = static_cast<uint8_t>(message >> 4);
    payload[1] = static_cast<uint8_t>(message << 4);
    return toByteArray(rtcmPacket(payload));
}

/// Dense multi-constellation epochs with interleaved RTCM3.
QByteArray syntheticEpochs()
{
    QByteArray stream;
    for (int second = 0; second < 6; ++second) {
        const std::string utc = "12000" + std::to_string(second) + ".00";
        stream += nmeaBytes("GPGGA," + utc + ",4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
        stream += rtcm(1005, 19);
        stream += nmeaBytes("GNGSA,A,3,01,02,03,04,,,,,,,,,1.8,0.9,1.5,1");
        stream += nmeaBytes("GNGSA,A,3,65,66,,,,,,,,,,,1.8,0.9,1.5,2");
        stream += nmeaBytes("GPGSV,2,1,05,01,40,083,46,02,17,308,41,03,07,344,39,04,22,228,45");
        stream += rtcm(1077, 60) + rtcm(1087, 40);
        stream += nmeaBytes("GPGSV,2,2,05,05,11,111,40");
        stream += nmeaBytes("GLGSV,1,1,02,65,22,100,40,66,33,200,41");
        stream += nmeaBytes("GAGSV,1,1,01,11,22,100,40");
        stream += nmeaBytes("GBGSV,1,1,01,21,22,100,40");
        stream += nmeaBytes("GQGSV,1,1,01,193,22,100,40");
        stream += rtcm(1097, 30) + rtcm(1127, 30) + rtcm(1230, 8);
        stream += nmeaBytes("GPGST," + utc + ",0.5,0.3,0.2,45.0,0.3,0.4,0.6");
        stream += nmeaBytes("GPZDA," + utc + ",12,07,2026,00,00");
        stream += "#UNKNOWN,1,2;3*00\r\n";
        stream += QByteArray("\x01\x02garbage\r\n", 11);
        stream += nmeaBytes("GPGGA," + utc + ",4807.038,N,01131.000,E,0,00,,,M,,M,,");
    }
    return stream;
}

/// The events of @a bytes decoded in @a chunk sized pieces, one summary line per event; batch boundaries are dropped.
QStringList decodeEvents(const QByteArray& bytes, qsizetype chunk)
{
    QStringList events;
    GPSRuntimeObserver observer;
    observer.decoded = [&events](const GPSEventBatch& batch) { events += summary(batch).mid(1); };
    GPSProtocolRuntime runtime(NMEA_FAMILY, std::make_unique<NMEAStreamFamily>(),
                               {.clock = {.nowUs = [] { return GPSTestClock::START_US; }}}, std::move(observer));
    const auto* data = reinterpret_cast<const uint8_t*>(bytes.constData());
    for (qsizetype offset = 0; offset < bytes.size(); offset += chunk) {
        (void) runtime.consume({data + offset, static_cast<size_t>(std::min(chunk, bytes.size() - offset))});
    }
    (void) runtime.consume({});
    return events;
}

/// Standard NMEA sentences on a controllable clock, keeping the latest published position.
struct NMEAReceiver
{
    NMEAReceiver()
        : runtime(NMEA_FAMILY, std::make_unique<NMEAStreamFamily>(), {.clock = {.nowUs = [this] { return now; }}},
                  {.decoded = [this](const GPSEventBatch& batch) {
                      for (const auto& event : batch.events) {
                          if (const auto* report = std::get_if<GPSDecodedPosition>(&event)) {
                              published = *report;
                          }
                      }
                  }})
    {}

    void feed(const QByteArray& body)
    {
        send(QByteArray::fromStdString(nmeaSentence({body.constData(), static_cast<size_t>(body.size())})));
    }

    void send(const QByteArray& bytes)
    {
        (void) runtime.consume(
            {reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size())});
    }

    uint64_t now = 1000000;
    std::optional<GPSDecodedPosition> published;
    GPSProtocolRuntime runtime;
};

QByteArray gga(const QByteArray& utc)
{
    return "GPGGA," + utc + ",4807.038,N,01131.000,E,1,08,0.9,100.0,M,10.0,M,,";
}

constexpr auto GSA = "GPGSA,A,3,01,,,,,,,,,,,,1.0,0.8,0.6";

constexpr qsizetype PRODUCTIVE_INPUTS = 5;

}  // namespace

void GPSNMEAStreamTest::_chunkingIndependence_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<int>("chunk");

    QList<std::pair<QString, QByteArray>> inputs;
    const QDir corpus(QString::fromUtf8(GPSTest::CORPUS_DIR));
    for (const auto& name : corpus.entryList({QStringLiteral("*.nmea"), QStringLiteral("*.ascii")}, QDir::Files)) {
        inputs.append({name, readFile(corpus.filePath(name)).value_or(QByteArray())});
    }
    const QDir fixtures(QString::fromUtf8(GPSTest::FIXTURE_DIR));
    for (const auto& name : {QStringLiteral("gga.nmea"), QStringLiteral("synthetic-gga.nmea"),
                             QStringLiteral("synthetic-gst.nmea"), QStringLiteral("mixed.gps")}) {
        inputs.append({QStringLiteral("fixture-") + name, readFile(fixtures.filePath(name)).value_or(QByteArray())});
    }
    inputs.append({QStringLiteral("synthetic-epochs"), syntheticEpochs()});
    QVERIFY(inputs.size() > 12);
    // Inputs without a standard navigation epoch decode to nothing; the navigation inputs must produce events.
    const auto productive = std::count_if(inputs.cbegin(), inputs.cend(), [](const auto& input) {
        return !decodeEvents(input.second, input.second.size()).isEmpty();
    });
    QVERIFY2(productive >= PRODUCTIVE_INPUTS, qPrintable(QString::number(productive)));
    for (const auto& [name, bytes] : inputs) {
        QVERIFY2(!bytes.isEmpty(), qPrintable(name));
        for (const int chunk : {1, 7, 150}) {
            QTest::addRow("%s-%d", qPrintable(name), chunk) << bytes << chunk;
        }
    }
}

void GPSNMEAStreamTest::_chunkingIndependence()
{
    QFETCH(QByteArray, bytes);
    QFETCH(int, chunk);
    QCOMPARE(decodeEvents(bytes, chunk), decodeEvents(bytes, bytes.size()));
}

void GPSNMEAStreamTest::_limitsReceiveToSatelliteDeadline()
{
    GPSTestClock clock(GPSTestClock::START_US);

    class Probe final : public GPSFamilyProtocol
    {
    public:
        bool configure(GPSCommandChannel&, GPSConfig, unsigned&) override { return true; }

        GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
        {
            return nmea.onFrame(frame, context);
        }

        GPSNMEAStream nmea;
    };

    auto probe = std::make_unique<Probe>();
    auto& nmea = probe->nmea;
    GPSProtocolRuntime runtime(NMEA_FAMILY, std::move(probe), {.clock = {.nowUs = [&clock] { return clock.nowUs(); }}});
    QCOMPARE(nmea.limitReceiveTimeout(std::chrono::milliseconds(5000), clock.nowUs()), std::chrono::milliseconds(5000));
    const QByteArray partial = nmeaBytes("GPGSV,2,1,05,01,40,083,46,02,17,308,41,03,07,344,39,04,22,228,45");
    (void) runtime.consume(
        {reinterpret_cast<const uint8_t*>(partial.constData()), static_cast<size_t>(partial.size())});
    // An incomplete multipart view is published once idle, so a receive may not block past that.
    const auto limited = nmea.limitReceiveTimeout(std::chrono::milliseconds(5000), clock.nowUs());
    QVERIFY(limited > std::chrono::milliseconds::zero());
    QVERIFY(limited <= NMEA::SatelliteAssembler::BATCH_TIMEOUT);
}

void GPSNMEAStreamTest::_vdopEpoch_data()
{
    QTest::addColumn<QByteArray>("firstUtc");
    QTest::addColumn<QByteArray>("nextUtc");
    QTest::addColumn<quint64>("elapsedUs");
    QTest::addColumn<bool>("retained");
    QTest::newRow("same-epoch") << QByteArray("123519") << QByteArray("123519") << quint64(1000) << true;
    QTest::newRow("next-epoch") << QByteArray("123519") << QByteArray("123520") << quint64(1000000) << false;
    QTest::newRow("midnight") << QByteArray("235959") << QByteArray("000000") << quint64(1000000) << false;
    QTest::newRow("expired-same-epoch") << QByteArray("123519") << QByteArray("123519") << quint64(7000000) << false;
    QTest::newRow("expired-next-epoch") << QByteArray("123519") << QByteArray("123526") << quint64(7000000) << false;
    QTest::newRow("missing-epoch") << QByteArray("123519") << QByteArray() << quint64(1000) << false;
}

void GPSNMEAStreamTest::_vdopEpoch()
{
    QFETCH(QByteArray, firstUtc);
    QFETCH(QByteArray, nextUtc);
    QFETCH(quint64, elapsedUs);
    QFETCH(bool, retained);
    NMEAReceiver receiver;
    const auto gst = "GPGST," + firstUtc + ",0,0,0,0,0.3,0.4,0.6";
    receiver.feed(gst);
    QVERIFY(!receiver.published);
    receiver.feed(gga(firstUtc));
    QVERIFY(receiver.published);
    QCOMPARE(receiver.published->navigation.horizontalAccuracyMeters, 0.5f);
    const auto positionReceipt = receiver.published->navigation.timestampUs;
    receiver.feed(GSA);
    ++receiver.now;
    receiver.feed(gst);
    QCOMPARE(receiver.published->navigation.timestampUs, positionReceipt);
    QCOMPARE(receiver.published->navigation.verticalDop, 0.6f);
    receiver.now = positionReceipt + elapsedUs;
    receiver.feed(gga(nextUtc));
    QCOMPARE(receiver.published->navigation.timestampUs, receiver.now);
    QCOMPARE(receiver.published->navigation.horizontalDop, 0.9f);
    if (retained) {
        QCOMPARE(receiver.published->navigation.verticalDop, 0.6f);
    } else {
        QVERIFY(std::isnan(receiver.published->navigation.verticalDop));
        QVERIFY(std::isnan(receiver.published->navigation.horizontalAccuracyMeters));
    }
}

void GPSNMEAStreamTest::_vdopReceiptIsNotRenewed()
{
    NMEAReceiver receiver;
    receiver.feed(gga("123519"));
    receiver.feed(GSA);
    for (unsigned second = 1; second <= 7; ++second) {
        receiver.now += 1000000;
        receiver.feed(gga("123519"));
        QCOMPARE(receiver.published->navigation.timestampUs, receiver.now);
        if (second <= 2) {
            QCOMPARE(receiver.published->navigation.verticalDop, 0.6f);
        } else {
            QVERIFY(std::isnan(receiver.published->navigation.verticalDop));
        }
    }
}

void GPSNMEAStreamTest::_unassociatedGsa_data()
{
    QTest::addColumn<qint64>("ageUs");
    QTest::newRow("before-position") << qint64(0);
    QTest::newRow("expired-position") << qint64(2000001);
    QTest::newRow("clock-regression") << qint64(-1);
}

void GPSNMEAStreamTest::_unassociatedGsa()
{
    QFETCH(qint64, ageUs);
    NMEAReceiver receiver;
    if (ageUs) {
        receiver.feed(gga("123519"));
        receiver.now = static_cast<uint64_t>(static_cast<qint64>(receiver.now) + ageUs);
    }
    receiver.feed(GSA);
    QVERIFY(!receiver.published || std::isnan(receiver.published->navigation.verticalDop));
    receiver.now = 4000000;
    receiver.feed(gga("123520"));
    QVERIFY(std::isnan(receiver.published->navigation.verticalDop));
}

void GPSNMEAStreamTest::_boundedFields_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QStringList>("expected");
    QTest::newRow("empty") << QByteArray() << QStringList{QString()};
    QTest::newRow("leading-and-trailing") << QByteArray(",a,") << QStringList{"", "a", ""};
    QTest::newRow("capacity") << QByteArray("a,,b,") << QStringList{"a", "", "b", ""};
    QTest::newRow("overflow") << QByteArray("a,b,c,d,e") << QStringList{};
    QTest::newRow("trailing-overflow") << QByteArray("a,b,c,d,") << QStringList{};
}

void GPSNMEAStreamTest::_boundedFields()
{
    QFETCH(QByteArray, body);
    QFETCH(QStringList, expected);
    std::array<std::string_view, 4> fields;
    const auto count = NMEA::splitFields({body.constData(), static_cast<size_t>(body.size())}, fields);
    QCOMPARE(count, static_cast<size_t>(expected.size()));
    for (size_t index = 0; index < count; ++index) {
        QCOMPARE(QString::fromLatin1(fields[index].data(), fields[index].size()), expected[index]);
    }
    QCOMPARE(NMEA::splitFields("a", {}), size_t(0));
}

void GPSNMEAStreamTest::_positionSourceEquivalence()
{
    const QList<QByteArray> bodies{
        "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,2.0,31.66,280511,,,A",
        "$GPGSA,A,3,02,,,,,,,,,,,,1.0,1.03,0.6",
        "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,",
        "$GPGST,092750.000,1,1,1,0,3,4,6",
        "$GPVTG,31.66,T,,M,1.08,N,2.0,K",
    };
    NMEAReceiver receiver;
    for (const auto& body : bodies) {
        receiver.send(NMEAUtils::repairChecksum(body));
    }
    QVERIFY(receiver.published);
    const auto& navigation = receiver.published->navigation;

    // Passive receivers are the only NMEA position input, so their epoch carries every navigation field.
    const auto observation = GPSObservation::fromNavigation(navigation, receiver.now);
    QCOMPARE(navigation.fixType, GPSPositionReport::FixType::Fix3D);
    QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::Fix3D);
    QVERIFY(qAbs(navigation.latitudeDegrees - 53.36133667) < 1e-6);
    QVERIFY(qAbs(navigation.longitudeDegrees + 6.50562) < 1e-6);
    QVERIFY(qAbs(navigation.altitudeMslMeters - 61.7) < 1e-9);
    QCOMPARE(observation.altitudeDatum, GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(navigation.satellitesUsed, std::optional<uint8_t>(8));
    QCOMPARE(observation.satellitesUsed, std::optional<int>(8));
    QCOMPARE(navigation.horizontalDop, 1.03f);
    QCOMPARE(navigation.verticalDop, 0.6f);
    QCOMPARE(navigation.horizontalAccuracyMeters, 5.0f);
    QCOMPARE(navigation.verticalAccuracyMeters, 6.0f);
    QCOMPARE(observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy), 5.0);
    QCOMPARE(observation.position.attribute(QGeoPositionInfo::VerticalAccuracy), 6.0);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSNMEAStreamTest, TestLabel::Unit)
