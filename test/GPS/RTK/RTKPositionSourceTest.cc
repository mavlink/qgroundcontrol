#include "RTKPositionSourceTest.h"

#include <QtCore/QTimeZone>
#include <QtCore/QtMath>
#include <QtTest/QSignalSpy>

#include <chrono>

#include "RTKPositionSource.h"

namespace {
sensor_gps_s positionFix()
{
    sensor_gps_s fix{};
    fix.fix_type = sensor_gps_s::FIX_TYPE_RTK_FIXED;
    fix.latitude_deg = 47.5;
    fix.longitude_deg = 8.5;
    fix.altitude_msl_m = 450;
    fix.altitude_ellipsoid_m = 497;
    fix.eph = 0.1f;
    fix.epv = 0.2f;
    fix.time_utc_usec = 1788864000000000ULL;
    return fix;
}
}  // namespace

void RTKPositionSourceTest::_convertsFixAndMotion()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    auto fix = positionFix();
    fix.vel_ned_valid = true;
    fix.vel_m_s = 2.5f;
    fix.vel_d_m_s = -1.5f;
    fix.cog_rad = qDegreesToRadians(-90.0f);
    fix.c_variance_rad = qDegreesToRadians(2.0f);
    source.startUpdates();
    source.updatePosition(fix);
    QCOMPARE(updates.size(), 1);
    const auto position = source.lastKnownPosition();
    QCOMPARE(position.coordinate(), QGeoCoordinate(47.5, 8.5, 450));
    QCOMPARE(position.timestamp(), QDateTime::fromMSecsSinceEpoch(1788864000000LL, QTimeZone::UTC));
    QCOMPARE(position.attribute(QGeoPositionInfo::HorizontalAccuracy), static_cast<double>(fix.eph));
    QCOMPARE(position.attribute(QGeoPositionInfo::VerticalAccuracy), static_cast<double>(fix.epv));
    QCOMPARE(position.attribute(QGeoPositionInfo::GroundSpeed), 2.5);
    QCOMPARE(position.attribute(QGeoPositionInfo::VerticalSpeed), 1.5);
    QVERIFY(qAbs(position.attribute(QGeoPositionInfo::Direction) - 270) < 0.001);
    QVERIFY(qAbs(position.attribute(QGeoPositionInfo::DirectionAccuracy) - 2) < 0.001);

    fix.fix_type = sensor_gps_s::FIX_TYPE_2D;
    fix.vel_ned_valid = false;
    fix.time_utc_usec = 0;
    const auto before = QDateTime::currentDateTimeUtc();
    source.updatePosition(fix);
    const auto twoDimensional = source.lastKnownPosition();
    QVERIFY(twoDimensional.isValid());
    QVERIFY(twoDimensional.timestamp() >= before);
    QCOMPARE(twoDimensional.coordinate().type(), QGeoCoordinate::Coordinate2D);
    QVERIFY(!twoDimensional.hasAttribute(QGeoPositionInfo::VerticalAccuracy));
    QVERIFY(!twoDimensional.hasAttribute(QGeoPositionInfo::GroundSpeed));
    QVERIFY(!twoDimensional.hasAttribute(QGeoPositionInfo::Direction));

    source.stopUpdates();
    updates.clear();
    source.updatePosition(fix);
    QVERIFY(updates.isEmpty());
    source.startUpdates();
    QVERIFY(updates.isEmpty());
}

void RTKPositionSourceTest::_validatesFix_data()
{
    QTest::addColumn<sensor_gps_s>("fix");
    QTest::addColumn<bool>("valid");
    auto fix = positionFix();
    QTest::newRow("valid") << fix << true;
    const auto now =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    fix.timestamp = static_cast<uint64_t>(now) - 6000000;
    QTest::newRow("queued-stale-fix") << fix << false;
    fix.timestamp = static_cast<uint64_t>(now) + 6000000;
    QTest::newRow("future-monotonic-time") << fix << false;
    fix.timestamp = static_cast<uint64_t>(now);
    QTest::newRow("fresh-monotonic-time") << fix << true;
    fix.timestamp = 0;
    fix.latitude_deg = 0;
    fix.longitude_deg = 0;
    QTest::newRow("equator-prime-meridian") << fix << true;
    fix = positionFix();
    fix.fix_type = sensor_gps_s::FIX_TYPE_NONE;
    QTest::newRow("no-fix") << fix << false;
    fix.fix_type = sensor_gps_s::FIX_TYPE_EXTRAPOLATED;
    QTest::newRow("extrapolated") << fix << false;
    fix = positionFix();
    fix.latitude_deg = 91;
    QTest::newRow("latitude-out-of-range") << fix << false;
    fix.latitude_deg = qQNaN();
    QTest::newRow("nan-coordinate") << fix << false;
    fix = positionFix();
    fix.eph = 0;
    QTest::newRow("unset-accuracy") << fix << false;
    fix.eph = -1;
    QTest::newRow("negative-accuracy") << fix << false;
    fix.eph = qInf();
    QTest::newRow("infinite-accuracy") << fix << false;
    fix.eph = qQNaN();
    QTest::newRow("nan-accuracy") << fix << false;
}

void RTKPositionSourceTest::_validatesFix()
{
    QFETCH(sensor_gps_s, fix);
    QFETCH(bool, valid);
    RTKPositionSource source;
    source.startUpdates();
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.updatePosition(fix);
    QCOMPARE(source.lastKnownPosition().isValid(), valid);
    QCOMPARE(updates.size(), valid ? 1 : 0);
    QCOMPARE(errors.size(), valid ? 0 : 1);
}

void RTKPositionSourceTest::_requestsAndReset()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.requestUpdate(20);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(source.error(), QGeoPositionInfoSource::UpdateTimeoutError);
    source.requestUpdate();
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
    source.updatePosition(positionFix());
    QCOMPARE(updates.size(), 1);
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
    source.updatePosition(positionFix());
    QCOMPARE(updates.size(), 1);
    source.reset();
    QVERIFY(!source.lastKnownPosition().isValid());
    source.startUpdates();
    QVERIFY(!source.lastKnownPosition().isValid());
    QCOMPARE(updates.size(), 1);
    source.updatePosition(positionFix());
    QCOMPARE(updates.size(), 2);
}

void RTKPositionSourceTest::_pendingRequestKeepsDeadline()
{
    RTKPositionSource source;
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.requestUpdate(50);
    source.requestUpdate(5000);
    source.requestUpdate(-1);
    QVERIFY(errors.isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, 1000);
    QCOMPARE(source.error(), QGeoPositionInfoSource::UpdateTimeoutError);
    source.requestUpdate(-1);
    QCOMPARE(errors.size(), 2);
    source.requestUpdate(5000);
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
    source.reset();
    QCOMPARE(source.error(), QGeoPositionInfoSource::ClosedError);
    source.requestUpdate();
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
}

void RTKPositionSourceTest::_reportsLossOnceUntilRecovery()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.startUpdates();
    auto invalid = positionFix();
    invalid.fix_type = sensor_gps_s::FIX_TYPE_NONE;
    source.updatePosition(invalid);
    source.updatePosition(invalid);
    source.updatePosition(invalid);
    QCOMPARE(errors.size(), 1);
    source.startUpdates();
    QCOMPARE(source.error(), QGeoPositionInfoSource::UpdateTimeoutError);
    source.updatePosition(invalid);
    QCOMPARE(errors.size(), 1);
    source.updatePosition(positionFix());
    QCOMPARE(updates.size(), 1);
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
    source.updatePosition(invalid);
    source.updatePosition(invalid);
    QCOMPARE(errors.size(), 2);
}

void RTKPositionSourceTest::_intervalCoalescesLatestFix()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.setUpdateInterval(50);
    source.startUpdates();
    auto fix = positionFix();
    fix.time_utc_usec = 0;
    source.updatePosition(fix);
    fix.longitude_deg = 8.6;
    source.updatePosition(fix);
    fix.longitude_deg = 8.7;
    source.updatePosition(fix);
    QVERIFY(updates.isEmpty());
    QCOMPARE(source.lastKnownPosition().coordinate().longitude(), 8.7);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(updates.first().first().value<QGeoPositionInfo>().coordinate().longitude(), 8.7);
    QCOMPARE(updates.first().first().value<QGeoPositionInfo>().timestamp(), source.lastKnownPosition().timestamp());
    source.stopUpdates();
    QVERIFY(!updates.wait(100));
}

void RTKPositionSourceTest::_intervalChangesWhileStarted()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.setUpdateInterval(10000);
    source.startUpdates();
    source.updatePosition(positionFix());
    QVERIFY(updates.isEmpty());
    source.setUpdateInterval(-1);
    QCOMPARE(source.updateInterval(), 0);
    QCOMPARE(updates.size(), 1);
    source.updatePosition(positionFix());
    QCOMPARE(updates.size(), 2);
    source.setUpdateInterval(50);
    source.updatePosition(positionFix());
    source.setUpdateInterval(50);
    QCOMPARE(updates.size(), 2);
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 3, TestTimeout::mediumMs());
}

void RTKPositionSourceTest::_requestBypassesInterval()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.setUpdateInterval(10000);
    source.startUpdates();
    source.updatePosition(positionFix());
    source.requestUpdate(5000);
    auto fix = positionFix();
    fix.longitude_deg = 9;
    source.updatePosition(fix);
    QCOMPARE(updates.size(), 1);
    QCOMPARE(updates.first().first().value<QGeoPositionInfo>().coordinate().longitude(), 9);
    // The fulfilled request must not leave an older fix queued for periodic delivery.
    source.setUpdateInterval(20);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(updates.size(), 1);
}

void RTKPositionSourceTest::_stopPreservesRequest()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    source.setUpdateInterval(20);
    source.startUpdates();
    source.updatePosition(positionFix());
    source.requestUpdate(5000);
    source.stopUpdates();
    QVERIFY(!updates.wait(100));
    source.updatePosition(positionFix());
    QCOMPARE(updates.size(), 1);
    source.updatePosition(positionFix());
    QCOMPARE(updates.size(), 1);
}

void RTKPositionSourceTest::_resetDiscardsPendingUpdate()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.setUpdateInterval(20);
    source.startUpdates();
    source.updatePosition(positionFix());
    source.requestUpdate(50);
    source.reset();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(source.error(), QGeoPositionInfoSource::ClosedError);
    QVERIFY(!updates.wait(100));
    QCOMPARE(errors.size(), 1);
    QVERIFY(!source.lastKnownPosition().isValid());
    source.updatePosition(positionFix());
    QTRY_COMPARE_WITH_TIMEOUT(updates.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
}

void RTKPositionSourceTest::_intervalRejectsStaleFix()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.setUpdateInterval(750);
    source.startUpdates();
    auto fix = positionFix();
    const auto now =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    fix.timestamp = static_cast<uint64_t>(now) - 4500000;
    source.updatePosition(fix);
    QVERIFY(source.lastKnownPosition().isValid());
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QVERIFY(updates.isEmpty());
}

void RTKPositionSourceTest::_silentIntervalsReportLossOnce()
{
    RTKPositionSource source;
    QSignalSpy updates(&source, &QGeoPositionInfoSource::positionUpdated);
    QSignalSpy errors(&source, &QGeoPositionInfoSource::errorOccurred);
    source.setUpdateInterval(20);
    source.startUpdates();
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QVERIFY(!errors.wait(100));
    source.updatePosition(positionFix());
    QCOMPARE(updates.size(), 1);
    QCOMPARE(source.error(), QGeoPositionInfoSource::NoError);
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 2, TestTimeout::mediumMs());
}

UT_REGISTER_TEST(RTKPositionSourceTest, TestLabel::Unit)
