#include "MapPositionTrackerTest.h"

#include <QtTest/QSignalSpy>

#include "MapPositionTracker.h"

namespace {

const QGeoCoordinate kVehicleCoord(47.3977419, 8.5455938);
const QGeoCoordinate kVehicleCoord2(47.40, 8.55);
const QGeoCoordinate kGcsCoord(47.6329078, -122.0876875);
constexpr QRectF kCenterRect(100, 100, 600, 400);

}  // namespace

void MapPositionTrackerTest::_vehicleOneShot()
{
    MapPositionTracker tracker;
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setAllowVehicleLocationCenter(true);

    QVERIFY(!tracker.firstVehiclePositionReceived());
    tracker.setVehicleCoordinate(kVehicleCoord);
    QCOMPARE(centerSpy.count(), 1);
    QCOMPARE(centerSpy.last().at(0).value<QGeoCoordinate>(), kVehicleCoord);
    QCOMPARE(centerSpy.last().at(1).toBool(), true);
    QVERIFY(tracker.firstVehiclePositionReceived());

    // One-shot: further updates don't center (no follow mode active)
    tracker.setVehicleCoordinate(kVehicleCoord2);
    QCOMPARE(centerSpy.count(), 1);
}

void MapPositionTrackerTest::_gcsOneShotNoVehicle()
{
    MapPositionTracker tracker;
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setAllowGCSLocationCenter(true);

    tracker.setGcsPosition(kGcsCoord);
    QCOMPARE(centerSpy.count(), 1);
    QCOMPARE(centerSpy.last().at(0).value<QGeoCoordinate>(), kGcsCoord);
    QCOMPARE(centerSpy.last().at(1).toBool(), false);

    // One-shot: further GCS updates don't center
    tracker.setGcsPosition(QGeoCoordinate(47.64, -122.08));
    QCOMPARE(centerSpy.count(), 1);
}

void MapPositionTrackerTest::_gcsOneShotSuppressedAfterVehicle()
{
    MapPositionTracker tracker;
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setAllowGCSLocationCenter(true);
    tracker.setAllowVehicleLocationCenter(true);

    tracker.setVehicleCoordinate(kVehicleCoord);
    QCOMPARE(centerSpy.count(), 1);

    // Vehicle position already centered the map: GCS position is ignored
    tracker.setGcsPosition(kGcsCoord);
    QCOMPARE(centerSpy.count(), 1);
}

void MapPositionTrackerTest::_gcsOneShotWithValidVehicle()
{
    // A valid but not-yet-centered vehicle position suppresses GCS centering
    // unless the map keeps following the vehicle anyway
    MapPositionTracker tracker;
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setAllowGCSLocationCenter(true);
    tracker.setVehicleCoordinate(kVehicleCoord);  // no allowVehicleLocationCenter: no one-shot

    tracker.setGcsPosition(kGcsCoord);
    QCOMPARE(centerSpy.count(), 0);

    MapPositionTracker followTracker;
    QSignalSpy followCenterSpy(&followTracker, &MapPositionTracker::centerMap);
    followTracker.setAllowGCSLocationCenter(true);
    followTracker.setCenterGCSWhenVehicleValid(true);
    followTracker.setVehicleCoordinate(kVehicleCoord);

    followTracker.setGcsPosition(kGcsCoord);
    QCOMPARE(followCenterSpy.count(), 1);
    QCOMPARE(followCenterSpy.last().at(0).value<QGeoCoordinate>(), kGcsCoord);
}

void MapPositionTrackerTest::_externalFirstReceivedSuppressesOneShot()
{
    // Consumers set firstVehiclePositionReceived after e.g. a mission-items fit
    MapPositionTracker tracker;
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setAllowVehicleLocationCenter(true);
    tracker.setFirstVehiclePositionReceived(true);

    tracker.setVehicleCoordinate(kVehicleCoord);
    QCOMPARE(centerSpy.count(), 0);
}

void MapPositionTrackerTest::_activeGatesAndReevaluates()
{
    MapPositionTracker tracker;
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setAllowVehicleLocationCenter(true);
    tracker.setActive(false);

    tracker.setVehicleCoordinate(kVehicleCoord);
    QCOMPARE(centerSpy.count(), 0);

    // Activation re-runs the one-shot for the position that arrived while inactive
    tracker.setActive(true);
    QCOMPARE(centerSpy.count(), 1);
    QCOMPARE(centerSpy.last().at(0).value<QGeoCoordinate>(), kVehicleCoord);
    QCOMPARE(centerSpy.last().at(1).toBool(), true);
}

void MapPositionTrackerTest::_activeReevaluatesGcsOneShot()
{
    // GCS fix arrived while inactive and no vehicle exists: activation falls
    // through the vehicle one-shot to the GCS one
    MapPositionTracker tracker;
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setAllowGCSLocationCenter(true);
    tracker.setActive(false);

    tracker.setGcsPosition(kGcsCoord);
    QCOMPARE(centerSpy.count(), 0);

    tracker.setActive(true);
    QCOMPARE(centerSpy.count(), 1);
    QCOMPARE(centerSpy.last().at(0).value<QGeoCoordinate>(), kGcsCoord);
    QCOMPARE(centerSpy.last().at(1).toBool(), false);
}

void MapPositionTrackerTest::_allowEnabledAfterPositionReevaluates()
{
    // Enabling the allow flags after positions are already valid must not
    // leave the one-shots dormant (QML binding order is unspecified)
    MapPositionTracker vehicleTracker;
    QSignalSpy vehicleCenterSpy(&vehicleTracker, &MapPositionTracker::centerMap);
    vehicleTracker.setVehicleCoordinate(kVehicleCoord);
    QCOMPARE(vehicleCenterSpy.count(), 0);

    vehicleTracker.setAllowVehicleLocationCenter(true);
    QCOMPARE(vehicleCenterSpy.count(), 1);
    QCOMPARE(vehicleCenterSpy.last().at(0).value<QGeoCoordinate>(), kVehicleCoord);
    QCOMPARE(vehicleCenterSpy.last().at(1).toBool(), true);

    MapPositionTracker gcsTracker;
    QSignalSpy gcsCenterSpy(&gcsTracker, &MapPositionTracker::centerMap);
    gcsTracker.setGcsPosition(kGcsCoord);
    QCOMPARE(gcsCenterSpy.count(), 0);

    gcsTracker.setAllowGCSLocationCenter(true);
    QCOMPARE(gcsCenterSpy.count(), 1);
    QCOMPARE(gcsCenterSpy.last().at(0).value<QGeoCoordinate>(), kGcsCoord);
    QCOMPARE(gcsCenterSpy.last().at(1).toBool(), false);
}

void MapPositionTrackerTest::_hardFollow()
{
    MapPositionTracker tracker;
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setKeepVehicleCentered(true);

    tracker.setVehicleCoordinate(kVehicleCoord);
    QCOMPARE(centerSpy.count(), 1);
    QCOMPARE(centerSpy.last().at(1).toBool(), false);

    tracker.setVehicleCoordinate(kVehicleCoord2);
    QCOMPARE(centerSpy.count(), 2);
    QCOMPARE(centerSpy.last().at(0).value<QGeoCoordinate>(), kVehicleCoord2);
}

void MapPositionTrackerTest::_interactionPausesAndResumes()
{
    MapPositionTracker tracker;
    tracker.setResumeDelayMsForTest(50);
    QSignalSpy centerSpy(&tracker, &MapPositionTracker::centerMap);
    tracker.setKeepVehicleCentered(true);

    tracker.setUserInteracting(true);
    tracker.setVehicleCoordinate(kVehicleCoord);
    QCOMPARE(centerSpy.count(), 0);

    // Cooldown still pauses following right after the interaction ends
    tracker.setUserInteracting(false);
    tracker.setVehicleCoordinate(kVehicleCoord2);
    QCOMPARE(centerSpy.count(), 0);

    // Cooldown expiry recenters on the vehicle
    QTRY_COMPARE_WITH_TIMEOUT(centerSpy.count(), 1, 5000);
    QCOMPARE(centerSpy.last().at(0).value<QGeoCoordinate>(), kVehicleCoord2);
    QCOMPARE(centerSpy.last().at(1).toBool(), false);
}

void MapPositionTrackerTest::_insetFollow()
{
    MapPositionTracker tracker;
    QSignalSpy recenterSpy(&tracker, &MapPositionTracker::recenterVehicleTo);
    tracker.setVehicleCoordinate(kVehicleCoord);
    tracker.setFirstVehiclePositionReceived(true);

    // Inside the center rect: no recenter
    tracker.evaluateInsetFollow(QPointF(400, 300), kCenterRect, {});
    QCOMPARE(recenterSpy.count(), 0);

    // Outside the center rect: recenter to its center
    tracker.evaluateInsetFollow(QPointF(50, 300), kCenterRect, {});
    QCOMPARE(recenterSpy.count(), 1);
    QCOMPARE(recenterSpy.last().at(0).toPointF(), kCenterRect.center());

    // Inside the center rect but under a corner UI element: recenter
    const QVariantList cornerRects{QVariant(QRectF(380, 280, 40, 40))};
    tracker.evaluateInsetFollow(QPointF(400, 300), kCenterRect, cornerRects);
    QCOMPARE(recenterSpy.count(), 2);
}

void MapPositionTrackerTest::_insetFollowGates()
{
    MapPositionTracker tracker;
    QSignalSpy recenterSpy(&tracker, &MapPositionTracker::recenterVehicleTo);
    const QPointF outside(50, 300);

    // No vehicle position yet
    tracker.evaluateInsetFollow(outside, kCenterRect, {});
    QCOMPARE(recenterSpy.count(), 0);

    tracker.setVehicleCoordinate(kVehicleCoord);

    // First position not marked received
    tracker.evaluateInsetFollow(outside, kCenterRect, {});
    QCOMPARE(recenterSpy.count(), 0);

    tracker.setFirstVehiclePositionReceived(true);

    // Hard follow owns centering
    tracker.setKeepVehicleCentered(true);
    tracker.evaluateInsetFollow(outside, kCenterRect, {});
    QCOMPARE(recenterSpy.count(), 0);
    tracker.setKeepVehicleCentered(false);

    // User interaction pauses inset follow
    tracker.setUserInteracting(true);
    tracker.evaluateInsetFollow(outside, kCenterRect, {});
    QCOMPARE(recenterSpy.count(), 0);
    tracker.setUserInteracting(false);
    // Still paused during the resume cooldown
    tracker.evaluateInsetFollow(outside, kCenterRect, {});
    QCOMPARE(recenterSpy.count(), 0);

    // A running recenter animation blocks re-evaluation
    MapPositionTracker animTracker;
    QSignalSpy animRecenterSpy(&animTracker, &MapPositionTracker::recenterVehicleTo);
    animTracker.setVehicleCoordinate(kVehicleCoord);
    animTracker.setFirstVehiclePositionReceived(true);
    animTracker.setAnimating(true);
    animTracker.evaluateInsetFollow(outside, kCenterRect, {});
    QCOMPARE(animRecenterSpy.count(), 0);
    animTracker.setAnimating(false);
    animTracker.evaluateInsetFollow(outside, kCenterRect, {});
    QCOMPARE(animRecenterSpy.count(), 1);
}

UT_REGISTER_TEST_LIGHTWEIGHT(MapPositionTrackerTest, TestLabel::Unit)
