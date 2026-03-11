#include "OsmParserThreadTest.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>
#include <QtTest/QSignalSpy>

#include "OsmParserThread.h"

void OsmParserThreadTest::_testParseValidOsmFile()
{
    OsmParserThread thread;
    QSignalSpy spy(&thread, &OsmParserThread::fileParsed);
    QVERIFY(spy.isValid());

    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(tempPath(QStringLiteral("test_XXXXXX.osm")));
    QVERIFY(tmpFile.open());

    const QByteArray osmXml =
        "<?xml version='1.0' encoding='UTF-8'?>\n"
        "<osm version='0.6'>\n"
        "  <bounds minlat='47.3970' minlon='8.5440' maxlat='47.3980' maxlon='8.5460'/>\n"
        "  <node id='1' lat='47.3975' lon='8.5450'/>\n"
        "  <node id='2' lat='47.3975' lon='8.5455'/>\n"
        "  <node id='3' lat='47.3978' lon='8.5455'/>\n"
        "  <node id='4' lat='47.3978' lon='8.5450'/>\n"
        "  <way id='100'>\n"
        "    <nd ref='1'/>\n"
        "    <nd ref='2'/>\n"
        "    <nd ref='3'/>\n"
        "    <nd ref='4'/>\n"
        "    <nd ref='1'/>\n"
        "    <tag k='building' v='yes'/>\n"
        "    <tag k='building:levels' v='3'/>\n"
        "  </way>\n"
        "</osm>\n";

    tmpFile.write(osmXml);
    tmpFile.flush();
    tmpFile.close();

    // Call parseOsmFile directly on the thread object to avoid needing a running event loop
    thread._parseOsmFile(tmpFile.fileName());

    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().at(0).toBool());
    QVERIFY(!thread.mapNodes().isEmpty());
    QVERIFY(!thread.mapBuildings().isEmpty());
}

void OsmParserThreadTest::_testParseInvalidFile()
{
    OsmParserThread thread;
    QSignalSpy spy(&thread, &OsmParserThread::fileParsed);
    QVERIFY(spy.isValid());

    // Non-existent path (strip leading slash since parseOsmFile adds one on unix)
    thread._parseOsmFile("nonexistent_path_12345.osm");

    // Should not emit fileParsed(true) — either emits false or doesn't emit
    if (spy.count() > 0) {
        QVERIFY(!spy.first().at(0).toBool());
    }
}

void OsmParserThreadTest::_testParseEmptyFile()
{
    OsmParserThread thread;
    QSignalSpy spy(&thread, &OsmParserThread::fileParsed);
    QVERIFY(spy.isValid());

    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(tempPath(QStringLiteral("test_empty_XXXXXX.osm")));
    QVERIFY(tmpFile.open());
    tmpFile.close();

    thread._parseOsmFile(tmpFile.fileName());

    // Empty file: should emit fileParsed(false) since decodeFile returns false
    QCOMPARE(spy.count(), 1);
    QVERIFY(!spy.first().at(0).toBool());
}

void OsmParserThreadTest::_testBuildingTypeAppend()
{
    OsmParserThread::BuildingType_t building;

    std::vector<QGeoCoordinate> outerPoints = {QGeoCoordinate(47.0, 8.0), QGeoCoordinate(47.1, 8.1)};
    std::vector<QGeoCoordinate> innerPoints = {QGeoCoordinate(47.05, 8.05)};

    building.append(outerPoints, false);
    QCOMPARE(static_cast<int>(building.points_gps.size()), 2);
    QCOMPARE(static_cast<int>(building.points_gps_inner.size()), 0);

    building.append(innerPoints, true);
    QCOMPARE(static_cast<int>(building.points_gps_inner.size()), 1);

    std::vector<QVector2D> localOuter = {QVector2D(1, 2), QVector2D(3, 4)};
    std::vector<QVector2D> localInner = {QVector2D(5, 6)};

    building.append(localOuter, false);
    QCOMPARE(static_cast<int>(building.points_local.size()), 2);

    building.append(localInner, true);
    QCOMPARE(static_cast<int>(building.points_local_inner.size()), 1);
}

void OsmParserThreadTest::_testBuildingTypeBoundingBox()
{
    OsmParserThread::BuildingType_t building;

    QCOMPARE_FUZZY(building.bb_max.x(), -1e6f, 1.0f);
    QCOMPARE_FUZZY(building.bb_max.y(), -1e6f, 1.0f);
    QCOMPARE_FUZZY(building.bb_min.x(), 1e6f, 1.0f);
    QCOMPARE_FUZZY(building.bb_min.y(), 1e6f, 1.0f);
}

static QString _writeResourceToTempFile(const QString& tempDirectoryPath, const QString& resourcePath,
                                        const QString& templateName)
{
    QFile resFile(resourcePath);
    if (!resFile.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray data = resFile.readAll();
    resFile.close();

    QTemporaryFile tmpFile(QDir(tempDirectoryPath).filePath(templateName));
    tmpFile.setAutoRemove(false);
    if (!tmpFile.open()) {
        return {};
    }
    tmpFile.write(data);
    tmpFile.flush();
    return tmpFile.fileName();
}

void OsmParserThreadTest::_testParseMultipleBuildings()
{
    const QString absolutePath =
        _writeResourceToTempFile(tempDirPath(), QStringLiteral(":/unittest/test_buildings.osm"),
                                 QStringLiteral("multi_XXXXXX.osm"));
    QVERIFY(!absolutePath.isEmpty());
    OsmParserThread thread;
    QSignalSpy spy(&thread, &OsmParserThread::fileParsed);
    thread._parseOsmFile(absolutePath);

    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().at(0).toBool());

    // 20 nodes total
    QCOMPARE(thread.mapNodes().size(), 20);

    // After relation merging: ways 100, 101, 102 remain as individual buildings.
    // Way 200 is merged with 201 into the relation building (keyed on 200).
    // Way 201 is removed as a standalone entry.
    // So: 100, 101, 102, 200 = 4 buildings (201 merged into 200 by the relation).
    const auto& buildings = thread.mapBuildings();

    // Building A (way 100): 3 levels
    QVERIFY(buildings.contains(100));
    QCOMPARE(buildings.value(100).levels, 3.0f);
    QCOMPARE(buildings.value(100).height, 0.0f);

    // Building B (way 101): height=12
    QVERIFY(buildings.contains(101));
    QCOMPARE(buildings.value(101).height, 12.0f);

    // Building C (way 102): bungalow = 1 level
    QVERIFY(buildings.contains(102));
    QCOMPARE(buildings.value(102).levels, 1.0f);

    QFile::remove(absolutePath);
}

void OsmParserThreadTest::_testParseMultipolygonRelation()
{
    const QString absolutePath =
        _writeResourceToTempFile(tempDirPath(), QStringLiteral(":/unittest/test_buildings.osm"),
                                 QStringLiteral("mpoly_XXXXXX.osm"));
    QVERIFY(!absolutePath.isEmpty());
    OsmParserThread thread;
    QSignalSpy spy(&thread, &OsmParserThread::fileParsed);
    thread._parseOsmFile(absolutePath);

    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().at(0).toBool());

    const auto& buildings = thread.mapBuildings();

    // The relation merges way 200 (outer) with way 201 (inner).
    // Way 201 should be removed as standalone; merged building keyed on 200.
    QVERIFY(buildings.contains(200));
    QVERIFY(!buildings.contains(201));

    const auto& merged = buildings.value(200);

    // Outer ring has 5 points (closed polygon), inner has 5 points
    QVERIFY(!merged.points_gps.empty());
    QVERIFY(!merged.points_gps_inner.empty());
    QVERIFY(!merged.points_local.empty());
    QVERIFY(!merged.points_local_inner.empty());

    // Levels from relation: max of way 200 (5) and way 201 (0) = 5
    QCOMPARE(merged.levels, 5.0f);

    QFile::remove(absolutePath);
}

void OsmParserThreadTest::_benchmarkParseOsmFile()
{
    const QString absolutePath = _writeResourceToTempFile(tempDirPath(), QStringLiteral(":/unittest/map_sim_small.osm"),
                                                          QStringLiteral("bench_XXXXXX.osm"));
    QVERIFY(!absolutePath.isEmpty());
    QBENCHMARK
    {
        OsmParserThread thread;
        thread._parseOsmFile(absolutePath);
    }

    QFile::remove(absolutePath);
}

UT_REGISTER_TEST(OsmParserThreadTest, TestLabel::Unit)
