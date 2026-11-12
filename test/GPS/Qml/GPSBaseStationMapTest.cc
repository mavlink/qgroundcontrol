#include "GPSBaseStationMapTest.h"

#include <chrono>
#include <memory>

#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "QmlTesting/QmlFixtures.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"
#include "UnitsSettings.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"

using namespace std::chrono_literals;

namespace {
/// The RTCM 1005 frame of the recording Core/Protocols/fixtures/mixed.gps, and where it says the base station is.
const QByteArray STATION_FRAME = QByteArray::fromHex("d300133ed000038a58d9493c872f34109d07d6af48205ad7f7");
const QGeoCoordinate STATION_POSITION(32.0658325, 34.7738190);
/// Where the tests have the vehicle: 5 km north of the base station.
const QGeoCoordinate VEHICLE_POSITION = STATION_POSITION.atDistanceAndAzimuth(5000, 0);

/// A correction stream on a virtual clock and a vehicle that reports its RTK baseline when told to.
struct Corrections
{
    Corrections()
    {
        saved.setFactValue(SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits(),
                           UnitsSettings::HorizontalDistanceUnitsMeters);
        vehicle.gpsFactGroup()->setLiveUpdates(true);
    }

    /// Sends @a frame and lets the manager publish what it changes.
    [[nodiscard]] bool send(const QByteArray& frame)
    {
        source->submit(frame, scheduler.nowMs());
        return scheduler.advanceBy(100ms);
    }

    /// Has the vehicle report a baseline of 5 m in GPS_RTK.
    void reportBaseline()
    {
        mavlink_gps_rtk_t rtk{};
        rtk.baseline_a_mm = 3000;
        rtk.baseline_b_mm = 4000;
        mavlink_message_t message{};
        mavlink_msg_gps_rtk_encode(1, 1, &message, &rtk);
        vehicle.gpsFactGroup()->handleMessage(&vehicle, message);
    }

    /// The initial properties of a base station component for this stream and vehicle, which is at VEHICLE_POSITION.
    QVariantMap properties()
    {
        return {{QStringLiteral("corrections"), QVariant::fromValue(&manager)},
                {QStringLiteral("vehicle"), QVariant::fromValue(&vehicle)},
                {QStringLiteral("_vehicleCoordinate"), QVariant::fromValue(VEHICLE_POSITION)}};
    }

    TestFixtures::SettingsFixture saved;
    Vehicle vehicle{MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR};
    ManualScheduler scheduler;
    GPSCorrectionManager manager{nullptr, &scheduler};
    std::unique_ptr<GPSCorrectionSourceHandle> source =
        manager.openSource(GPSCorrectionSettings::Ntrip, QStringLiteral("caster/MOUNT"));
};

bool closeTo(const QGeoCoordinate& coordinate, const QGeoCoordinate& expected)
{
    return coordinate.isValid() && coordinate.distanceTo(expected) < 0.1;
}
}  // namespace

void GPSBaseStationMapTest::_mapShowsBaseStation()
{
    Corrections corrections;
    TestFixtures::QmlEngine engine;
    // The component asks its map only whether it shows satellite imagery.
    const std::unique_ptr<QObject> map =
        engine.create(QByteArrayLiteral("import QtQml\nQtObject { property bool isSatelliteMap: false }"));
    QVERIFY2(map, qPrintable(engine.lastError()));
    QVariantMap properties = corrections.properties();
    properties.insert(QStringLiteral("map"), QVariant::fromValue(map.get()));
    const std::unique_ptr<QObject> base =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("FlyView/FlyViewBaseStation.qml")), properties);
    QVERIFY2(base, qPrintable(engine.lastError()));
    auto* marker = base->findChild<QQuickItem*>(QStringLiteral("baseStationMarker"));
    auto* line = base->findChild<QQuickItem*>(QStringLiteral("baseStationLine"));
    auto* baseline = base->findChild<QQuickItem*>(QStringLiteral("baseStationBaseline"));
    QVERIFY(marker && line && baseline);
    const auto path = [line]() { return line->property("path").value<QList<QGeoCoordinate>>(); };

    // A stream shows no base station before it tells where it is.
    QVERIFY(corrections.send(GPSTest::rtcmMessage(1077, 20)));
    QCOMPARE(corrections.manager.state(), GPSCorrectionManager::State::Fresh);
    QVERIFY(!marker->property("visible").toBool());
    QVERIFY(!line->property("visible").toBool());
    QVERIFY(path().isEmpty());

    // The marker is at the base station, and the line runs from it to the vehicle.
    QVERIFY(corrections.send(STATION_FRAME));
    QVERIFY(marker->property("visible").toBool());
    QVERIFY(closeTo(marker->property("coordinate").value<QGeoCoordinate>(), STATION_POSITION));
    QVERIFY(line->property("visible").toBool());
    QCOMPARE(path().size(), 2);
    QVERIFY(closeTo(path().constFirst(), STATION_POSITION));
    QVERIFY(closeTo(path().constLast(), VEHICLE_POSITION));

    // The baseline is the one the vehicle reports, halfway along the line, and only while it reports one.
    QVERIFY(!baseline->property("visible").toBool());
    corrections.reportBaseline();
    QVERIFY(baseline->property("visible").toBool());
    QVERIFY(closeTo(baseline->property("coordinate").value<QGeoCoordinate>(),
                    STATION_POSITION.atDistanceAndAzimuth(2500, 0)));
    auto* text = baseline->property("sourceItem").value<QQuickItem*>();
    QVERIFY(text);
    QCOMPARE(text->property("text").toString(), QStringLiteral("5.00 m"));

    // A stream that is no longer selected has no base station to show.
    QVERIFY(corrections.scheduler.advanceBy(GPSCorrectionSelector::FRESHNESS_TIMEOUT + 1s));
    QCOMPARE(corrections.manager.state(), GPSCorrectionManager::State::Waiting);
    QVERIFY(!marker->property("visible").toBool());
    QVERIFY(!line->property("visible").toBool());
    QVERIFY(!baseline->property("visible").toBool());
    QVERIFY(path().isEmpty());
}

void GPSBaseStationMapTest::_geoMapShowsBaseStation()
{
    Corrections corrections;
    TestFixtures::QmlEngine engine;
    // Without a scene nothing is projected to the screen: the test reads what the items are given to draw.
    const std::unique_ptr<QObject> base = engine.create(
        TestFixtures::sourceQmlUrl(QStringLiteral("FlyView/FlyViewGeoBaseStation.qml")), corrections.properties());
    QVERIFY2(base, qPrintable(engine.lastError()));
    auto* marker = base->findChild<QQuickItem*>(QStringLiteral("geoBaseStationMarker"));
    auto* line = base->findChild<QObject*>(QStringLiteral("geoBaseStationLine"));
    auto* baseline = base->findChild<QQuickItem*>(QStringLiteral("geoBaseStationBaseline"));
    QVERIFY(marker && line && baseline);
    const auto path = [line]() {
        QList<QGeoCoordinate> coordinates;
        for (const QVariant& coordinate : line->property("coordinates").toList()) {
            coordinates.append(coordinate.value<QGeoCoordinate>());
        }
        return coordinates;
    };

    QVERIFY(corrections.send(GPSTest::rtcmMessage(1077, 20)));
    QVERIFY(!marker->property("visible").toBool());
    QVERIFY(path().isEmpty());

    QVERIFY(corrections.send(STATION_FRAME));
    QVERIFY(marker->property("visible").toBool());
    QVERIFY(closeTo(marker->property("coordinate").value<QGeoCoordinate>(), STATION_POSITION));
    QCOMPARE(path().size(), 2);
    QVERIFY(closeTo(path().constFirst(), STATION_POSITION));
    QVERIFY(closeTo(path().constLast(), VEHICLE_POSITION));
    QCOMPARE(baseline->property("text").toString(), QString());
    corrections.reportBaseline();
    QCOMPARE(baseline->property("text").toString(), QStringLiteral("5.00 m"));
    // The baseline is drawn on the line, which is not on the screen here.
    QVERIFY(!baseline->property("visible").toBool());

    QVERIFY(corrections.scheduler.advanceBy(GPSCorrectionSelector::FRESHNESS_TIMEOUT + 1s));
    QVERIFY(!marker->property("visible").toBool());
    QVERIFY(path().isEmpty());
}

void GPSBaseStationMapTest::_noLineWithoutVehicle_data()
{
    QTest::addColumn<bool>("geoMap");
    QTest::newRow("map") << false;
    QTest::newRow("geomap") << true;
}

void GPSBaseStationMapTest::_noLineWithoutVehicle()
{
    QFETCH(bool, geoMap);
    Corrections corrections;
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> map =
        engine.create(QByteArrayLiteral("import QtQml\nQtObject { property bool isSatelliteMap: false }"));
    QVERIFY2(map, qPrintable(engine.lastError()));
    // No vehicle is active: the base station is marked, and no line is drawn.
    QVariantMap properties{{QStringLiteral("corrections"), QVariant::fromValue(&corrections.manager)},
                           {QStringLiteral("vehicle"), QVariant::fromValue(static_cast<Vehicle*>(nullptr))}};
    if (!geoMap) {
        properties.insert(QStringLiteral("map"), QVariant::fromValue(map.get()));
    }
    const std::unique_ptr<QObject> base =
        engine.create(TestFixtures::sourceQmlUrl(geoMap ? QStringLiteral("FlyView/FlyViewGeoBaseStation.qml")
                                                        : QStringLiteral("FlyView/FlyViewBaseStation.qml")),
                      properties);
    QVERIFY2(base, qPrintable(engine.lastError()));
    QVERIFY(corrections.send(STATION_FRAME));
    QCOMPARE(corrections.manager.state(), GPSCorrectionManager::State::Fresh);
    auto* marker = base->findChild<QObject*>(geoMap ? QStringLiteral("geoBaseStationMarker")
                                                    : QStringLiteral("baseStationMarker"));
    auto* line =
        base->findChild<QObject*>(geoMap ? QStringLiteral("geoBaseStationLine") : QStringLiteral("baseStationLine"));
    QVERIFY(marker && line);
    const auto pathSize = [line, geoMap]() {
        return geoMap ? line->property("coordinates").toList().size()
                      : line->property("path").value<QList<QGeoCoordinate>>().size();
    };
    QVERIFY(marker->property("visible").toBool());
    QCOMPARE(pathSize(), 0);

    // A vehicle whose position is not known yet has no line either.
    QVERIFY(base->setProperty("vehicle", QVariant::fromValue(&corrections.vehicle)));
    QCOMPARE(pathSize(), 0);
    QVERIFY(marker->property("visible").toBool());
}

UT_REGISTER_TEST(GPSBaseStationMapTest, TestLabel::Unit)
