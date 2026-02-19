#include "Viewer3DMapProviderTest.h"

#include <QtTest/QSignalSpy>

#include "OsmParser.h"
#include "Viewer3DMapProvider.h"

void Viewer3DMapProviderTest::_testOsmParserIsMapProvider()
{
    OsmParser parser;
    Viewer3DMapProvider* provider = &parser;
    QVERIFY(provider != nullptr);
}

void Viewer3DMapProviderTest::_testInitialMapNotLoaded()
{
    OsmParser parser;
    QVERIFY(!parser.mapLoaded());
}

void Viewer3DMapProviderTest::_testGpsRefInitiallyInvalid()
{
    OsmParser parser;
    QVERIFY(!parser.gpsRef().isValid());
}

void Viewer3DMapProviderTest::_testBoundingBoxDefault()
{
    OsmParser parser;
    auto [bbMin, bbMax] = parser.mapBoundingBox();
    QVERIFY(!bbMin.isValid());
    QVERIFY(!bbMax.isValid());
}

void Viewer3DMapProviderTest::_testGpsRefSignalEmission()
{
    OsmParser parser;
    QSignalSpy spy(&parser, &Viewer3DMapProvider::gpsRefChanged);
    QVERIFY(spy.isValid());

    QGeoCoordinate ref(47.3977, 8.5456, 400);
    parser.setGpsRef(ref);

    QCOMPARE(spy.count(), 1);
    QGeoCoordinate emitted = spy.takeFirst().at(0).value<QGeoCoordinate>();
    QCOMPARE(emitted, ref);
}

void Viewer3DMapProviderTest::_testMapChangedSignalEmission()
{
    OsmParser parser;
    QSignalSpy spy(&parser, &Viewer3DMapProvider::mapChanged);
    QVERIFY(spy.isValid());

    // mapChanged is emitted when a parse completes; verify spy is valid
    // (actual parse requires a valid OSM file, tested in OsmParserThreadTest)
    QCOMPARE(spy.count(), 0);
}

UT_REGISTER_TEST(Viewer3DMapProviderTest, TestLabel::Unit)
