#include "GPSAsciiProtocolTest.h"

#include <array>
#include <cmath>
#include <optional>
#include <string_view>
#include <utility>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QStringList>
#include <QtTest/QSignalSpy>

#include "GPSAsciiProtocol.h"
#include "GPSProtocolTestIO.h"
#include "NMEAPositionSource.h"
#include "NMEAUtils.h"
#include "ProtocolTestPackets.h"
#include "Quectel/QuectelCodec_p.h"
#include "Support/UnicoreReceiverModel.h"
#include "Unicore/GPSDriverUnicore.h"

namespace {
class AsciiReceiver final : public GPSAsciiProtocol
{
public:
    using GPSAsciiProtocol::GPSAsciiProtocol;

    int configure(unsigned&, const GPSConfig&) override
    {
        resetStream();
        return 0;
    }
};

void feed(GPSProtocol& receiver, const QByteArray& body)
{
    const QByteArray bytes =
        QByteArray::fromStdString(nmeaSentence({body.constData(), static_cast<size_t>(body.size())}));
    receiver.consume({reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size())});
}

QByteArray gga(const QByteArray& utc)
{
    return "GPGGA," + utc + ",4807.038,N,01131.000,E,1,08,0.9,100.0,M,10.0,M,,";
}

constexpr auto GSA = "GPGSA,A,3,01,,,,,,,,,,,,1.0,0.8,0.6";
}  // namespace

void GPSAsciiProtocolTest::_vdopEpoch_data()
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

void GPSAsciiProtocolTest::_vdopEpoch()
{
    QFETCH(QByteArray, firstUtc);
    QFETCH(QByteArray, nextUtc);
    QFETCH(quint64, elapsedUs);
    QFETCH(bool, retained);
    uint64_t now = 1000000;
    GPSNativePositionReport position;
    std::optional<GPSNativePositionReport> published;
    GPSProtocolIO io;
    io.nowUs = [&now] { return now; };
    io.decoded = [&published](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* report = std::get_if<GPSNativePositionReport>(&event)) {
                published = *report;
            }
        }
    };
    AsciiReceiver receiver(captureGPSReports(std::move(io), position), false);
    const auto gst = "GPGST," + firstUtc + ",0,0,0,0,0.3,0.4,0.6";
    feed(receiver, gst);
    QVERIFY(!published);
    feed(receiver, gga(firstUtc));
    QVERIFY(published);
    QCOMPARE(published->navigation.horizontalAccuracyMeters, 0.5f);
    const auto positionReceipt = published->navigation.timestampUs;
    feed(receiver, GSA);
    ++now;
    feed(receiver, gst);
    QCOMPARE(published->navigation.timestampUs, positionReceipt);
    QCOMPARE(published->navigation.verticalDop, 0.6f);
    now = positionReceipt + elapsedUs;
    feed(receiver, gga(nextUtc));
    QCOMPARE(published->navigation.timestampUs, now);
    QCOMPARE(published->navigation.horizontalDop, 0.9f);
    if (retained) {
        QCOMPARE(published->navigation.verticalDop, 0.6f);
    } else {
        QVERIFY(std::isnan(published->navigation.verticalDop));
        QVERIFY(std::isnan(published->navigation.horizontalAccuracyMeters));
    }
}

void GPSAsciiProtocolTest::_vdopReceiptIsNotRenewed()
{
    uint64_t now = 1000000;
    GPSNativePositionReport position;
    GPSProtocolIO io;
    io.nowUs = [&now] { return now; };
    AsciiReceiver receiver(captureGPSReports(std::move(io), position), false);
    feed(receiver, gga("123519"));
    feed(receiver, GSA);
    for (unsigned second = 1; second <= 7; ++second) {
        now += 1000000;
        feed(receiver, gga("123519"));
        QCOMPARE(position.navigation.timestampUs, now);
        if (second <= 2) {
            QCOMPARE(position.navigation.verticalDop, 0.6f);
        } else {
            QVERIFY(std::isnan(position.navigation.verticalDop));
        }
    }
}

void GPSAsciiProtocolTest::_unassociatedGsa_data()
{
    QTest::addColumn<qint64>("ageUs");
    QTest::newRow("before-position") << qint64(0);
    QTest::newRow("expired-position") << qint64(2000001);
    QTest::newRow("clock-regression") << qint64(-1);
}

void GPSAsciiProtocolTest::_unassociatedGsa()
{
    QFETCH(qint64, ageUs);
    uint64_t now = 1000000;
    GPSNativePositionReport position;
    GPSProtocolIO io;
    io.nowUs = [&now] { return now; };
    AsciiReceiver receiver(captureGPSReports(std::move(io), position), false);
    if (ageUs) {
        feed(receiver, gga("123519"));
        now = static_cast<uint64_t>(static_cast<qint64>(now) + ageUs);
    }
    feed(receiver, GSA);
    QVERIFY(std::isnan(position.navigation.verticalDop));
    now = 4000000;
    feed(receiver, gga("123520"));
    QVERIFY(std::isnan(position.navigation.verticalDop));
}

void GPSAsciiProtocolTest::_boundedFields_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QStringList>("expected");
    QTest::newRow("empty") << QByteArray() << QStringList{QString()};
    QTest::newRow("leading-and-trailing") << QByteArray(",a,") << QStringList{"", "a", ""};
    QTest::newRow("capacity") << QByteArray("a,,b,") << QStringList{"a", "", "b", ""};
    QTest::newRow("overflow") << QByteArray("a,b,c,d,e") << QStringList{};
    QTest::newRow("trailing-overflow") << QByteArray("a,b,c,d,") << QStringList{};
}

void GPSAsciiProtocolTest::_boundedFields()
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

void GPSAsciiProtocolTest::_positionSourceEquivalence()
{
    const QList<QByteArray> bodies{
        "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,2.0,31.66,280511,,,A",
        "$GPGSA,A,3,02,,,,,,,,,,,,1.0,1.03,0.6",
        "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,",
        "$GPGST,092750.000,1,1,1,0,3,4,6",
        "$GPVTG,31.66,T,,M,1.08,N,2.0,K",
    };
    QByteArray wire;
    uint64_t now = 1000000;
    GPSNativePositionReport nativePosition;
    GPSProtocolIO io;
    io.nowUs = [&now] { return now; };
    AsciiReceiver receiver(captureGPSReports(std::move(io), nativePosition), false);
    for (const auto& body : bodies) {
        const auto sentence = NMEAUtils::repairChecksum(body);
        wire += sentence;
        receiver.consume(
            {reinterpret_cast<const uint8_t*>(sentence.constData()), static_cast<size_t>(sentence.size())});
    }

    QBuffer device(&wire);
    QVERIFY(device.open(QIODevice::ReadOnly));
    NMEAPositionSource source(&device);
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.requestUpdate(1000);
    emit device.readyRead();
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::shortMs());
    const auto observation = source._lastObservation;

    QCOMPARE(nativePosition.navigation.fixType, GPSPositionReport::FixType::Fix3D);
    QCOMPARE(observation.fixQuality, GPSObservation::FixQuality::Fix3D);
    QVERIFY(qAbs(nativePosition.navigation.latitudeDegrees - observation.position.coordinate().latitude()) < 1e-9);
    QVERIFY(qAbs(nativePosition.navigation.longitudeDegrees - observation.position.coordinate().longitude()) < 1e-9);
    QVERIFY(qAbs(nativePosition.navigation.altitudeMslMeters - observation.position.coordinate().altitude()) < 1e-9);
    QCOMPARE(nativePosition.navigation.satellitesUsed, std::optional<uint8_t>(8));
    QCOMPARE(observation.satellitesUsed, std::optional<int>(8));
    QCOMPARE(nativePosition.navigation.horizontalDop, 1.03f);
    QCOMPARE(nativePosition.navigation.verticalDop, 0.6f);
    QCOMPARE(observation.horizontalDop, std::optional<double>(1.03));
    QCOMPARE(observation.verticalDop, std::optional<double>(0.6));
    QCOMPARE(nativePosition.navigation.horizontalAccuracyMeters, 5.0f);
    QCOMPARE(nativePosition.navigation.verticalAccuracyMeters, 6.0f);
    QCOMPARE(observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy), 5.0);
    QCOMPARE(observation.position.attribute(QGeoPositionInfo::VerticalAccuracy), 6.0);
}

void GPSAsciiProtocolTest::_quectelCodec()
{
    const QByteArray body("PQTMCFGMSGRATE,OK,GGA,1,");
    const QByteArray wire =
        QByteArray::fromStdString(nmeaSentence({body.constData(), static_cast<size_t>(body.size())}));
    const std::string_view bodyView(body.constData(), body.size());
    QCOMPARE(QuectelCodec::frame(bodyView), wire.toStdString());
    const std::string_view line(wire.constData(), wire.size() - 2);
    QCOMPARE(QuectelCodec::checkedBody(line), bodyView);
    auto corrupted = wire;
    corrupted[3] ^= 1;
    QVERIFY(QuectelCodec::checkedBody({corrupted.constData(), static_cast<size_t>(corrupted.size())}).empty());
    QVERIFY(QuectelCodec::checkedBody("$PQTMCFGMSGRATE,OK,GGA,1,*+1").empty());
    QuectelCodec::Fields fields(bodyView);
    QCOMPARE(fields.size(), size_t(5));
    QVERIFY(fields.back().empty());
    fields.pop_back();
    QCOMPARE(fields.size(), size_t(4));
    QCOMPARE(fields[2], std::string_view("GGA"));
    QCOMPARE(QuectelCodec::readback(fields, "PQTMCFGMSGRATE", true), GPSCommandOutcome::ReadbackVerified);
    const QuectelCodec::Fields overflow("PQTMCFGMSGRATE,OK,GGA,1,2,3,4,5,6,7,8,9,10");
    QVERIFY(overflow.overflowed());
    QCOMPARE(QuectelCodec::readback(overflow, "PQTMCFGMSGRATE", true), GPSCommandOutcome::Rejected);
    QCOMPARE(QuectelCodec::readback(overflow, "PQTMCFGSVIN", true), GPSCommandOutcome::Pending);
    double value = 42;
    QVERIFY(QuectelCodec::number("1.25e-1", value));
    QCOMPARE(value, 0.125);
    for (const auto invalid : {"+1", " 1", "1 ", "1junk", "nan", "inf", "1e9999"}) {
        QVERIFY(!QuectelCodec::number(invalid, value));
        QCOMPARE(value, 0.125);
    }
}

void GPSAsciiProtocolTest::_unicoreFailureDetails_data()
{
    QTest::addColumn<int>("fault");
    QTest::addColumn<QString>("expected");
    using Fault = GPSTest::UnicoreReceiver::Fault;
    QTest::newRow("rejected") << int(Fault::Reject) << QStringLiteral("Unicore command 'UNLOG' was rejected");
    QTest::newRow("timeout") << int(Fault::Silence) << QStringLiteral("Unicore command 'UNLOG' timed out");
    QTest::newRow("transport") << int(Fault::WriteError) << QStringLiteral("Unicore test write failure");
    QTest::newRow("cancelled") << int(Fault::Cancel) << QString();
}

void GPSAsciiProtocolTest::_unicoreFailureDetails()
{
    QFETCH(int, fault);
    QFETCH(QString, expected);
    gps_test_time = 0;
    gps_test_warnings.clear();
    GPSTest::UnicoreReceiver peer;
    peer.fault = static_cast<GPSTest::UnicoreReceiver::Fault>(fault);
    peer.faultCommand = "UNLOG";
    GPSNativeUnicore receiver(peer.io(), false);
    unsigned baud = 115200;
    GPSProtocol::GPSConfig config;
    config.base.mode = GPSBaseStationConfig::ReceiverAveraging{1};
    QVERIFY(receiver.configure(baud, config) < 0);
    QVERIFY(!receiver.receiverReady());
    QVERIFY(!peer.results.empty());
    QCOMPARE(peer.results.back().evidence.command, std::string("UNLOG"));
    if (peer.fault == GPSTest::UnicoreReceiver::Fault::Cancel) {
        QCOMPARE(receiver.ioError(), GPSProtocol::ReadCancelled);
        QVERIFY(receiver.ioErrorDetail().isEmpty());
        QVERIFY(gps_test_warnings.empty());
        QCOMPARE(peer.results.back().evidence.outcome, GPSCommandOutcome::Cancelled);
    } else {
        QVERIFY2(receiver.ioErrorDetail().contains(expected), qPrintable(receiver.ioErrorDetail()));
        if (peer.fault == GPSTest::UnicoreReceiver::Fault::WriteError) {
            QCOMPARE(receiver.ioErrorDetail(), expected);
        }
    }
}

void GPSAsciiProtocolTest::_unicoreUnsupportedDetails()
{
    gps_test_time = 0;
    gps_test_warnings.clear();
    GPSTest::UnicoreReceiver peer;
    peer.version = GPSTest::unicoreNative("VERSIONA",
                                          "\"UM982\",\"R5.00Build20000\",\"auth\",\"serial\",\"efuse\",\"2024/08/08\"");
    GPSNativeUnicore receiver(peer.io(), false);
    unsigned baud = 115200;
    GPSProtocol::GPSConfig config;
    config.base.mode = GPSBaseStationConfig::ReceiverAveraging{1};
    QVERIFY(receiver.configure(baud, config) < 0);
    QVERIFY(receiver.ioErrorDetail().contains("Unsupported Unicore receiver"));
    QVERIFY(receiver.ioErrorDetail().contains("R5.00Build20000"));
    QVERIFY(receiver.ioErrorDetail().contains("R4.10Build7650"));
    QCOMPARE(peer.commands.size(), size_t(1));
    peer.version = GPSTest::UNICORE_VERSION;
    QCOMPARE(receiver.configure(baud, config), 0);
    QVERIFY(receiver.ioErrorDetail().isEmpty());
    QVERIFY(receiver.receiverReady());
}

UT_REGISTER_TEST(GPSAsciiProtocolTest, TestLabel::Unit)
