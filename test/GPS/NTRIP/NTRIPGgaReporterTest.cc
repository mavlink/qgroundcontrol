#include "NTRIPGgaReporterTest.h"

#include <chrono>
#include <optional>

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "ManualScheduler.h"
#include "NMEASentence.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIPGgaReporter.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Support/GPSTestHelpers.h"

using namespace GPSTest;

namespace {

using Source = NTRIPGgaReporter::PositionSource;

constexpr char kGgaProviderLog[] = "GPS.NTRIPGgaProvider";

}  // namespace

void NTRIPGgaReporterTest::_sourceClearedOnStopAndFreshStart()
{
    NTRIPGgaReporter provider;
    MockNTRIPTransport transport;

    provider.setPositionProvider(NTRIPGgaReporter::PositionSource::VehicleGPS,
                                 []() { return ggaObservation(QGeoCoordinate(47.3977, 8.5456, 450.0)); });

    provider.start(&transport);
    QCOMPARE(provider.currentSource(), NTRIPGgaReporter::tr("Vehicle GPS"));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(transport.sentNmea.first().startsWith("$GPGGA,"));
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.at(NMEA::Field::GGA_QUALITY).toUInt(), NMEA::GgaQuality::ESTIMATED);
    QVERIFY(fields.at(NMEA::Field::GGA_HDOP).isEmpty());
    QVERIFY(fields.at(NMEA::Field::GGA_SATELLITES_USED).isEmpty());

    provider.stop();
    QVERIFY(provider.currentSource().isEmpty());

    provider.setPositionProvider(NTRIPGgaReporter::PositionSource::VehicleGPS,
                                 []() { return std::optional<GPSObservation>{}; });
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPGgaReporterTest::_gcsObservation_data()
{
    QTest::addColumn<QGeoCoordinate>("coordinate");
    QTest::addColumn<double>("horizontalAccuracy");
    QTest::addColumn<bool>("fixValid");
    QTest::addColumn<bool>("accepted");
    const QGeoCoordinate position(47.3977, 8.5456, 450);
    QTest::newRow("missing-vertical-accuracy") << position << 1.0 << true << true;
    QTest::newRow("missing-all-accuracy") << position << qQNaN() << true << true;
    QTest::newRow("unknown-altitude") << QGeoCoordinate(47.3977, 8.5456) << 1.0 << true << false;
    QTest::newRow("infinite-altitude") << QGeoCoordinate(47.3977, 8.5456, qInf()) << 1.0 << true << false;
    QTest::newRow("zero-island") << QGeoCoordinate(0, 0, 450) << 1.0 << true << true;
    QTest::newRow("invalid-coordinate") << QGeoCoordinate() << 1.0 << true << false;
    QTest::newRow("no-fix") << position << 1.0 << false << false;
}

void NTRIPGgaReporterTest::_gcsObservation()
{
    QFETCH(QGeoCoordinate, coordinate);
    QFETCH(double, horizontalAccuracy);
    QFETCH(bool, fixValid);
    QFETCH(bool, accepted);
    NTRIPGgaReporter provider;
    MockNTRIPTransport transport;
    provider.configure({Source::GCSPosition});
    provider.setPositionProvider(Source::GCSPosition, [&]() {
        return ggaObservation(coordinate, fixValid ? GPSFixQuality::Fix3D : GPSFixQuality::NoFix, std::nullopt,
                              qIsFinite(horizontalAccuracy) ? std::optional<double>(horizontalAccuracy) : std::nullopt);
    });
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), accepted ? 1 : 0);
    QCOMPARE(provider.currentSource(), accepted ? NTRIPGgaReporter::tr("GCS Position") : QString());
    if (accepted) {
        const auto& wire = transport.sentNmea.first();
        const auto decoded = NMEA::sentence(std::string_view(wire.constData(), wire.size()));
        QVERIFY(decoded);
        const auto fix = NMEA::gga(*decoded);
        QVERIFY(fix);
        QVERIFY(qAbs(fix->latitude - coordinate.latitude()) < 1e-6);
        QVERIFY(qAbs(fix->longitude - coordinate.longitude()) < 1e-6);
        const auto fields = transport.sentNmea.first().split(',');
        QCOMPARE(fields.size(), 15);
        QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE), QByteArray("450.0"));
        QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE_UNITS), QByteArray("M"));
        QVERIFY(fields.at(NMEA::Field::GGA_GEOID_SEPARATION).isEmpty());
        QCOMPARE(fields.at(NMEA::Field::GGA_GEOID_UNITS), QByteArray("M"));
        QVERIFY(checksumValid(transport.sentNmea.first()));
    }
}

void NTRIPGgaReporterTest::_providerMetadata_data()
{
    QTest::addColumn<GPSFixQuality>("quality");
    QTest::addColumn<unsigned>("expectedQuality");
    using Quality = GPSFixQuality;
    QTest::newRow("gps") << Quality::Fix3D << NMEA::GgaQuality::GPS;
    QTest::newRow("differential") << Quality::Differential << NMEA::GgaQuality::DIFFERENTIAL;
    QTest::newRow("rtk-float") << Quality::RTKFloat << NMEA::GgaQuality::RTK_FLOAT;
    QTest::newRow("rtk-fixed") << Quality::RTKFixed << NMEA::GgaQuality::RTK_FIXED;
    QTest::newRow("estimated") << Quality::Extrapolated << NMEA::GgaQuality::ESTIMATED;
    QTest::newRow("unknown") << Quality::Unknown << NMEA::GgaQuality::ESTIMATED;
}

void NTRIPGgaReporterTest::_providerMetadata()
{
    QFETCH(GPSFixQuality, quality);
    QFETCH(unsigned, expectedQuality);
    NTRIPGgaReporter provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::VehicleGPS,
                                 [quality]() { return ggaObservation(QGeoCoordinate(47, 8, 450), quality, 18, 0.7); });
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.at(NMEA::Field::GGA_QUALITY).toUInt(), expectedQuality);
    QCOMPARE(fields.at(NMEA::Field::GGA_SATELLITES_USED).toUInt(), 18u);
    QCOMPARE(fields.at(NMEA::Field::GGA_HDOP).toDouble(), 0.7);
}

void NTRIPGgaReporterTest::_ggaSourceSelection()
{
    NTRIPGgaReporter provider;
    MockNTRIPTransport transport;
    QList<Source> calls;
    for (auto source : {Source::VehicleGPS, Source::VehicleEKF, Source::RTKReceiver, Source::GCSPosition}) {
        provider.setPositionProvider(source, [&, source]() {
            calls.append(source);
            return source == Source::RTKReceiver ? ggaObservation(QGeoCoordinate(47, 8, 450)) : std::nullopt;
        });
    }
    provider.start(&transport);
    QCOMPARE(calls, (QList<Source>{Source::VehicleGPS, Source::VehicleEKF, Source::RTKReceiver}));
    QCOMPARE(transport.sentNmea.size(), 1);
    provider.stop();
    calls.clear();
    provider.configure({Source::VehicleEKF});
    provider.start(&transport);
    QCOMPARE(calls, (QList<Source>{Source::VehicleEKF}));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPGgaReporterTest::_ggaSelectionDiagnostics()
{
    const TestFixtures::LoggingCategoryFixture logging(kGgaProviderLog);
    ManualScheduler scheduler;
    NTRIPGgaReporter provider(nullptr, &scheduler);
    MockNTRIPTransport transport;
    constexpr std::chrono::milliseconds interval{100};
    const auto sendFastRetry = [&]() { QVERIFY(scheduler.advanceBy(NTRIPGgaReporter::FAST_RETRY_INTERVAL)); };
    const auto sendNormal = [&]() { QVERIFY(scheduler.advanceBy(interval)); };
    bool vehicleAvailable = false;
    bool gcsAvailable = false;
    QGeoCoordinate gcsPosition(48.125, 9.25, 123.4);
    provider.setPositionProvider(Source::VehicleGPS, [&]() {
        return vehicleAvailable ? ggaObservation(QGeoCoordinate(47.3977, 8.5456, 450)) : std::nullopt;
    });
    provider.setPositionProvider(Source::RTKReceiver, []() { return ggaObservation(QGeoCoordinate(47, 8)); });
    provider.setPositionProvider(Source::GCSPosition,
                                 [&]() { return gcsAvailable ? ggaObservation(gcsPosition) : std::nullopt; });

    provider.configure({Source::Auto, interval});
    QStringList expected{QStringLiteral("GGA source selection: requested=Auto no eligible source")};
    expectLogMessage(kGgaProviderLog, QtDebugMsg, exactMessage(expected.last()));
    provider.start(&transport);
    verifyExpectedLogMessage();
    sendFastRetry();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    QVERIFY(transport.sentNmea.isEmpty());

    gcsAvailable = true;
    expected.append(QStringLiteral("GGA source selection: requested=Auto provider=GCSPosition fallback=yes"));
    expectLogMessage(kGgaProviderLog, QtDebugMsg, exactMessage(expected.last()));
    sendFastRetry();
    verifyExpectedLogMessage();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    QCOMPARE(provider.currentSource(), NTRIPGgaReporter::tr("GCS Position"));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(checksumValid(transport.sentNmea.last()));

    gcsPosition.setLatitude(48.25);
    sendNormal();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    QCOMPARE(transport.sentNmea.size(), 2);
    QVERIFY(transport.sentNmea.first() != transport.sentNmea.last());

    vehicleAvailable = true;
    expected.append(QStringLiteral("GGA source selection: requested=Auto provider=VehicleGPS fallback=no"));
    expectLogMessage(kGgaProviderLog, QtDebugMsg, exactMessage(expected.last()));
    sendNormal();
    verifyExpectedLogMessage();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    QCOMPARE(provider.currentSource(), NTRIPGgaReporter::tr("Vehicle GPS"));

    provider.configure({Source::GCSPosition, interval});
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    expected.append(QStringLiteral("GGA source selection: requested=GCSPosition provider=GCSPosition fallback=no"));
    expectLogMessage(kGgaProviderLog, QtDebugMsg, exactMessage(expected.last()));
    sendNormal();
    verifyExpectedLogMessage();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    QCOMPARE(provider.currentSource(), NTRIPGgaReporter::tr("GCS Position"));
    QCOMPARE(transport.sentNmea.size(), 4);

    provider.configure({Source::RTKReceiver, interval});
    expected.append(QStringLiteral("GGA source selection: requested=RTKReceiver no eligible source"));
    expectLogMessage(kGgaProviderLog, QtDebugMsg, exactMessage(expected.last()));
    sendNormal();
    verifyExpectedLogMessage();
    sendFastRetry();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    QCOMPARE(transport.sentNmea.size(), 4);

    vehicleAvailable = false;
    gcsAvailable = false;
    provider.configure({Source::Auto, interval});
    expected.append(QStringLiteral("GGA source selection: requested=Auto no eligible source"));
    expectLogMessage(kGgaProviderLog, QtDebugMsg, exactMessage(expected.last()));
    sendFastRetry();
    verifyExpectedLogMessage();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    provider.stop();
    sendFastRetry();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    expected.append(QStringLiteral("GGA source selection: requested=Auto no eligible source"));
    expectLogMessage(kGgaProviderLog, QtDebugMsg, exactMessage(expected.last()));
    provider.start(&transport);
    verifyExpectedLogMessage();
    QCOMPARE(debugMessages(kGgaProviderLog), expected);
    QCOMPARE(transport.sentNmea.size(), 4);
}

void NTRIPGgaReporterTest::_ggaSourceChangesPreserveCadence()
{
    constexpr std::chrono::milliseconds INTERVAL{100};
    ManualScheduler scheduler;
    NTRIPGgaReporter provider(nullptr, &scheduler);
    MockNTRIPTransport transport;
    Source selected = Source::VehicleGPS;
    bool usedSelectedSource = true;
    for (const auto source : {Source::VehicleGPS, Source::RTKReceiver}) {
        provider.setPositionProvider(source, [&, source]() {
            usedSelectedSource &= source == selected;
            return ggaObservation(QGeoCoordinate(47, 8, 450));
        });
    }
    provider.configure({selected, INTERVAL});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);

    int sourceChanges = 0;
    // Keep changing sources until three scheduled sends survive reconfiguration.
    while (transport.sentNmea.size() < 4 && sourceChanges < 100) {
        selected = selected == Source::VehicleGPS ? Source::RTKReceiver : Source::VehicleGPS;
        ++sourceChanges;
        const auto sentBefore = transport.sentNmea.size();
        provider.configure({selected, INTERVAL});
        QCOMPARE(transport.sentNmea.size(), sentBefore);
        QVERIFY(scheduler.advanceBy(INTERVAL / 4));
    }
    provider.stop();
    QCOMPARE(transport.sentNmea.size(), 4);
    QVERIFY(sourceChanges > 3);
    QVERIFY(usedSelectedSource);
}

void NTRIPGgaReporterTest::_ggaIntervalChangesRestartCadence_data()
{
    QTest::addColumn<int>("initialIntervalMs");
    QTest::addColumn<int>("updatedIntervalMs");
    QTest::newRow("shorter") << 3600000 << 100;
    QTest::newRow("longer") << 100 << 1000;
}

void NTRIPGgaReporterTest::_ggaIntervalChangesRestartCadence()
{
    QFETCH(int, initialIntervalMs);
    QFETCH(int, updatedIntervalMs);
    ManualScheduler scheduler;
    NTRIPGgaReporter provider(nullptr, &scheduler);
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::VehicleGPS, []() { return ggaObservation(QGeoCoordinate(47, 8, 450)); });
    provider.setPositionProvider(Source::RTKReceiver, []() { return ggaObservation(QGeoCoordinate(48, 9, 460)); });
    provider.configure({Source::VehicleGPS, std::chrono::milliseconds{initialIntervalMs}});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);

    QSignalSpy sourceChanged(&provider, &NTRIPGgaReporter::sourceChanged);
    provider.configure({Source::RTKReceiver, std::chrono::milliseconds{updatedIntervalMs}});
    QCOMPARE(transport.sentNmea.size(), 1);
    // The next send follows the updated interval, counted from the change.
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds{updatedIntervalMs - 1}));
    QVERIFY(sourceChanged.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds{1}));
    QVERIFY(!sourceChanged.isEmpty());
    provider.stop();
    QCOMPARE(transport.sentNmea.size(), 2);
    QCOMPARE(sourceChanged.first().first().toString(), NTRIPGgaReporter::tr("RTK Receiver"));
}

void NTRIPGgaReporterTest::_ggaConfigurationPreservesFastRetry()
{
    ManualScheduler scheduler;
    NTRIPGgaReporter provider(nullptr, &scheduler);
    MockNTRIPTransport transport;
    provider.configure({Source::VehicleGPS, std::chrono::hours{1}});
    provider.setPositionProvider(Source::RTKReceiver, []() { return ggaObservation(QGeoCoordinate(47, 8, 450)); });
    QSignalSpy sourceChanged(&provider, &NTRIPGgaReporter::sourceChanged);
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    provider.configure({Source::RTKReceiver, std::chrono::milliseconds{100}});
    QVERIFY(transport.sentNmea.isEmpty());
    // The configuration change keeps the pending fast retry.
    QVERIFY(scheduler.advanceBy(NTRIPGgaReporter::FAST_RETRY_INTERVAL - std::chrono::milliseconds{1}));
    QVERIFY(sourceChanged.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds{1}));
    QVERIFY(!sourceChanged.isEmpty());
    QCOMPARE(transport.sentNmea.size(), 1);
    QCOMPARE(provider.currentSource(), NTRIPGgaReporter::tr("RTK Receiver"));
    // Cadence keeps running while polling.
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds{100}));
    QCOMPARE(transport.sentNmea.size(), 2);
    provider.stop();
}

void NTRIPGgaReporterTest::_ggaFastRetryUntilFirstFix()
{
    ManualScheduler scheduler;
    NTRIPGgaReporter provider(nullptr, &scheduler);
    MockNTRIPTransport transport;
    bool fixed = false;
    provider.setPositionProvider(NTRIPGgaReporter::PositionSource::VehicleGPS,
                                 [&]() { return fixed ? ggaObservation(QGeoCoordinate(47, 8, 450)) : std::nullopt; });
    provider.configure({NTRIPGgaReporter::PositionSource::Auto, std::chrono::seconds{60}});
    provider.start(&transport);
    // A position that appears long after connecting still reaches the caster at the fast cadence.
    QVERIFY(scheduler.advanceBy(10 * NTRIPGgaReporter::FAST_RETRY_INTERVAL));
    QVERIFY(transport.sentNmea.isEmpty());
    fixed = true;
    QVERIFY(scheduler.advanceBy(NTRIPGgaReporter::FAST_RETRY_INTERVAL));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(scheduler.advanceBy(NTRIPGgaReporter::FAST_RETRY_INTERVAL));
    QCOMPARE(transport.sentNmea.size(), 1);
    provider.stop();
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPGgaReporterTest, TestLabel::Unit)
