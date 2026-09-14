#include "NTRIPGgaProviderTest.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "MockNTRIPTransport.h"
#include "NMEAUtils.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"

void NTRIPGgaProviderTest::testSourceClearedOnStopAndFreshStart()
{
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() {
        return PositionResult{QGeoCoordinate(47.3977, 8.5456, 450.0), QStringLiteral("Vehicle GPS")};
    });

    provider.start(&transport);
    QCOMPARE(provider.currentSource(), QStringLiteral("Vehicle GPS"));
    QCOMPARE(transport.sentNmea.size(), 1);

    provider.stop();
    QVERIFY(provider.currentSource().isEmpty());

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() { return PositionResult{}; });
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPGgaProviderTest::testDefaultRTKBaseProvider()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    auto* facts = qobject_cast<GPSRTKFactGroup*>(GPSManager::instance()->gpsRtk()->gpsRtkFactGroup());
    QVERIFY(facts);
    saved.setFactValue(settings->ntripGgaPositionSource(), static_cast<int>(NTRIPGgaProvider::PositionSource::RTKBase));
    saved.setFactValue(facts->valid(), true);
    saved.setFactValue(facts->currentLatitude(), 47.3977);
    saved.setFactValue(facts->currentLongitude(), 8.5456);
    saved.setFactValue(facts->currentAltitude(), 450.0);

    MockNTRIPTransport transport;
    NTRIPGgaProvider provider;
    provider.init(settings);
    provider.start(&transport);
    QCOMPARE(provider.currentSource(), QStringLiteral("RTK Base"));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(transport.sentNmea.first().contains(",4723.8620,N,00832.7360,E,"));
    QVERIFY(transport.sentNmea.first().contains(",450.0,M,"));
    QVERIFY(NMEAUtils::verifyChecksum(transport.sentNmea.first()));
    provider.stop();

    facts->valid()->setRawValue(false);
    transport.sentNmea.clear();
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
    QVERIFY(transport.sentNmea.isEmpty());
}

UT_REGISTER_TEST(NTRIPGgaProviderTest, TestLabel::Unit)
