#include <chrono>
#include <memory>
#include <utility>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "ManualScheduler.h"
#include "MockNTRIPTransport.h"
#include "NMEASentence.h"
#include "NMEAUtils.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPGgaProviderTest.h"
#include "NTRIPTestSupport.h"

using namespace NTRIPTestSupport;

void NTRIPGgaProviderTest::_expectDebugMessage(const char* category, const QString& message)
{
    expectLogMessage(category, QtDebugMsg,
                     QRegularExpression(QRegularExpression::anchoredPattern(QRegularExpression::escape(message))));
}

void NTRIPGgaProviderTest::_verifyDebugMessage()
{
    verifyExpectedLogMessage();
}

void NTRIPGgaProviderTest::ggaAltitudeDatum_data()
{
    QTest::addColumn<GPSAltitudeDatum>("datum");
    QTest::addColumn<bool>("accepted");
    QTest::newRow("unknown") << GPSAltitudeDatum::Unknown << false;
    QTest::newRow("ellipsoid") << GPSAltitudeDatum::Ellipsoid << false;
    QTest::newRow("explicit-msl") << GPSAltitudeDatum::MeanSeaLevel << true;
}

void NTRIPGgaProviderTest::ggaAltitudeDatum()
{
    QFETCH(GPSAltitudeDatum, datum);
    QFETCH(bool, accepted);
    using Source = NTRIPGgaProvider::PositionSource;
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::RTKReceiver, [datum]() {
        return PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("RTK"), datum};
    });
    provider.setPositionProvider(Source::GCSPosition, []() {
        return PositionResult{QGeoCoordinate(48, 9, 100), QStringLiteral("GCS"), GPSAltitudeDatum::MeanSeaLevel};
    });
    provider.configure({Source::RTKReceiver});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), accepted ? 1 : 0);
    provider.stop();
    transport.sentNmea.clear();
    provider.configure({Source::Auto});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);
    QCOMPARE(provider.currentSource(), accepted ? QStringLiteral("RTK") : QStringLiteral("GCS"));
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.size(), 15);
    QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE), accepted ? QByteArray("450.0") : QByteArray("100.0"));
    QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE_UNITS), QByteArray("M"));
    QVERIFY(fields.at(NMEA::Field::GGA_GEOID_SEPARATION).isEmpty());
    QCOMPARE(fields.at(NMEA::Field::GGA_GEOID_UNITS), QByteArray("M"));
    QVERIFY(NMEAUtils::verifyChecksum(transport.sentNmea.first()));
}

void NTRIPGgaProviderTest::ggaSourceSelection()
{
    using Source = NTRIPGgaProvider::PositionSource;
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    QList<Source> calls;
    for (auto source : {Source::VehicleGPS, Source::VehicleEKF, Source::RTKReceiver, Source::GCSPosition}) {
        provider.setPositionProvider(source, [&, source]() {
            calls.append(source);
            return source == Source::RTKReceiver ? PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("RTK"),
                                                                  GPSAltitudeDatum::MeanSeaLevel}
                                                 : PositionResult{};
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

void NTRIPGgaProviderTest::ggaSelectionDiagnostics()
{
    using Source = NTRIPGgaProvider::PositionSource;
    const DebugCapture logs("GPS.NTRIP.NTRIPGgaProvider");
    ManualScheduler scheduler;
    NTRIPGgaProvider provider(nullptr, &scheduler);
    MockNTRIPTransport transport;
    constexpr std::chrono::milliseconds interval{100};
    const auto sendFastRetry = [&]() { QVERIFY(scheduler.advanceBy(NTRIPGgaProvider::kFastRetryInterval)); };
    const auto sendNormal = [&]() { QVERIFY(scheduler.advanceBy(interval)); };
    bool vehicleAvailable = false;
    bool gcsAvailable = false;
    QGeoCoordinate gcsPosition(48.125, 9.25, 123.4);
    provider.setPositionProvider(Source::VehicleGPS, [&]() {
        return vehicleAvailable ? PositionResult{QGeoCoordinate(47.3977, 8.5456, 450), QStringLiteral("Vehicle GPS"),
                                                 GPSAltitudeDatum::MeanSeaLevel}
                                : PositionResult{};
    });
    provider.setPositionProvider(Source::RTKReceiver, []() {
        return PositionResult{QGeoCoordinate(47, 8, 500), QStringLiteral("RTK Base"), GPSAltitudeDatum::Ellipsoid};
    });
    provider.setPositionProvider(Source::GCSPosition, [&]() {
        return gcsAvailable
                   ? PositionResult{gcsPosition, QStringLiteral("GCS Position"), GPSAltitudeDatum::MeanSeaLevel}
                   : PositionResult{};
    });

    provider.configure({Source::Auto, interval});
    QStringList expected{QStringLiteral("GGA source selection: requested=Auto no eligible source")};
    _expectDebugMessage("GPS.NTRIP.NTRIPGgaProvider", expected.last());
    provider.start(&transport);
    _verifyDebugMessage();
    sendFastRetry();
    QCOMPARE(logs.messages(), expected);
    QVERIFY(transport.sentNmea.isEmpty());

    gcsAvailable = true;
    expected.append(QStringLiteral("GGA source selection: requested=Auto provider=GCSPosition fallback=yes"));
    _expectDebugMessage("GPS.NTRIP.NTRIPGgaProvider", expected.last());
    sendFastRetry();
    _verifyDebugMessage();
    QCOMPARE(logs.messages(), expected);
    QCOMPARE(provider.currentSource(), QStringLiteral("GCS Position"));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(NMEAUtils::verifyChecksum(transport.sentNmea.last()));

    gcsPosition.setLatitude(48.25);
    sendNormal();
    QCOMPARE(logs.messages(), expected);
    QCOMPARE(transport.sentNmea.size(), 2);
    QVERIFY(transport.sentNmea.first() != transport.sentNmea.last());

    vehicleAvailable = true;
    expected.append(QStringLiteral("GGA source selection: requested=Auto provider=VehicleGPS fallback=no"));
    _expectDebugMessage("GPS.NTRIP.NTRIPGgaProvider", expected.last());
    sendNormal();
    _verifyDebugMessage();
    QCOMPARE(logs.messages(), expected);
    QCOMPARE(provider.currentSource(), QStringLiteral("Vehicle GPS"));

    provider.configure({Source::GCSPosition, interval});
    QCOMPARE(logs.messages(), expected);
    expected.append(QStringLiteral("GGA source selection: requested=GCSPosition provider=GCSPosition fallback=no"));
    _expectDebugMessage("GPS.NTRIP.NTRIPGgaProvider", expected.last());
    sendNormal();
    _verifyDebugMessage();
    QCOMPARE(logs.messages(), expected);
    QCOMPARE(provider.currentSource(), QStringLiteral("GCS Position"));
    QCOMPARE(transport.sentNmea.size(), 4);

    provider.configure({Source::RTKReceiver, interval});
    expected.append(QStringLiteral("GGA source selection: requested=RTKReceiver no eligible source"));
    _expectDebugMessage("GPS.NTRIP.NTRIPGgaProvider", expected.last());
    sendNormal();
    _verifyDebugMessage();
    sendFastRetry();
    QCOMPARE(logs.messages(), expected);
    QCOMPARE(transport.sentNmea.size(), 4);

    vehicleAvailable = false;
    gcsAvailable = false;
    provider.configure({Source::Auto, interval});
    expected.append(QStringLiteral("GGA source selection: requested=Auto no eligible source"));
    _expectDebugMessage("GPS.NTRIP.NTRIPGgaProvider", expected.last());
    sendFastRetry();
    _verifyDebugMessage();
    QCOMPARE(logs.messages(), expected);
    provider.stop();
    sendFastRetry();
    QCOMPARE(logs.messages(), expected);
    expected.append(QStringLiteral("GGA source selection: requested=Auto no eligible source"));
    _expectDebugMessage("GPS.NTRIP.NTRIPGgaProvider", expected.last());
    provider.start(&transport);
    _verifyDebugMessage();
    QCOMPARE(logs.messages(), expected);
    QCOMPARE(transport.sentNmea.size(), 4);
}

void NTRIPGgaProviderTest::ggaDiagnosticRetiresProvider_data()
{
    QTest::addColumn<int>("action");
    QTest::newRow("stop") << 0;
    QTest::newRow("delete") << 1;
    QTest::newRow("restart") << 2;
}

void NTRIPGgaProviderTest::ggaDiagnosticRetiresProvider()
{
    QFETCH(int, action);
    DebugCapture logs("GPS.NTRIP.NTRIPGgaProvider");
    auto provider = std::make_unique<NTRIPGgaProvider>();
    MockNTRIPTransport transport;
    provider->setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() {
        return PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("Vehicle GPS"),
                              GPSAltitudeDatum::MeanSeaLevel};
    });
    bool handled = false;
    logs.onMessage = [&]() {
        if (std::exchange(handled, true)) {
            return;
        }
        if (action == 0) {
            provider->stop();
        } else if (action == 1) {
            provider.reset();
        } else {
            provider->start(&transport);
        }
    };
    _expectDebugMessage("GPS.NTRIP.NTRIPGgaProvider",
                        QStringLiteral("GGA source selection: requested=Auto provider=VehicleGPS fallback=no"));
    provider->start(&transport);
    _verifyDebugMessage();
    QVERIFY(handled);
    QCOMPARE(transport.sentNmea.size(), action == 2 ? 1 : 0);
    if (action == 1) {
        QVERIFY(!provider);
    } else {
        QCOMPARE(provider->currentSource(), action == 2 ? QStringLiteral("Vehicle GPS") : QString());
    }
}

void NTRIPGgaProviderTest::ggaSourceChangesPreserveCadence()
{
    using Source = NTRIPGgaProvider::PositionSource;
    constexpr std::chrono::milliseconds INTERVAL{100};
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    Source selected = Source::VehicleGPS;
    bool usedSelectedSource = true;
    for (const auto source : {Source::VehicleGPS, Source::RTKReceiver}) {
        provider.setPositionProvider(source, [&, source]() {
            usedSelectedSource &= source == selected;
            return PositionResult{QGeoCoordinate(47, 8, 450), QString::number(static_cast<int>(source)),
                                  GPSAltitudeDatum::MeanSeaLevel};
        });
    }
    provider.configure({selected, INTERVAL});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);

    int sourceChanges = 0;
    QTimer reconfigure;
    connect(&reconfigure, &QTimer::timeout, this, [&]() {
        if (transport.sentNmea.size() >= 4) {
            reconfigure.stop();
            provider.stop();
            return;
        }
        selected = selected == Source::VehicleGPS ? Source::RTKReceiver : Source::VehicleGPS;
        ++sourceChanges;
        const auto sentBefore = transport.sentNmea.size();
        provider.configure({selected, INTERVAL});
        QCOMPARE(transport.sentNmea.size(), sentBefore);
    });
    // Keep changing sources until three scheduled sends survive reconfiguration.
    reconfigure.start(0);
    QTRY_COMPARE_WITH_TIMEOUT(transport.sentNmea.size(), 4, TestTimeout::mediumMs());
    reconfigure.stop();
    provider.stop();
    QVERIFY(sourceChanges > 3);
    QVERIFY(usedSelectedSource);
}

void NTRIPGgaProviderTest::ggaIntervalChangesRestartCadence_data()
{
    QTest::addColumn<int>("initialIntervalMs");
    QTest::addColumn<int>("updatedIntervalMs");
    QTest::newRow("shorter") << 3600000 << 100;
    QTest::newRow("longer") << 100 << 1000;
}

void NTRIPGgaProviderTest::ggaIntervalChangesRestartCadence()
{
    QFETCH(int, initialIntervalMs);
    QFETCH(int, updatedIntervalMs);
    using Source = NTRIPGgaProvider::PositionSource;
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::VehicleGPS, []() {
        return PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("GPS"), GPSAltitudeDatum::MeanSeaLevel};
    });
    provider.setPositionProvider(Source::RTKReceiver, []() {
        return PositionResult{QGeoCoordinate(48, 9, 460), QStringLiteral("RTK"), GPSAltitudeDatum::MeanSeaLevel};
    });
    provider.configure({Source::VehicleGPS, std::chrono::milliseconds{initialIntervalMs}});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);

    QSignalSpy sourceChanged(&provider, &NTRIPGgaProvider::sourceChanged);
    QElapsedTimer elapsed;
    elapsed.start();
    provider.configure({Source::RTKReceiver, std::chrono::milliseconds{updatedIntervalMs}});
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(sourceChanged.wait(TestTimeout::mediumMs()));
    provider.stop();
    QCOMPARE(transport.sentNmea.size(), 2);
    QCOMPARE(sourceChanged.first().first().toString(), QStringLiteral("RTK"));
    // Allow coarse-timer early delivery, without constraining late CI scheduling.
    QVERIFY(elapsed.elapsed() >= updatedIntervalMs * 4 / 5);
}

void NTRIPGgaProviderTest::ggaConfigurationPreservesFastRetry()
{
    using Source = NTRIPGgaProvider::PositionSource;
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.configure({Source::VehicleGPS, std::chrono::hours{1}});
    provider.setPositionProvider(Source::RTKReceiver, []() {
        return PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("RTK"), GPSAltitudeDatum::MeanSeaLevel};
    });
    QSignalSpy sourceChanged(&provider, &NTRIPGgaProvider::sourceChanged);
    QElapsedTimer elapsed;
    elapsed.start();
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    provider.configure({Source::RTKReceiver, std::chrono::milliseconds{100}});
    QVERIFY(transport.sentNmea.isEmpty());
    QVERIFY(sourceChanged.wait(TestTimeout::mediumMs()));
    QVERIFY(elapsed.elapsed() >= NTRIPGgaProvider::kFastRetryInterval.count() * 4 / 5);
    QCOMPARE(transport.sentNmea.size(), 1);
    QCOMPARE(provider.currentSource(), QStringLiteral("RTK"));
    QTRY_COMPARE_WITH_TIMEOUT(transport.sentNmea.size(), 2, TestTimeout::mediumMs());
    provider.stop();
}

void NTRIPGgaProviderTest::ggaCallbackStopsProvider()
{
    const DebugCapture logs("GPS.NTRIP.NTRIPGgaProvider");
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, [&]() {
        provider.stop();
        return PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("Vehicle GPS"),
                              GPSAltitudeDatum::MeanSeaLevel};
    });
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    QVERIFY(provider.currentSource().isEmpty());
    QVERIFY(logs.messages().isEmpty());
}
