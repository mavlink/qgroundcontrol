#include "RallyPointTest.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "MAVLinkEnums.h"
#include "QGroundControlQmlGlobal.h"
#include "RallyPoint.h"
#include "TerrainQuery.h"
#include "UnitTestCoords.h"

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

    // Constructing from a coordinate is not an edit
    QVERIFY(!rp.dirty());
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

void RallyPointTest::_testFactsExposeEachAxis()
{
    RallyPoint rp(referenceCoord());

    // The QML editors bind their text fields to these Facts
    QCOMPARE(rp.latitude()->rawValue().toDouble(), referenceCoord().latitude());
    QCOMPARE(rp.longitude()->rawValue().toDouble(), referenceCoord().longitude());
    QCOMPARE(rp.altitude()->rawValue().toDouble(), referenceCoord().altitude());
    QCOMPARE(rp.property("altitude").value<Fact*>(), rp.altitude());
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

void RallyPointTest::_testMavFrameMapping_data()
{
    QTest::addColumn<int>("mavFrame");
    QTest::addColumn<int>("expectedAltitudeFrame");
    QTest::addColumn<int>("expectedUploadFrame");

    QTest::newRow("global") << int(MAV_FRAME_GLOBAL) << int(QGroundControlQmlGlobal::AltitudeFrameAbsolute)
                            << int(MAV_FRAME_GLOBAL);
    QTest::newRow("global int") << int(MAV_FRAME_GLOBAL_INT) << int(QGroundControlQmlGlobal::AltitudeFrameAbsolute)
                                << int(MAV_FRAME_GLOBAL);
    QTest::newRow("relative") << int(MAV_FRAME_GLOBAL_RELATIVE_ALT)
                              << int(QGroundControlQmlGlobal::AltitudeFrameRelative)
                              << int(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QTest::newRow("relative int") << int(MAV_FRAME_GLOBAL_RELATIVE_ALT_INT)
                                  << int(QGroundControlQmlGlobal::AltitudeFrameRelative)
                                  << int(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QTest::newRow("terrain") << int(MAV_FRAME_GLOBAL_TERRAIN_ALT) << int(QGroundControlQmlGlobal::AltitudeFrameTerrain)
                             << int(MAV_FRAME_GLOBAL_TERRAIN_ALT);
    QTest::newRow("terrain int") << int(MAV_FRAME_GLOBAL_TERRAIN_ALT_INT)
                                 << int(QGroundControlQmlGlobal::AltitudeFrameTerrain)
                                 << int(MAV_FRAME_GLOBAL_TERRAIN_ALT);
    QTest::newRow("unsupported frame is relative")
        << int(MAV_FRAME_LOCAL_NED) << int(QGroundControlQmlGlobal::AltitudeFrameRelative)
        << int(MAV_FRAME_GLOBAL_RELATIVE_ALT);
}

void RallyPointTest::_testMavFrameMapping()
{
    QFETCH(int, mavFrame);
    QFETCH(int, expectedAltitudeFrame);
    QFETCH(int, expectedUploadFrame);

    const QGroundControlQmlGlobal::AltitudeFrame altitudeFrame =
        RallyPoint::altitudeFrameFromMavFrame(static_cast<MAV_FRAME>(mavFrame));

    QCOMPARE(int(altitudeFrame), expectedAltitudeFrame);
    QCOMPARE(int(RallyPoint::mavFrame(altitudeFrame)), expectedUploadFrame);
}

void RallyPointTest::_testAltitudeFrameSurvivesConstructionCopyAndAssignment()
{
    RallyPoint source(referenceCoord(), QGroundControlQmlGlobal::AltitudeFrameAbsolute);
    QCOMPARE(int(source.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameAbsolute));
    QVERIFY(!source.dirty());

    RallyPoint copy(source);
    QCOMPARE(int(copy.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameAbsolute));

    RallyPoint dest(referenceCoord());
    QCOMPARE(int(dest.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameRelative));
    QSignalSpy frameSpy(&dest, &RallyPoint::altitudeFrameChanged);
    dest = source;
    QCOMPARE(int(dest.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameAbsolute));
    QCOMPARE(frameSpy.count(), 1);
    // Only the frame differs, which is still an edit of the destination
    QVERIFY(dest.dirty());
}

void RallyPointTest::_testSetAltitudeFrameMarksDirty()
{
    RallyPoint rp(referenceCoord());
    QSignalSpy frameSpy(&rp, &RallyPoint::altitudeFrameChanged);

    rp.setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameRelative);
    QCOMPARE(frameSpy.count(), 0);
    QVERIFY(!rp.dirty());

    rp.setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameAbsolute);
    QCOMPARE(frameSpy.count(), 1);
    QVERIFY(rp.dirty());
}

void RallyPointTest::_testChangeAltitudeFrameResetsAltitude_data()
{
    QTest::addColumn<int>("startFrame");
    QTest::addColumn<int>("frame");
    QTest::addColumn<double>("seedAltitude");
    QTest::addColumn<double>("expectedAltitude");

    const int absolute = QGroundControlQmlGlobal::AltitudeFrameAbsolute;
    const int relative = QGroundControlQmlGlobal::AltitudeFrameRelative;
    const int terrain = QGroundControlQmlGlobal::AltitudeFrameTerrain;

    QTest::newRow("relative defaults to 0") << absolute << relative << 0.0 << 0.0;
    QTest::newRow("relative with seed") << absolute << relative << 40.0 << 40.0;
    QTest::newRow("terrain with seed") << absolute << terrain << 40.0 << 40.0;
    // Without a terrain request AMSL is a plain frame switch, which is what ArduPilot flight altitudes need
    QTest::newRow("absolute without terrain seeds the altitude") << relative << absolute << 40.0 << 40.0;
}

void RallyPointTest::_testChangeAltitudeFrameResetsAltitude()
{
    QFETCH(int, startFrame);
    QFETCH(int, frame);
    QFETCH(double, seedAltitude);
    QFETCH(double, expectedAltitude);

    // A number in one frame is wrong in the other, so the altitude is reseeded instead of carried over
    RallyPoint rp(referenceCoord(), static_cast<QGroundControlQmlGlobal::AltitudeFrame>(startFrame));

    rp.changeAltitudeFrame(static_cast<QGroundControlQmlGlobal::AltitudeFrame>(frame), seedAltitude);

    QCOMPARE(int(rp.altitudeFrame()), frame);
    QCOMPARE(rp.coordinate().altitude(), expectedAltitude);
    QVERIFY(!rp.terrainQueryPending());
    QVERIFY(!rp._followTerrain);
}

void RallyPointTest::_answerTerrainQuery(RallyPoint& rp, bool success, const QList<double>& heights)
{
    rp._lastTerrainQueryLatitude = rp.coordinate().latitude();
    rp._lastTerrainQueryLongitude = rp.coordinate().longitude();
    rp._terrainDataReceived(success, heights);
}

void RallyPointTest::_testTerrainAltitudeSeedsAbsoluteFrame()
{
    RallyPoint rp(referenceCoord());

    _answerTerrainQuery(rp, true, {321.5});

    QCOMPARE(int(rp.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameAbsolute));
    QCOMPARE(rp.coordinate().altitude(), 321.5);
    QVERIFY(!rp.terrainQueryFailed());
    QVERIFY(rp.dirty());
    // Seeding is not a user edit, the point keeps following the terrain as it moves
    QVERIFY(rp._followTerrain);
}

void RallyPointTest::_testTerrainFailureLeavesPointUnchanged()
{
    // Without terrain data the point must not end up in AMSL with a made up altitude
    RallyPoint rp(referenceCoord());
    QSignalSpy failedSpy(&rp, &RallyPoint::terrainQueryFailedChanged);

    _answerTerrainQuery(rp, false, {});

    QCOMPARE(int(rp.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameRelative));
    QCOMPARE(rp.coordinate().altitude(), referenceCoord().altitude());
    QVERIFY(rp.terrainQueryFailed());
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(!rp._followTerrain);

    // A later success clears the failure
    _answerTerrainQuery(rp, true, {10.0});
    QVERIFY(!rp.terrainQueryFailed());
}

void RallyPointTest::_testEditingAltitudeStopsFollowingTerrain()
{
    RallyPoint rp(referenceCoord());
    _answerTerrainQuery(rp, true, {321.5});
    QVERIFY(rp._followTerrain);

    rp.altitude()->setRawValue(400.0);

    QVERIFY(!rp._followTerrain);
    QCOMPARE(rp.coordinate().altitude(), 400.0);
}

void RallyPointTest::_testEditingAltitudeDiscardsPendingTerrainAnswer()
{
    RallyPoint rp(referenceCoord());

    // Stands in for the request a frame change to AMSL has in flight
    TerrainAtCoordinateQuery pendingQuery(false /* autoDelete */);
    connect(&pendingQuery, &TerrainAtCoordinateQuery::terrainDataReceived, &rp, &RallyPoint::_terrainDataReceived);
    rp._terrainQuery = &pendingQuery;

    rp.altitude()->setRawValue(400.0);
    pendingQuery.signalTerrainData(true, {321.5});

    QCOMPARE(int(rp.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameRelative));
    QCOMPARE(rp.coordinate().altitude(), 400.0);
}

void RallyPointTest::_testTerrainAnswerForMovedPointIsDiscarded()
{
    RallyPoint rp(referenceCoord());
    rp._lastTerrainQueryLatitude = referenceCoord().latitude();
    rp._lastTerrainQueryLongitude = referenceCoord().longitude();

    const QGeoCoordinate movedCoord(referenceCoord().latitude() + 0.001, referenceCoord().longitude(), 125.5);
    rp.setCoordinate(movedCoord);
    rp._terrainDataReceived(true, {321.5});

    QCOMPARE(int(rp.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameRelative));
    QCOMPARE(rp.coordinate().altitude(), referenceCoord().altitude());
    QVERIFY(!rp._followTerrain);
    // A new request for the new position replaced the stale one
    QVERIFY(rp._terrainQuery);
    QCOMPARE(rp._lastTerrainQueryLatitude, movedCoord.latitude());
}

void RallyPointTest::_testUseTerrainAltitudeSupersedesPendingMoveRequest()
{
    RallyPoint rp(referenceCoord());
    _answerTerrainQuery(rp, true, {321.5});
    rp.setCoordinate(QGeoCoordinate(referenceCoord().latitude() + 0.001, referenceCoord().longitude(), 321.5));
    QVERIFY(rp._terrainQueryTimer.isActive());

    rp.useTerrainAltitude();

    QVERIFY(!rp._terrainQueryTimer.isActive());
    QVERIFY(rp._terrainQuery);
}

void RallyPointTest::_testTerrainQueryPending()
{
    RallyPoint rp(referenceCoord());
    QVERIFY(!rp.terrainQueryPending());

    rp.useTerrainAltitude();
    QVERIFY(rp.terrainQueryPending());

    // A failure must not leave the point pending, or save and upload would be blocked for good
    _answerTerrainQuery(rp, false, {});
    QVERIFY(!rp.terrainQueryPending());

    // A following point is pending again once it moves and a new request is due
    _answerTerrainQuery(rp, true, {321.5});
    QVERIFY(!rp.terrainQueryPending());
    rp.setCoordinate(QGeoCoordinate(referenceCoord().latitude() + 0.001, referenceCoord().longitude(), 321.5));
    QVERIFY(rp.terrainQueryPending());

    // Taking over the altitude cancels the request
    rp.altitude()->setRawValue(400.0);
    QVERIFY(!rp.terrainQueryPending());
}

void RallyPointTest::_testSelectingFrameCancelsPendingTerrainAnswer()
{
    RallyPoint rp(referenceCoord());

    // Stands in for the request that selecting AMSL has in flight while the frame is still Relative
    TerrainAtCoordinateQuery pendingQuery(false /* autoDelete */);
    connect(&pendingQuery, &TerrainAtCoordinateQuery::terrainDataReceived, &rp, &RallyPoint::_terrainDataReceived);
    rp._terrainQuery = &pendingQuery;

    rp.changeAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameRelative, 0);
    pendingQuery.signalTerrainData(true, {321.5});

    QCOMPARE(int(rp.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameRelative));
    QCOMPARE(rp.coordinate().altitude(), referenceCoord().altitude());
    QVERIFY(!rp.terrainQueryPending());
}

void RallyPointTest::_testExplicitAbsoluteRequestSwitchesFrameWhenTerrainFails()
{
    RallyPoint rp(referenceCoord());

    rp.changeAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameAbsolute, 0, true /* seedFromTerrain */);
    QVERIFY(rp.terrainQueryPending());
    _answerTerrainQuery(rp, false, {});

    // Without terrain data the user can still enter an AMSL altitude
    QCOMPARE(int(rp.altitudeFrame()), int(QGroundControlQmlGlobal::AltitudeFrameAbsolute));
    QCOMPARE(rp.coordinate().altitude(), referenceCoord().altitude());
    QVERIFY(rp.terrainQueryFailed());
    QVERIFY(!rp._followTerrain);

    // Typing the altitude takes over and dismisses the warning
    rp.altitude()->setRawValue(250.0);
    QVERIFY(!rp.terrainQueryFailed());
}

UT_REGISTER_TEST(RallyPointTest, TestLabel::Unit, TestLabel::MissionManager)
