#include "CityMapGeometryTest.h"

#include <QtTest/QSignalSpy>

#include "CityMapGeometry.h"
#include "OsmParser.h"

void CityMapGeometryTest::_testDefaultModelName()
{
    CityMapGeometry geo;
    QCOMPARE(geo.modelName(), QStringLiteral("city_map_default_name"));
}

void CityMapGeometryTest::_testSetModelName()
{
    CityMapGeometry geo;
    QSignalSpy spy(&geo, &CityMapGeometry::modelNameChanged);
    QVERIFY(spy.isValid());

    geo.setModelName("test_model");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(geo.modelName(), QStringLiteral("test_model"));
}

void CityMapGeometryTest::_testSetMapProviderNull()
{
    CityMapGeometry geo;
    OsmParser parser;
    geo.setMapProvider(&parser);

    QSignalSpy spy(&geo, &CityMapGeometry::mapProviderChanged);
    QVERIFY(spy.isValid());

    geo.setMapProvider(nullptr);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!geo.mapProvider());

    geo.setMapProvider(nullptr);
    QCOMPARE(spy.count(), 1);
}

void CityMapGeometryTest::_testSetMapProvider()
{
    CityMapGeometry geo;
    OsmParser parser;
    QSignalSpy spy(&geo, &CityMapGeometry::mapProviderChanged);
    QVERIFY(spy.isValid());

    geo.setMapProvider(&parser);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(geo.mapProvider(), &parser);
}

void CityMapGeometryTest::_testLoadOsmMapWithoutParser()
{
    CityMapGeometry geo;
    QVERIFY(!geo._loadOsmMap());
}

void CityMapGeometryTest::_testClearViewer()
{
    CityMapGeometry geo;
    geo._vertexData = QByteArray("test data");

    geo._clearViewer();

    QVERIFY(geo._vertexData.isEmpty());
}

void CityMapGeometryTest::_testSetOsmFilePath()
{
    CityMapGeometry geo;
    QSignalSpy spy(&geo, &CityMapGeometry::osmFilePathChanged);
    QVERIFY(spy.isValid());

    geo._setOsmFilePath(QVariant("/tmp/test.osm"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(geo.osmFilePath(), QStringLiteral("/tmp/test.osm"));
}

void CityMapGeometryTest::_testUpdateViewerWithoutParser()
{
    CityMapGeometry geo;
    geo._updateViewer();
    QVERIFY(geo._vertexData.isEmpty());
}

UT_REGISTER_TEST(CityMapGeometryTest, TestLabel::Unit)
