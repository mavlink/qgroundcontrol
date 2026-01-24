#include "PerformanceBenchmarkTest.h"
#include "MissionItem.h"
#include "QGCMAVLink.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QElapsedTimer>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

void PerformanceBenchmarkTest::init()
{
    OfflineTest::init();
}

void PerformanceBenchmarkTest::cleanup()
{
    OfflineTest::cleanup();
}

void PerformanceBenchmarkTest::_benchmarkCoordinateCreation()
{
    int i = 0;
    QBENCHMARK {
        QGeoCoordinate coord(47.0 + (i * 0.0001), 8.0 + (i * 0.0001), 100.0 + i);
        Q_UNUSED(coord.isValid());
        ++i;
    }
}

void PerformanceBenchmarkTest::_benchmarkCoordinateDistance()
{
    QGeoCoordinate coord1(47.0, 8.0, 100.0);
    QGeoCoordinate coord2(47.1, 8.1, 150.0);

    double distance = 0;
    QBENCHMARK {
        distance = coord1.distanceTo(coord2);
    }

    // Verify distance is calculated correctly
    QCOMPARE_GT(distance, 0.0);
}

void PerformanceBenchmarkTest::_benchmarkCoordinateOffset()
{
    QGeoCoordinate coord(47.0, 8.0, 100.0);

    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < kLargeIterations; ++i) {
        QGeoCoordinate offset = coord.atDistanceAndAzimuth(100.0 + i, i % 360);
        Q_UNUSED(offset.isValid());
    }

    const qint64 elapsed = timer.elapsed();
    qDebug() << "Coordinate offset:" << kLargeIterations << "iterations in" << elapsed << "ms"
             << "(" << (static_cast<double>(elapsed) / kLargeIterations * 1000) << "us/op)";

    QVERIFY(elapsed < 10000);
}

void PerformanceBenchmarkTest::_benchmarkJsonParsing()
{
    // Create a sample JSON document similar to a plan file
    QJsonObject planObject;
    planObject["fileType"] = "Plan";
    planObject["version"] = 1;

    QJsonArray missionItems;
    for (int i = 0; i < 100; ++i) {
        QJsonObject item;
        item["type"] = "SimpleItem";
        item["coordinate"] = QJsonArray{47.0 + i * 0.001, 8.0 + i * 0.001, 100.0};
        item["command"] = 16; // MAV_CMD_NAV_WAYPOINT
        missionItems.append(item);
    }
    planObject["mission"] = missionItems;

    QJsonDocument doc(planObject);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < kIterations; ++i) {
        QJsonParseError error;
        QJsonDocument parsed = QJsonDocument::fromJson(jsonData, &error);
        QVERIFY(error.error == QJsonParseError::NoError);
        Q_UNUSED(parsed.object());
    }

    const qint64 elapsed = timer.elapsed();
    qDebug() << "JSON parsing (100 items):" << kIterations << "iterations in" << elapsed << "ms"
             << "(" << (static_cast<double>(elapsed) / kIterations) << "ms/op)";

    QVERIFY(elapsed < 10000);
}

void PerformanceBenchmarkTest::_benchmarkJsonSerialization()
{
    QJsonObject planObject;
    planObject["fileType"] = "Plan";
    planObject["version"] = 1;

    QJsonArray missionItems;
    for (int i = 0; i < 100; ++i) {
        QJsonObject item;
        item["type"] = "SimpleItem";
        item["coordinate"] = QJsonArray{47.0 + i * 0.001, 8.0 + i * 0.001, 100.0};
        item["command"] = 16;
        missionItems.append(item);
    }
    planObject["mission"] = missionItems;

    QJsonDocument doc(planObject);

    QElapsedTimer timer;
    timer.start();

    qint64 totalBytes = 0;
    for (int i = 0; i < kIterations; ++i) {
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        totalBytes += jsonData.size();
    }

    const qint64 elapsed = timer.elapsed();
    qDebug() << "JSON serialization (100 items):" << kIterations << "iterations in" << elapsed << "ms"
             << "(" << (static_cast<double>(elapsed) / kIterations) << "ms/op)";

    QVERIFY(totalBytes > 0);
    QVERIFY(elapsed < 10000);
}

void PerformanceBenchmarkTest::_benchmarkMissionItemCreation()
{
    QElapsedTimer timer;
    timer.start();

    QList<MissionItem*> items;
    for (int i = 0; i < kIterations; ++i) {
        MissionItem* item = new MissionItem(
            i,                          // sequence number
            MAV_CMD_NAV_WAYPOINT,       // command
            MAV_FRAME_GLOBAL_RELATIVE_ALT,
            0, 0, 0, 0,                 // params 1-4
            47.0 + i * 0.001,           // latitude
            8.0 + i * 0.001,            // longitude
            100.0,                      // altitude
            true,                       // autocontinue
            false                       // isCurrentItem
        );
        items.append(item);
    }

    const qint64 elapsed = timer.elapsed();
    qDebug() << "MissionItem creation:" << kIterations << "items in" << elapsed << "ms"
             << "(" << (static_cast<double>(elapsed) / kIterations * 1000) << "us/item)";

    QCOMPARE(items.count(), kIterations);

    // Cleanup
    qDeleteAll(items);

    QVERIFY(elapsed < 5000);
}

void PerformanceBenchmarkTest::_benchmarkWaypointListManipulation()
{
    QList<QGeoCoordinate> waypoints;

    QElapsedTimer timer;
    timer.start();

    // Append operations
    for (int i = 0; i < kIterations; ++i) {
        waypoints.append(QGeoCoordinate(47.0 + i * 0.001, 8.0 + i * 0.001, 100.0));
    }

    const qint64 appendTime = timer.elapsed();

    // Insert at beginning (expensive operation)
    timer.restart();
    for (int i = 0; i < 100; ++i) {
        waypoints.prepend(QGeoCoordinate(46.0 + i * 0.001, 7.0 + i * 0.001, 50.0));
    }

    const qint64 prependTime = timer.elapsed();

    // Random access
    timer.restart();
    double totalLat = 0;
    for (int i = 0; i < kIterations; ++i) {
        totalLat += waypoints[i % waypoints.size()].latitude();
    }

    const qint64 accessTime = timer.elapsed();

    // Remove operations
    timer.restart();
    while (waypoints.size() > 100) {
        waypoints.removeLast();
    }

    const qint64 removeTime = timer.elapsed();

    qDebug() << "Waypoint list operations:";
    qDebug() << "  Append" << kIterations << "items:" << appendTime << "ms";
    qDebug() << "  Prepend 100 items:" << prependTime << "ms";
    qDebug() << "  Random access" << kIterations << "times:" << accessTime << "ms";
    qDebug() << "  Remove" << (kIterations - 100) << "items:" << removeTime << "ms";

    QVERIFY(totalLat != 0);
    QVERIFY(appendTime < 5000);
}
