#include "RallyPointTest.h"

#include "RallyPoint.h"
#include "UnitTestCoords.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

namespace {

// A distinctive non-zero coordinate so every component can be distinguished
// on failure.
QGeoCoordinate referenceCoord()
{
    return QGeoCoordinate(47.6062, -122.3321, 125.5);
}

} // namespace

void RallyPointTest::_testCoordinateConstruction()
{
    const QGeoCoordinate coord = referenceCoord();
    RallyPoint rp(coord);

    QCOMPARE(rp.coordinate().latitude(),  coord.latitude());
    QCOMPARE(rp.coordinate().longitude(), coord.longitude());
    QCOMPARE(rp.coordinate().altitude(),  coord.altitude());

    // setCoordinate() is called from the constructor; that triggers setDirty(true).
    QVERIFY(rp.dirty());
}

void RallyPointTest::_testCopyConstruction()
{
    RallyPoint source(referenceCoord());
    RallyPoint copy(source);

    QCOMPARE(copy.coordinate(), source.coordinate());
    // Copy constructor starts with dirty=false.
    QVERIFY(!copy.dirty());
}

void RallyPointTest::_testAssignmentUpdatesAllAxes()
{
    RallyPoint source(referenceCoord());
    RallyPoint dest(QGeoCoordinate(0.0, 0.0, 0.0));

    QSignalSpy coordSpy(&dest, &RallyPoint::coordinateChanged);

    dest = source;

    // All three axes are copied through the internal Facts.
    QCOMPARE(dest.coordinate().latitude(),  source.coordinate().latitude());
    QCOMPARE(dest.coordinate().longitude(), source.coordinate().longitude());
    QCOMPARE(dest.coordinate().altitude(),  source.coordinate().altitude());

    // Assignment operator always emits coordinateChanged at least once.
    QVERIFY(coordSpy.count() >= 1);
}

void RallyPointTest::_testSetCoordinateEmitsSignalAndMarksDirty()
{
    RallyPoint rp(QGeoCoordinate(0.0, 0.0, 0.0));
    rp.setDirty(false);  // Reset the dirty flag seeded by the constructor.

    QSignalSpy coordSpy(&rp, &RallyPoint::coordinateChanged);
    QSignalSpy dirtySpy(&rp, &RallyPoint::dirtyChanged);

    rp.setCoordinate(referenceCoord());

    QCOMPARE(rp.coordinate(), referenceCoord());

    // setCoordinate emits coordinateChanged exactly once for the top-level
    // change. Per-fact change signals may also trigger internal re-emits, so
    // assert at least one without being brittle about the count.
    QVERIFY(coordSpy.count() >= 1);

    // Dirty must transition false -> true.
    QCOMPARE(dirtySpy.count(), 1);
    QCOMPARE(dirtySpy.takeFirst().at(0).toBool(), true);
    QVERIFY(rp.dirty());
}

void RallyPointTest::_testSetCoordinateIdenticalIsNoOp()
{
    RallyPoint rp(referenceCoord());
    rp.setDirty(false);

    QSignalSpy coordSpy(&rp, &RallyPoint::coordinateChanged);
    QSignalSpy dirtySpy(&rp, &RallyPoint::dirtyChanged);

    // Re-applying the exact same coordinate must not emit signals or set dirty.
    rp.setCoordinate(referenceCoord());

    QCOMPARE(coordSpy.count(), 0);
    QCOMPARE(dirtySpy.count(), 0);
    QVERIFY(!rp.dirty());
}

void RallyPointTest::_testDirtyFlagTransitions()
{
    RallyPoint rp(referenceCoord());
    rp.setDirty(false);

    QSignalSpy dirtySpy(&rp, &RallyPoint::dirtyChanged);

    // false -> false: no signal.
    rp.setDirty(false);
    QCOMPARE(dirtySpy.count(), 0);

    // false -> true.
    rp.setDirty(true);
    QCOMPARE(dirtySpy.count(), 1);
    QCOMPARE(dirtySpy.takeFirst().at(0).toBool(), true);

    // true -> true: no signal.
    rp.setDirty(true);
    QCOMPARE(dirtySpy.count(), 0);

    // true -> false.
    rp.setDirty(false);
    QCOMPARE(dirtySpy.count(), 1);
    QCOMPARE(dirtySpy.takeFirst().at(0).toBool(), false);
    QVERIFY(!rp.dirty());
}

void RallyPointTest::_testTextFieldFactsExposeThreeAxes()
{
    RallyPoint rp(referenceCoord());

    // RallyPoint exposes three facts (latitude, longitude, altitude) to QML
    // via the textFieldFacts property. Verify the list is what the QML side
    // relies on.
    const QVariant textFieldFactsVariant = rp.property("textFieldFacts");
    const QVariantList textFieldFacts = textFieldFactsVariant.toList();
    QCOMPARE(textFieldFacts.size(), 3);
}

void RallyPointTest::_testGetDefaultFactAltitudeNonNegative()
{
    // Default altitude comes from RallyPoint.FactMetaData.json. The exact
    // value isn't contractually fixed, but it must be a finite non-negative
    // double that QGC can use as a seed for new rally points.
    const double alt = RallyPoint::getDefaultFactAltitude();
    QVERIFY(qIsFinite(alt));
    QVERIFY(alt >= 0.0);
}

UT_REGISTER_TEST(RallyPointTest, TestLabel::Unit, TestLabel::MissionManager)
