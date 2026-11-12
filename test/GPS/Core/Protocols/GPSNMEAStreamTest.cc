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
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QtMath>

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

constexpr GPSReceiverFamily NMEA_FAMILY{.type = GPSType::Passive, .stream = GPSNMEAStream::STREAM};

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
                              ++positions;
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
    int positions = 0;
    GPSProtocolRuntime runtime;
};

QByteArray gga(const QByteArray& utc)
{
    return "GPGGA," + utc + ",4807.038,N,01131.000,E,1,08,0.9,100.0,M,10.0,M,,";
}

constexpr auto GSA = "GPGSA,A,3,01,,,,,,,,,,,,1.0,0.8,0.6";
constexpr auto VTG = "GPVTG,90.0,T,,M,2.0,N,3.7,K";

QByteArray rmc(const QByteArray& utc)
{
    return "GPRMC," + utc + ",A,4807.038,N,01131.000,E,2.0,90.0,230394,,,A";
}

QByteArray gst(const QByteArray& utc)
{
    return "GPGST," + utc + ",0,0,0,0,0.3,0.4,0.6";
}

/// The sentence of @a type for the epoch @a utc.
QByteArray sentenceOf(const QByteArray& type, const QByteArray& utc)
{
    if (type == "GGA") {
        return gga(utc);
    }
    if (type == "RMC") {
        return rmc(utc);
    }
    if (type == "GST") {
        return gst(utc);
    }
    if (type == "GLL") {
        return "GPGLL,4807.038,N,01131.000,E," + utc + ",A";
    }
    if (type == "GSV") {
        return "GPGSV,1,1,01,01,40,083,41";
    }
    return type == "GSA" ? GSA : VTG;
}

/// The sentence of @a type for the epoch @a utc, with @a report as its course, VDOP or accuracy.
QByteArray sentenceOfReport(const QByteArray& type, const QByteArray& utc, int report)
{
    const QByteArray value = QByteArray::number(report) + ".0";
    if (type == "RMC") {
        return "GPRMC," + utc + ",A,4807.038,N,01131.000,E,2.0," + value + ",230394,,,A";
    }
    if (type == "VTG") {
        return "GPVTG," + value + ",T,,M,2.0,N,3.7,K";
    }
    if (type == "GSA") {
        return "GPGSA,A,3,01,,,,,,,,,,,,1.0,0.8," + value;
    }
    if (type == "GST") {
        return "GPGST," + utc + ",0,0,0,0," + value + ",0," + value;
    }
    return sentenceOf(type, utc);
}

constexpr uint64_t EPOCH_INTERVAL_US = 1000000;
constexpr uint64_t SENTENCE_INTERVAL_US = 10000;

/// The time the sentence of @a body takes on a serial link of @a baud, at ten bits a byte.
uint64_t wireTimeUs(const QByteArray& body, unsigned baud)
{
    return static_cast<uint64_t>(nmeaBytes(body).size()) * 10 * 1000000 / baud;
}

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

void GPSNMEAStreamTest::_ggaQuality_data()
{
    QTest::addColumn<QByteArray>("quality");
    QTest::addColumn<bool>("published");
    QTest::addColumn<GPSFixQuality>("fixType");
    QTest::newRow("precise-positioning-service") << QByteArray("3") << true << GPSFixQuality::Fix3D;
    QTest::newRow("simulation") << QByteArray("8") << true << GPSFixQuality::Fix3D;
    QTest::newRow("novatel-sbas") << QByteArray("9") << true << GPSFixQuality::Differential;
    QTest::newRow("undefined") << QByteArray("10") << false << GPSFixQuality::Unknown;
}

void GPSNMEAStreamTest::_ggaQuality()
{
    QFETCH(QByteArray, quality);
    QFETCH(bool, published);
    QFETCH(GPSFixQuality, fixType);
    NMEAReceiver receiver;
    receiver.feed("GPGGA,123519,4807.038,N,01131.000,E," + quality + ",08,0.9,100.0,M,10.0,M,,");
    QCOMPARE(receiver.published.has_value(), published);
    if (published) {
        QCOMPARE(receiver.published->navigation.fixType, fixType);
        QVERIFY(GPSObservation::fromNavigation(receiver.published->navigation, receiver.now).hasNavigationSolution());
    }
}

void GPSNMEAStreamTest::_sentenceOrder_data()
{
    QTest::addColumn<QByteArrayList>("order");
    QTest::addColumn<QByteArray>("publishedBy");
    QTest::addColumn<bool>("velocity");
    QTest::addColumn<bool>("verticalDop");
    QTest::addColumn<bool>("accuracy");
    QTest::newRow("gga-first") << QByteArrayList{"GGA", "GSA", "GSV", "RMC", "VTG"} << QByteArray("VTG") << true << true
                               << false;
    QTest::newRow("gga-first-without-rmc")
        << QByteArrayList{"GGA", "GST", "GSA", "VTG"} << QByteArray("VTG") << true << true << true;
    // The GLL adds nothing to the GGA, so the epoch does not wait for it.
    QTest::newRow("gga-between") << QByteArrayList{"RMC", "VTG", "GGA", "GSA", "GSV", "GLL"} << QByteArray("GSA")
                                 << true << true << false;
    QTest::newRow("gga-last") << QByteArrayList{"RMC", "VTG", "GSA", "GST", "GGA"} << QByteArray("GGA") << true << true
                              << true;
    // The GST begins its epoch before the GGA makes it the current one.
    QTest::newRow("gst-first") << QByteArrayList{"GST", "GGA", "GSA"} << QByteArray("GSA") << false << true << true;
    QTest::newRow("gga-only") << QByteArrayList{"GGA"} << QByteArray("GGA") << false << false << false;
}

void GPSNMEAStreamTest::_sentenceOrder()
{
    QFETCH(QByteArrayList, order);
    QFETCH(QByteArray, publishedBy);
    QFETCH(bool, velocity);
    QFETCH(bool, verticalDop);
    QFETCH(bool, accuracy);
    NMEAReceiver receiver;
    uint64_t positionReceipt = 0;
    QByteArray publisher;
    // What follows the GGA is waited for once two epochs have shown that the receiver sends it.
    for (const QByteArray& utc : {QByteArray("123519"), QByteArray("123520"), QByteArray("123521")}) {
        receiver.now += EPOCH_INTERVAL_US;
        for (const auto& type : order) {
            receiver.now += SENTENCE_INTERVAL_US;
            if (type == "GGA") {
                positionReceipt = receiver.now;
            }
            const int published = receiver.positions;
            receiver.feed(sentenceOf(type, utc));
            if (receiver.positions != published) {
                publisher = type;
            }
        }
    }
    // Every epoch is published once, and the third by the last sentence the receiver adds to its GGA.
    QCOMPARE(receiver.positions, 3);
    QCOMPARE(publisher, publishedBy);
    const auto& navigation = receiver.published->navigation;
    QCOMPARE(navigation.timestampUs, positionReceipt);
    QCOMPARE(receiver.published->velocityValid, velocity);
    QCOMPARE(std::isfinite(navigation.speedMetersPerSecond), velocity);
    QCOMPARE(std::isfinite(navigation.courseRadians), velocity);
    QCOMPARE(std::isfinite(navigation.verticalDop), verticalDop);
    QCOMPARE(std::isfinite(navigation.horizontalAccuracyMeters), accuracy);
}

void GPSNMEAStreamTest::_sentenceNotWaitedFor_data()
{
    QTest::addColumn<QByteArray>("type");
    QTest::addColumn<int>("everyEpochs");
    QTest::addColumn<quint64>("delayUs");
    // An RMC at half the rate of the GGA follows every other epoch only.
    QTest::newRow("slower-sentence") << QByteArray("RMC") << 2 << quint64(SENTENCE_INTERVAL_US);
    // A VTG carries no time: in the second half of the interval it may open the next epoch's burst.
    QTest::newRow("untimed-late-in-the-interval") << QByteArray("VTG") << 1 << quint64(EPOCH_INTERVAL_US * 6 / 10);
}

void GPSNMEAStreamTest::_sentenceNotWaitedFor()
{
    QFETCH(QByteArray, type);
    QFETCH(int, everyEpochs);
    QFETCH(quint64, delayUs);
    NMEAReceiver receiver;
    const uint64_t start = receiver.now;
    for (int epoch = 0; epoch < 6; ++epoch) {
        const QByteArray utc = QByteArray::number(123519 + epoch);
        receiver.now = start + epoch * EPOCH_INTERVAL_US;
        receiver.feed(gga(utc));
        QCOMPARE(receiver.positions, epoch + 1);
        QCOMPARE(receiver.published->navigation.timestampUs, receiver.now);
        // The sentence that came after the epoch before was published is not published with this one either.
        QVERIFY(!receiver.published->velocityValid);
        if (epoch % everyEpochs == 0) {
            receiver.now += delayUs;
            receiver.feed(sentenceOf(type, utc));
        }
    }
    QCOMPARE(receiver.positions, 6);
}

void GPSNMEAStreamTest::_timedSentenceLateInInterval_data()
{
    QTest::addColumn<QByteArray>("type");
    QTest::addColumn<int>("satellitesEvery");
    // SiRF and MediaTek receivers send GGA, GSA, GSV and RMC in this order, the GSV in every epoch or in every fifth.
    QTest::newRow("rmc-satellites-every-epoch") << QByteArray("RMC") << 1;
    QTest::newRow("rmc-satellites-every-fifth-epoch") << QByteArray("RMC") << 5;
    QTest::newRow("gst-satellites-every-epoch") << QByteArray("GST") << 1;
    QTest::newRow("gst-satellites-every-fifth-epoch") << QByteArray("GST") << 5;
}

void GPSNMEAStreamTest::_timedSentenceLateInInterval()
{
    QFETCH(QByteArray, type);
    QFETCH(int, satellitesEvery);
    // One epoch a second at 4800 baud, the rate of NMEA 0183. The GSV sentences of twelve satellites take 0.44 s,
    // which puts the sentence after them more than half the interval after the GGA.
    constexpr unsigned BAUD = 4800;
    const QByteArrayList satellites{
        "GPGSV,3,1,12,01,40,083,46,02,17,308,41,03,07,344,39,04,22,228,45",
        "GPGSV,3,2,12,05,11,111,40,06,62,150,44,07,35,060,42,08,19,190,38",
        "GPGSV,3,3,12,09,51,270,43,10,08,020,33,11,27,130,41,12,74,300,47",
    };
    NMEAReceiver receiver;
    const uint64_t start = receiver.now;
    const auto feed = [&receiver](const QByteArray& body) {
        receiver.now += wireTimeUs(body, BAUD);
        receiver.feed(body);
    };
    for (int epoch = 0; epoch < 12; ++epoch) {
        const QByteArray utc = QByteArray::number(123519 + epoch);
        const bool withSatellites = epoch % satellitesEvery == 0;
        receiver.now = start + epoch * EPOCH_INTERVAL_US;
        feed(gga(utc));
        const uint64_t positionReceipt = receiver.now;
        feed(GSA);
        if (withSatellites) {
            for (const auto& page : satellites) {
                feed(page);
            }
        }
        const int publishedBefore = receiver.positions;
        feed(sentenceOf(type, utc));
        QCOMPARE(receiver.now - positionReceipt > EPOCH_INTERVAL_US / 2, withSatellites);
        QCOMPARE(receiver.positions, epoch + 1);
        const auto& navigation = receiver.published->navigation;
        QCOMPARE(navigation.timestampUs, positionReceipt);
        if (epoch >= 2) {
            // The sentence carries its epoch's time, so two epochs make it the receiver's order however late it
            // came: this epoch waited for it, as does the one after an epoch whose satellites delayed it.
            QCOMPARE(publishedBefore, epoch);
            QVERIFY(type == "GST" ? std::isfinite(navigation.horizontalAccuracyMeters)
                                  : receiver.published->velocityValid);
        }
    }
}

void GPSNMEAStreamTest::_missingSentence_data()
{
    QTest::addColumn<QByteArrayList>("order");
    QTest::addColumn<QByteArray>("missing");
    QTest::addColumn<bool>("overduePublished");
    // The next epoch waits for its GSA, so its GGA publishes the epoch before it as it stands.
    QTest::newRow("next-epoch-waits") << QByteArrayList{"GGA", "GSA", "RMC"} << QByteArray("RMC") << true;
    // The next epoch's RMC ends the epoch before it.
    QTest::newRow("next-epoch-begins") << QByteArrayList{"RMC", "GGA", "GSA"} << QByteArray("GSA") << true;
    // The next epoch is published by its GGA and replaces the epoch before it.
    QTest::newRow("next-epoch-complete") << QByteArrayList{"GGA", "RMC"} << QByteArray("RMC") << false;
}

void GPSNMEAStreamTest::_missingSentence()
{
    QFETCH(QByteArrayList, order);
    QFETCH(QByteArray, missing);
    QFETCH(bool, overduePublished);
    NMEAReceiver receiver;
    const auto feedEpoch = [&receiver](const QByteArrayList& types, const QByteArray& utc) {
        uint64_t positionReceipt = 0;
        for (const auto& type : types) {
            receiver.now += SENTENCE_INTERVAL_US;
            if (type == "GGA") {
                positionReceipt = receiver.now;
            }
            receiver.feed(sentenceOf(type, utc));
        }
        receiver.now += EPOCH_INTERVAL_US;
        return positionReceipt;
    };
    feedEpoch(order, "123519");
    feedEpoch(order, "123520");
    QCOMPARE(receiver.positions, 2);

    QByteArrayList incomplete = order;
    incomplete.removeOne(missing);
    const uint64_t waiting = feedEpoch(incomplete, "123521");
    QCOMPARE(receiver.positions, 2);

    // The first sentence of the next epoch ends the one that waits.
    const uint64_t first = receiver.now;
    receiver.feed(sentenceOf(order.first(), "123522"));
    QCOMPARE(receiver.positions, 3);
    if (overduePublished) {
        QCOMPARE(receiver.published->navigation.timestampUs, waiting);
    }
    const uint64_t rest = feedEpoch(order.mid(1), "123522");
    QCOMPARE(receiver.positions, overduePublished ? 4 : 3);
    QCOMPARE(receiver.published->navigation.timestampUs, order.first() == "GGA" ? first : rest);
}

void GPSNMEAStreamTest::_untimedSentenceBeforeItsPosition()
{
    // GSA carries no time. A receiver that sends it before the epoch's timed sentences delivers it late in the interval
    // of the epoch before it, and waiting for it there would hold every position for most of an epoch.
    NMEAReceiver receiver;
    for (int epoch = 0; epoch < 5; ++epoch) {
        const QByteArray utc = QByteArray::number(123519 + epoch);
        receiver.now += EPOCH_INTERVAL_US - 2 * SENTENCE_INTERVAL_US;
        receiver.feed(GSA);
        receiver.now += SENTENCE_INTERVAL_US;
        receiver.feed(rmc(utc));
        receiver.now += SENTENCE_INTERVAL_US;
        receiver.feed(gga(utc));
        QCOMPARE(receiver.positions, epoch + 1);
        QCOMPARE(receiver.published->navigation.timestampUs, receiver.now);
    }
}

void GPSNMEAStreamTest::_emptyFieldKeepsEpochValue_data()
{
    QTest::addColumn<QByteArrayList>("sentences");
    const QByteArray utc("123519");
    // The epoch has its speed, course, VDOP and accuracy; a later sentence before the GGA leaves those fields empty.
    QTest::newRow("vtg-after-rmc") << QByteArrayList{rmc(utc), GSA, "GPVTG,,T,,M,,N,,K"};
    QTest::newRow("rmc-after-vtg") << QByteArrayList{sentenceOf("GLL", utc), VTG, GSA,
                                                     "GPRMC," + utc + ",A,4807.038,N,01131.000,E,,,230394,,,A"};
    QTest::newRow("gsa-after-gsa") << QByteArrayList{rmc(utc), GSA, QByteArray("GPGSA,A,3,02") + QByteArray(14, ',')};
    // The test's GST precedes the sentence that begins the epoch; the empty one comes before it too, or in the epoch.
    const QByteArray emptyGst = "GPGST," + utc + QByteArray(7, ',');
    QTest::newRow("gst-after-gst") << QByteArrayList{emptyGst, rmc(utc), GSA};
    QTest::newRow("gst-in-epoch-after-gst") << QByteArrayList{rmc(utc), GSA, emptyGst};
}

void GPSNMEAStreamTest::_emptyFieldKeepsEpochValue()
{
    QFETCH(QByteArrayList, sentences);
    NMEAReceiver receiver;
    receiver.feed(gst("123519"));
    for (const auto& sentence : sentences) {
        receiver.feed(sentence);
    }
    QVERIFY(!receiver.published);
    receiver.feed(gga("123519"));
    QVERIFY(receiver.published);
    const auto& navigation = receiver.published->navigation;
    QVERIFY(receiver.published->velocityValid);
    // Two knots in the RMC, 3.7 km/h in the VTG.
    QVERIFY(qAbs(navigation.speedMetersPerSecond - 1.03f) < 0.01f);
    QCOMPARE(navigation.courseRadians, qDegreesToRadians(90.0f));
    QCOMPARE(navigation.verticalDop, 0.6f);
    QCOMPARE(navigation.horizontalAccuracyMeters, 0.5f);
    QCOMPARE(navigation.verticalAccuracyMeters, 0.6f);
}

void GPSNMEAStreamTest::_lateSentenceCompletesWaitingEpoch()
{
    NMEAReceiver receiver;
    for (const QByteArray& utc : {QByteArray("123519"), QByteArray("123520")}) {
        receiver.now += EPOCH_INTERVAL_US;
        receiver.feed(gga(utc));
        receiver.feed(VTG);
    }
    receiver.now += EPOCH_INTERVAL_US;
    const uint64_t positionReceipt = receiver.now;
    receiver.feed(gga("123521"));
    QCOMPARE(receiver.positions, 2);
    // Too late to be waited for in the epochs to come, but this epoch waits and no other has begun.
    receiver.now += EPOCH_INTERVAL_US * 6 / 10;
    receiver.feed(VTG);
    QCOMPARE(receiver.positions, 3);
    QCOMPARE(receiver.published->navigation.timestampUs, positionReceipt);
    QVERIFY(receiver.published->velocityValid);
}

void GPSNMEAStreamTest::_awaitedSentenceAfterTwoSeconds_data()
{
    QTest::addColumn<QByteArray>("type");
    QTest::newRow("rmc") << QByteArray("RMC");
    QTest::newRow("gst") << QByteArray("GST");
    // Without a time of their own, they join the epoch that waits for them.
    QTest::newRow("gsa") << QByteArray("GSA");
    QTest::newRow("vtg") << QByteArray("VTG");
}

void GPSNMEAStreamTest::_awaitedSentenceAfterTwoSeconds()
{
    QFETCH(QByteArray, type);
    // A receiver that reports every five seconds at 4800 baud and sends the GSV sentences of four constellations
    // between its GGA and the sentence that follows: twelve of them put that sentence 1.92 s after the GGA, thirteen
    // put it 2.07 s after.
    constexpr uint64_t INTERVAL_US = 5000000;
    NMEAReceiver receiver;
    const uint64_t start = receiver.now;
    int epoch = 0;
    for (const uint64_t delayUs : {1920000, 1920000, 2070000, 1920000, 1920000, 2070000}) {
        const QByteArray utc = QByteArray::number(123500 + 5 * epoch);
        receiver.now = start + epoch * INTERVAL_US;
        const uint64_t positionReceipt = receiver.now;
        receiver.feed(gga(utc));
        receiver.now += delayUs;
        receiver.feed(sentenceOf(type, utc));
        ++epoch;
        QCOMPARE(receiver.positions, epoch);
        const auto& navigation = receiver.published->navigation;
        QCOMPARE(navigation.timestampUs, positionReceipt);
        if (epoch > 2) {
            // Two epochs have shown the sentence, so this one waited for it and is published with it.
            QVERIFY(type == "GST"   ? std::isfinite(navigation.horizontalAccuracyMeters)
                    : type == "GSA" ? std::isfinite(navigation.verticalDop)
                                    : receiver.published->velocityValid);
        }
    }
}

void GPSNMEAStreamTest::_epochLongerThanTwoSeconds_data()
{
    QTest::addColumn<QByteArrayList>("order");
    QTest::newRow("gga-last") << QByteArrayList{"RMC", "GSA", "GST", "GSV", "GGA"};
    QTest::newRow("gst-first") << QByteArrayList{"GST", "GSV", "GGA"};
    QTest::newRow("gga-first") << QByteArrayList{"GGA", "GSV", "GSA", "RMC", "GST"};
}

void GPSNMEAStreamTest::_epochLongerThanTwoSeconds()
{
    QFETCH(QByteArrayList, order);
    // A receiver that reports every five seconds at 4800 baud, where the GSV sentences of four constellations take
    // 2.07 s: what precedes them and what follows them belong to one epoch.
    constexpr uint64_t INTERVAL_US = 5000000;
    constexpr uint64_t SATELLITES_US = 2070000;
    NMEAReceiver receiver;
    const uint64_t start = receiver.now;
    for (int epoch = 0; epoch < 4; ++epoch) {
        const QByteArray utc = QByteArray::number(123500 + 5 * epoch);
        receiver.now = start + epoch * INTERVAL_US;
        uint64_t positionReceipt = 0;
        for (const auto& type : order) {
            receiver.now += type == "GSV" ? SATELLITES_US : SENTENCE_INTERVAL_US;
            if (type == "GGA") {
                positionReceipt = receiver.now;
            }
            receiver.feed(sentenceOf(type, utc));
        }
        QCOMPARE(receiver.positions, epoch + 1);
        const auto& navigation = receiver.published->navigation;
        QCOMPARE(navigation.timestampUs, positionReceipt);
        // What precedes the GGA is published with it; what follows it is waited for once two epochs have shown it.
        if (order.last() == "GGA" || epoch >= 2) {
            QCOMPARE(receiver.published->velocityValid, order.contains("RMC"));
            QCOMPARE(std::isfinite(navigation.verticalDop), order.contains("GSA"));
            QCOMPARE(std::isfinite(navigation.horizontalAccuracyMeters), order.contains("GST"));
        }
    }
}

void GPSNMEAStreamTest::_fixLossWhileWaiting()
{
    NMEAReceiver receiver;
    for (const QByteArray& utc : {QByteArray("123519"), QByteArray("123520")}) {
        receiver.now += EPOCH_INTERVAL_US;
        receiver.feed(gga(utc));
        receiver.feed(rmc(utc));
    }
    receiver.now += EPOCH_INTERVAL_US;
    receiver.feed(gga("123521"));
    QCOMPARE(receiver.positions, 2);
    receiver.feed("GPRMC,123521,V,,,,,,,230394,,,N");
    QCOMPARE(receiver.positions, 3);
    QCOMPARE(receiver.published->navigation.fixType, GPSFixQuality::NoFix);

    // The epoch that was waiting is not published after the loss: the next position is the next epoch's.
    receiver.now += EPOCH_INTERVAL_US;
    const uint64_t positionReceipt = receiver.now;
    receiver.feed(gga("123522"));
    receiver.feed(rmc("123522"));
    QCOMPARE(receiver.positions, 4);
    QCOMPARE(receiver.published->navigation.timestampUs, positionReceipt);
    QCOMPARE(receiver.published->navigation.fixType, GPSFixQuality::Fix3D);
}

void GPSNMEAStreamTest::_fixLossKeepsSentenceOrder_data()
{
    QTest::addColumn<QByteArrayList>("loss");
    const QByteArray noFixGga("GPGGA,123522,,,,,0,00,,,M,,M,,");
    const QByteArray noFixGsa("GPGSA,A,1,,,,,,,,,,,,,,,");
    const QByteArray voidRmc("GPRMC,123522,V,,,,,,,230394,,,N");
    // The receiver still sends the epoch's GGA as a fix, and the sentences after it declare the loss.
    QTest::newRow("after-the-gga") << QByteArrayList{gga("123522"), noFixGsa, voidRmc};
    // The fix returns after the epoch's GGA, so its RMC begins an epoch that has no GGA.
    QTest::newRow("fix-returns-after-the-gga") << QByteArrayList{noFixGga, GSA, rmc("123522")};
    // The RMC stays void for an epoch longer than the GGA.
    QTest::newRow("rmc-void-longer") << QByteArrayList{noFixGga,      noFixGsa, voidRmc,
                                                       gga("123523"), GSA,      "GPRMC,123523,V,,,,,,,230394,,,N"};
}

void GPSNMEAStreamTest::_fixLossKeepsSentenceOrder()
{
    QFETCH(QByteArrayList, loss);
    NMEAReceiver receiver;
    // Three epochs show that the receiver sends GSA and RMC after its GGA.
    for (const QByteArray& utc : {QByteArray("123519"), QByteArray("123520"), QByteArray("123521")}) {
        receiver.now += EPOCH_INTERVAL_US;
        for (const auto& sentence : {gga(utc), QByteArray(GSA), rmc(utc)}) {
            receiver.feed(sentence);
        }
    }
    QVERIFY(receiver.published->velocityValid);
    receiver.now += EPOCH_INTERVAL_US;
    for (const auto& sentence : loss) {
        receiver.feed(sentence);
    }
    QCOMPARE(receiver.published->navigation.fixType, GPSFixQuality::NoFix);

    // The first epoch after the loss waits for them as the last one before it did.
    receiver.now += 10 * EPOCH_INTERVAL_US;
    const uint64_t positionReceipt = receiver.now;
    const int published = receiver.positions;
    receiver.feed(gga("123530"));
    receiver.feed(GSA);
    QCOMPARE(receiver.positions, published);
    receiver.feed(rmc("123530"));
    QCOMPARE(receiver.positions, published + 1);
    QCOMPARE(receiver.published->navigation.fixType, GPSFixQuality::Fix3D);
    QCOMPARE(receiver.published->navigation.timestampUs, positionReceipt);
    QVERIFY(receiver.published->velocityValid);
}

void GPSNMEAStreamTest::_epochWithoutGga()
{
    // A receiver that sends its RMC every second and its GGA every other second.
    NMEAReceiver receiver;
    int withGga = 0;
    for (int epoch = 0; epoch < 8; ++epoch) {
        const QByteArray utc = QByteArray::number(123519 + epoch);
        receiver.now += EPOCH_INTERVAL_US;
        if (epoch % 2 == 0) {
            receiver.feed(gga(utc));
            ++withGga;
        }
        receiver.feed(rmc(utc));
        // Only a GGA's position is published.
        QCOMPARE(receiver.positions, withGga);
        // An epoch without a GGA does not show what follows one, so two epochs with a GGA show that its RMC does, and
        // the third waits for it.
        if (withGga > 2) {
            QVERIFY(receiver.published->velocityValid);
        }
    }
}

void GPSNMEAStreamTest::_positionNotFromSatellites_data()
{
    QTest::addColumn<QByteArrayList>("order");
    QTest::addColumn<QByteArray>("quality");
    QTest::addColumn<QByteArray>("mode");
    QTest::addColumn<GPSFixQuality>("fixType");
    // A Garmin 10x that loses its satellites reports a dead-reckoned position.
    QTest::newRow("dead-reckoning") << QByteArrayList{"GGA", "GSA", "RMC"} << QByteArray("6") << QByteArray("E")
                                    << GPSFixQuality::Extrapolated;
    // A Garmin handheld receiver in its simulator mode.
    QTest::newRow("simulator") << QByteArrayList{"RMC", "GGA", "GSA", "GLL"} << QByteArray("8") << QByteArray("S")
                               << GPSFixQuality::Fix3D;
    QTest::newRow("manual-input") << QByteArrayList{"GGA", "RMC"} << QByteArray("7") << QByteArray("M")
                                  << GPSFixQuality::Fix3D;
}

void GPSNMEAStreamTest::_positionNotFromSatellites()
{
    QFETCH(QByteArrayList, order);
    QFETCH(QByteArray, quality);
    QFETCH(QByteArray, mode);
    QFETCH(GPSFixQuality, fixType);
    NMEAReceiver receiver;
    for (int epoch = 0; epoch < 4; ++epoch) {
        const QByteArray utc = QByteArray::number(123519 + epoch);
        receiver.now += EPOCH_INTERVAL_US;
        for (const auto& type : order) {
            if (type == "GGA") {
                receiver.feed("GPGGA," + utc + ",4807.038,N,01131.000,E," + quality + ",00,0.9,100.0,M,10.0,M,,");
            } else if (type == "RMC") {
                receiver.feed("GPRMC," + utc + ",V,4807.038,N,01131.000,E,2.0,90.0,230394,,," + mode);
            } else if (type == "GLL") {
                receiver.feed("GPGLL,4807.038,N,01131.000,E," + utc + ",V," + mode);
            } else {
                receiver.feed(sentenceOf(type, utc));
            }
        }
        // The status of the RMC and the GLL is void for every position that is not a satellite fix. Their mode says
        // what the GGA's quality says, so they declare no fix loss and the RMC is the epoch's.
        QCOMPARE(receiver.positions, epoch + 1);
        QCOMPARE(receiver.published->navigation.fixType, fixType);
        if (epoch >= 2) {
            QVERIFY(receiver.published->velocityValid);
        }
    }
}

void GPSNMEAStreamTest::_timeDoesNotAdvance_data()
{
    QTest::addColumn<QByteArrayList>("order");
    QTest::addColumn<int>("reportsPerTime");
    const std::array<std::pair<const char*, QByteArrayList>, 5> orders{{
        {"gga-first", {"GGA", "GSA", "RMC", "VTG"}},
        {"gga-first-with-gst", {"GGA", "GST", "GSA"}},
        {"gga-between", {"RMC", "VTG", "GGA", "GSA"}},
        {"gga-last", {"RMC", "VTG", "GSA", "GST", "GGA"}},
        {"gst-first", {"GST", "GGA", "GSA"}},
    }};
    for (const auto& [name, order] : orders) {
        // A receiver that prints whole seconds and reports five times a second.
        QTest::addRow("whole-seconds-%s", name) << order << 5;
        QTest::addRow("clock-stands-still-%s", name) << order << 1000;
    }
}

void GPSNMEAStreamTest::_timeDoesNotAdvance()
{
    QFETCH(QByteArrayList, order);
    QFETCH(int, reportsPerTime);
    const bool velocity = order.contains("RMC") || order.contains("VTG");
    NMEAReceiver receiver;
    for (int report = 1; report <= 16; ++report) {
        const QByteArray utc = QByteArray::number(123519 + report / reportsPerTime);
        receiver.now += EPOCH_INTERVAL_US / 5;
        for (const auto& type : order) {
            receiver.now += SENTENCE_INTERVAL_US;
            receiver.feed(sentenceOfReport(type, utc, report));
        }
        // Each report is published once, and never with what another report of the same time carried.
        QCOMPARE(receiver.positions, report);
        const auto& navigation = receiver.published->navigation;
        if (receiver.published->velocityValid) {
            QCOMPARE(qRound(qRadiansToDegrees(navigation.courseRadians)), report);
        }
        if (std::isfinite(navigation.verticalDop)) {
            QCOMPARE(navigation.verticalDop, float(report));
        }
        if (std::isfinite(navigation.horizontalAccuracyMeters)) {
            QCOMPARE(navigation.horizontalAccuracyMeters, float(report));
        }
        // Two reports show what the receiver sends after its GGA.
        if (report > 2) {
            QCOMPARE(receiver.published->velocityValid, velocity);
            QCOMPARE(std::isfinite(navigation.verticalDop), order.contains("GSA"));
            QCOMPARE(std::isfinite(navigation.horizontalAccuracyMeters), order.contains("GST"));
        }
    }
}

void GPSNMEAStreamTest::_timeStepsBack()
{
    NMEAReceiver receiver;
    for (const auto& sentence : {rmc("123519"), QByteArray(GSA), gst("123519"), gga("123519")}) {
        receiver.feed(sentence);
    }
    QVERIFY(receiver.published && receiver.published->velocityValid);
    receiver.now += EPOCH_INTERVAL_US / 2;
    receiver.feed(gga("123520"));
    // The receiver reports a time of day again. What the earlier epoch of that time carried is not this one's.
    receiver.now += EPOCH_INTERVAL_US / 2;
    receiver.feed(gga("123519"));
    QCOMPARE(receiver.positions, 3);
    const auto& navigation = receiver.published->navigation;
    QCOMPARE(navigation.timestampUs, receiver.now);
    QVERIFY(!receiver.published->velocityValid);
    QVERIFY(std::isnan(navigation.verticalDop));
    QVERIFY(std::isnan(navigation.horizontalAccuracyMeters));
}

void GPSNMEAStreamTest::_sentenceOfAnotherTalker_data()
{
    QTest::addColumn<QByteArrayList>("order");
    QTest::addColumn<int>("reportsPerTime");
    // A SiRFstar V reports the satellites of each constellation in a GGA of its own.
    QTest::newRow("gga-per-constellation") << QByteArrayList{"GPGGA", "GLGGA", "GPGSA", "GNRMC"} << 1;
    // A Telit PLS83 repeats the RMC as well.
    QTest::newRow("gga-and-rmc-per-constellation") << QByteArrayList{"GPGGA", "GAGGA", "GPRMC", "GARMC"} << 1;
    QTest::newRow("gga-last") << QByteArrayList{"GPRMC", "GARMC", "GPGLL", "GAGLL", "GPGGA", "GAGGA"} << 1;
    // The first talker's sentence again is the next report, also of a receiver whose time stands still.
    QTest::newRow("time-stands-still") << QByteArrayList{"GPGGA", "GLGGA", "GPGSA", "GNRMC"} << 1000;
}

void GPSNMEAStreamTest::_sentenceOfAnotherTalker()
{
    QFETCH(QByteArrayList, order);
    QFETCH(int, reportsPerTime);
    NMEAReceiver receiver;
    for (int report = 1; report <= 6; ++report) {
        const QByteArray utc = QByteArray::number(123519 + report / reportsPerTime);
        receiver.now += EPOCH_INTERVAL_US;
        uint64_t positionReceipt = 0;
        QByteArrayList sent;
        for (const auto& name : order) {
            const QByteArray type = name.mid(NMEA::Sentence::TALKER_LENGTH);
            // The epoch's sentence is the first of its type. The one another talker repeats differs in its satellite
            // count or its course.
            const bool first = !sent.contains(type);
            sent.append(type);
            receiver.now += SENTENCE_INTERVAL_US;
            if (type == "GGA" && first) {
                positionReceipt = receiver.now;
            }
            const QByteArray body = type == "GGA" ? "GPGGA," + utc + ",4807.038,N,01131.000,E,1," +
                                                        (first ? "08" : "02") + ",0.9,100.0,M,10.0,M,,"
                                                  : sentenceOfReport(type, utc, first ? 90 : 180);
            receiver.feed(name.first(NMEA::Sentence::TALKER_LENGTH) + body.mid(NMEA::Sentence::TALKER_LENGTH));
        }
        // One position for each report, however many talkers repeat its sentences.
        QCOMPARE(receiver.positions, report);
        const auto& navigation = receiver.published->navigation;
        QCOMPARE(navigation.timestampUs, positionReceipt);
        QCOMPARE(navigation.satellitesUsed, std::optional<uint8_t>(8));
        // Two reports show what the receiver sends after its GGA.
        if (report > 2) {
            QVERIFY(receiver.published->velocityValid);
            QCOMPARE(qRound(qRadiansToDegrees(navigation.courseRadians)), 90);
        }
    }
}

void GPSNMEAStreamTest::_vdopEpoch_data()
{
    QTest::addColumn<QByteArray>("firstUtc");
    QTest::addColumn<QByteArray>("nextUtc");
    QTest::addColumn<quint64>("elapsedUs");
    QTest::newRow("same-time") << QByteArray("123519") << QByteArray("123519") << quint64(1000);
    QTest::newRow("next-epoch") << QByteArray("123519") << QByteArray("123520") << quint64(1000000);
    QTest::newRow("midnight") << QByteArray("235959") << QByteArray("000000") << quint64(1000000);
    QTest::newRow("same-time-later") << QByteArray("123519") << QByteArray("123519") << quint64(7000000);
    QTest::newRow("later-epoch") << QByteArray("123519") << QByteArray("123526") << quint64(7000000);
    QTest::newRow("missing-epoch") << QByteArray("123519") << QByteArray() << quint64(1000);
}

void GPSNMEAStreamTest::_vdopEpoch()
{
    QFETCH(QByteArray, firstUtc);
    QFETCH(QByteArray, nextUtc);
    QFETCH(quint64, elapsedUs);
    NMEAReceiver receiver;
    receiver.feed(rmc(firstUtc));
    receiver.feed(GSA);
    receiver.feed(gst(firstUtc));
    QVERIFY(!receiver.published);
    receiver.feed(gga(firstUtc));
    QVERIFY(receiver.published);
    QCOMPARE(receiver.published->navigation.horizontalAccuracyMeters, 0.5f);
    QCOMPARE(receiver.published->navigation.verticalDop, 0.6f);
    const auto positionReceipt = receiver.published->navigation.timestampUs;
    receiver.now = positionReceipt + elapsedUs;
    // The next GGA is another epoch, whatever its time, and has only what its own sentences carry.
    receiver.feed(gga(nextUtc));
    QCOMPARE(receiver.published->navigation.timestampUs, receiver.now);
    QCOMPARE(receiver.published->navigation.horizontalDop, 0.9f);
    QVERIFY(std::isnan(receiver.published->navigation.verticalDop));
    QVERIFY(std::isnan(receiver.published->navigation.horizontalAccuracyMeters));
}

void GPSNMEAStreamTest::_unassociatedGsa_data()
{
    QTest::addColumn<qint64>("ageUs");
    QTest::newRow("before-position") << qint64(0);
    QTest::newRow("after-published-position") << qint64(2000001);
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
    // No epoch has begun, or the current one went out: the GSA is not published, with it or with the next one.
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
        "$GPVTG,31.66,T,,M,1.08,N,2.0,K",
        "$GPGSA,A,3,02,,,,,,,,,,,,1.0,1.03,0.6",
        "$GPGST,092750.000,1,1,1,0,3,4,6",
        "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,",
    };
    NMEAReceiver receiver;
    for (const auto& body : bodies) {
        receiver.send(GPSTest::repairChecksum(body));
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
