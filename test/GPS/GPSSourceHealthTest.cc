#include "GPSSourceHealthTest.h"

#include <memory>

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include "GPSSourceHealth.h"

namespace {
QGeoPositionInfo position()
{
    QGeoPositionInfo result(QGeoCoordinate(0, 0, 500), QDateTime::fromMSecsSinceEpoch(1000));
    result.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 5);
    result.setAttribute(QGeoPositionInfo::VerticalAccuracy, 2);
    result.setAttribute(QGeoPositionInfo::GroundSpeed, 1);
    result.setAttribute(QGeoPositionInfo::Direction, 360);
    return result;
}
}  // namespace

void GPSSourceHealthTest::_normalizesObservation_data()
{
    QTest::addColumn<QGeoPositionInfo>("fix");
    QTest::addColumn<bool>("usable");
    QTest::addColumn<bool>("altitude");
    QTest::addColumn<bool>("heading");
    QTest::newRow("origin-and-old-receiver-clock") << position() << true << true << true;
    const auto row = [](const char* name, QGeoPositionInfo::Attribute attribute, double value, bool usable,
                        bool altitude, bool heading) {
        auto fix = position();
        fix.setAttribute(attribute, value);
        QTest::newRow(name) << fix << usable << altitude << heading;
    };
    row("inaccurate", QGeoPositionInfo::HorizontalAccuracy, 101, false, false, false);
    row("unknown-accuracy", QGeoPositionInfo::HorizontalAccuracy, qQNaN(), false, false, false);
    row("zero-vertical-accuracy", QGeoPositionInfo::VerticalAccuracy, 0, true, false, true);
    row("negative-vertical-accuracy", QGeoPositionInfo::VerticalAccuracy, -1, true, false, true);
    row("nan-vertical-accuracy", QGeoPositionInfo::VerticalAccuracy, qQNaN(), true, false, true);
    row("poor-vertical-accuracy", QGeoPositionInfo::VerticalAccuracy, 11, true, false, true);
    row("stationary", QGeoPositionInfo::GroundSpeed, 0, true, true, false);
    row("negative-direction-accuracy", QGeoPositionInfo::DirectionAccuracy, -1, true, true, false);
    row("poor-direction-accuracy", QGeoPositionInfo::DirectionAccuracy, 31, true, true, false);
    auto fix = position();
    fix.setCoordinate(QGeoCoordinate(0, 0));
    QTest::newRow("2d-with-vertical-accuracy") << fix << true << false << true;
    fix.removeAttribute(QGeoPositionInfo::HorizontalAccuracy);
    QTest::newRow("missing-accuracy") << fix << false << false << false;
}

void GPSSourceHealthTest::_normalizesObservation()
{
    QFETCH(QGeoPositionInfo, fix);
    QFETCH(bool, usable);
    QFETCH(bool, altitude);
    QFETCH(bool, heading);
    GPSSourceHealth health;
    health.updatePosition(fix);
    QCOMPARE(health.usable(), usable);
    QCOMPARE(health.coordinate().isValid(), usable);
    QCOMPARE(health.coordinate().type() == QGeoCoordinate::Coordinate3D, altitude);
    QCOMPARE(qIsFinite(health.observation().heading()), heading);
    if (heading) {
        QCOMPARE(health.observation().heading(), 0);
    }
}

void GPSSourceHealthTest::_ageAndRecovery()
{
    GPSSourceHealth health;
    health._freshnessTimeoutMs = 100;
    QCOMPARE(health.state(), GPSSourceHealth::NoData);
    QVERIFY(!health._positionTimer.isActive());
    const auto before = QDateTime::currentDateTimeUtc();
    health.updatePosition(position(), 60);
    QVERIFY(health.usable());
    QVERIFY(health.receivedAt() >= before.addMSecs(-60));
    QVERIFY(health.receivedAt() <= QDateTime::currentDateTimeUtc().addMSecs(-60));
    QVERIFY(health._positionTimer.remainingTime() <= 40);
    QTRY_COMPARE_WITH_TIMEOUT(health.state(), GPSSourceHealth::Stale, TestTimeout::mediumMs());
    QVERIFY(!health.coordinate().isValid());
    QVERIFY(qIsNaN(health.horizontalAccuracy()));
    health.updatePosition(position());
    QVERIFY(health.usable());
    health.updatePosition(position(), 100);
    QCOMPARE(health.state(), GPSSourceHealth::Stale);
    QVERIFY(!health._positionTimer.isActive());
    health.updatePosition(position(), -1);
    QCOMPARE(health.state(), GPSSourceHealth::Invalid);
    health.updatePosition(position());
    health.invalidatePosition();
    QCOMPARE(health.state(), GPSSourceHealth::Invalid);
    health.reset();
    QCOMPARE(health.state(), GPSSourceHealth::NoData);
    QVERIFY(!health.receivedAt().isValid());
    QVERIFY(!health.observation().position.isValid());
    QVERIFY(!health._positionTimer.isActive());
}

void GPSSourceHealthTest::_independentSatelliteExpiry()
{
    GPSSourceHealth health;
    health._freshnessTimeoutMs = 100;
    health.updatePosition(position());
    health.updateSatellitesInView(8);
    health.updateSatellitesInUse(5);
    QTimer views;
    connect(&views, &QTimer::timeout, &health, [&]() { health.updateSatellitesInView(8); });
    views.start(20);
    QTRY_COMPARE_WITH_TIMEOUT(health.satellitesInUseCount(), -1, TestTimeout::mediumMs());
    QCOMPARE(health.satellitesInViewCount(), 8);
    QCOMPARE(health.state(), GPSSourceHealth::Stale);
    views.stop();
    QTRY_COMPARE_WITH_TIMEOUT(health.satellitesInViewCount(), -1, TestTimeout::mediumMs());
    health.updateSatellitesInView(0);
    QCOMPARE(health.satellitesInViewCount(), 0);
    health.updateSatellitesInUse(0);
    QCOMPARE(health.satellitesInUseCount(), 0);
    health.updateSatellitesInView(8, 100);
    QCOMPARE(health.satellitesInViewCount(), -1);
    health.updateSatellitesInUse(5, -1);
    QCOMPARE(health.satellitesInUseCount(), -1);
    health.updateSatellitesInUse(5);
    health.reset();
    QCOMPARE(health.satellitesInUseCount(), -1);
    QVERIFY(!health._satellitesInUseTimer.isActive());
}

void GPSSourceHealthTest::_settingsStatus()
{
    GPSSourceHealth health;
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import QGroundControl.AppSettings
        GpsSourceStatus {
            fixStatusObjectName: "fix"
            satelliteStatusObjectName: "satellites"
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> panel(component.createWithInitialProperties({{"health", QVariant::fromValue(&health)}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* fix = panel->findChild<QObject*>(QStringLiteral("fix"));
    auto* satellites = panel->findChild<QObject*>(QStringLiteral("satellites"));
    QVERIFY(fix);
    QVERIFY(satellites);
    QVERIFY(fix->property("text").toString().contains(QStringLiteral("waiting")));
    health.updatePosition(position());
    QVERIFY(fix->property("text").toString().contains(QStringLiteral("5.0 m")));
    health.updateSatellitesInView(8);
    health.updateSatellitesInUse(0);
    QCOMPARE(satellites->property("text").toString(), QStringLiteral("Satellites: 0 in use / 8 in view"));
    health.updatePosition(position(), GPSSourceHealth::FRESHNESS_TIMEOUT_MS);
    QCOMPARE(fix->property("text").toString(), QStringLiteral("Position: stale"));
    health.invalidatePosition();
    QCOMPARE(fix->property("text").toString(), QStringLiteral("Position: no usable fix"));
    health.reset();
    QCOMPARE(satellites->property("text").toString(), QStringLiteral("Satellites: Unknown in use / Unknown in view"));
}

UT_REGISTER_TEST(GPSSourceHealthTest, TestLabel::Unit)
