#include "GPSSatelliteInfoSourceTest.h"
#include "GPSSatelliteInfoSource.h"

#include <QtPositioning/QGeoSatelliteInfo>
#include <QtTest/QSignalSpy>

// Build a minimal satellite_info_s with `n` entries.
// Caller fills in svid/prn/used arrays as needed.
static satellite_info_s makeSatInfo(uint8_t count)
{
    satellite_info_s info{};
    info.count = count;
    return info;
}

// ---------------------------------------------------------------------------
// testInitialState
// ---------------------------------------------------------------------------

void GPSSatelliteInfoSourceTest::testInitialState()
{
    GPSSatelliteInfoSource source;

    QCOMPARE(source.error(), QGeoSatelliteInfoSource::NoError);
    QCOMPARE(source.minimumUpdateInterval(), 100);
}

// ---------------------------------------------------------------------------
// testStartStop
// ---------------------------------------------------------------------------

void GPSSatelliteInfoSourceTest::testStartStop()
{
    GPSSatelliteInfoSource source;

    // No crash on start/stop before any data
    source.startUpdates();
    source.stopUpdates();

    // While stopped, updateFromSatelliteInfo must not emit
    source.startUpdates();
    source.stopUpdates();

    QSignalSpy inViewSpy(&source, &QGeoSatelliteInfoSource::satellitesInViewUpdated);
    auto info = makeSatInfo(1);
    info.svid[0] = 1; // GPS svid

    source.updateFromSatelliteInfo(info); // stopped — no emission
    QCOMPARE(inViewSpy.count(), 0);
}

// ---------------------------------------------------------------------------
// testUpdateEmitsSignals
// ---------------------------------------------------------------------------

void GPSSatelliteInfoSourceTest::testUpdateEmitsSignals()
{
    GPSSatelliteInfoSource source;
    source.startUpdates();

    QSignalSpy inViewSpy(&source, &QGeoSatelliteInfoSource::satellitesInViewUpdated);
    QSignalSpy inUseSpy(&source, &QGeoSatelliteInfoSource::satellitesInUseUpdated);
    QVERIFY(inViewSpy.isValid());
    QVERIFY(inUseSpy.isValid());

    // Build a 3-satellite fixture:
    //   sat 0: GPS svid=1,  used
    //   sat 1: GPS svid=2,  not used
    //   sat 2: GLONASS svid=65, used
    auto info = makeSatInfo(3);
    info.svid[0] = 1;   info.used[0] = 1; info.snr[0] = 40;
    info.svid[1] = 2;   info.used[1] = 0; info.snr[1] = 35;
    info.svid[2] = 65;  info.used[2] = 1; info.snr[2] = 30;

    source.updateFromSatelliteInfo(info);

    QCOMPARE(inViewSpy.count(), 1);
    QCOMPARE(inUseSpy.count(), 1);

    const auto inView = inViewSpy.at(0).at(0).value<QList<QGeoSatelliteInfo>>();
    QCOMPARE(inView.size(), 3);

    const auto inUse = inUseSpy.at(0).at(0).value<QList<QGeoSatelliteInfo>>();
    QCOMPARE(inUse.size(), 2); // only sat 0 and sat 2 are used
}

// ---------------------------------------------------------------------------
// testSvidMapping
// ---------------------------------------------------------------------------

void GPSSatelliteInfoSourceTest::testSvidMapping()
{
    GPSSatelliteInfoSource source;
    source.startUpdates();

    QSignalSpy inViewSpy(&source, &QGeoSatelliteInfoSource::satellitesInViewUpdated);

    auto info = makeSatInfo(3);
    // GPS:     svid 1–32  → QGeoSatelliteInfo::GPS
    info.svid[0] = 1;
    // GLONASS: svid 65–96 → QGeoSatelliteInfo::GLONASS
    info.svid[1] = 65;
    // Galileo: svid 211–246 → QGeoSatelliteInfo::GALILEO
    info.svid[2] = 211;

    source.updateFromSatelliteInfo(info);
    QCOMPARE(inViewSpy.count(), 1);

    const auto sats = inViewSpy.at(0).at(0).value<QList<QGeoSatelliteInfo>>();
    QCOMPARE(sats.size(), 3);

    QCOMPARE(sats.at(0).satelliteSystem(), QGeoSatelliteInfo::GPS);
    QCOMPARE(sats.at(1).satelliteSystem(), QGeoSatelliteInfo::GLONASS);
    QCOMPARE(sats.at(2).satelliteSystem(), QGeoSatelliteInfo::GALILEO);
}

UT_REGISTER_TEST(GPSSatelliteInfoSourceTest, TestLabel::Unit)
