#include "TrajectoryPointsTest.h"

#include <QtCore/QMetaObject>
#include <QtCore/QVariant>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include <cmath>

#include "Fact.h"
#include "TrajectoryPoints.h"
#include "Vehicle.h"

namespace {

const QGeoCoordinate kStart(47.397742, 8.545594, 100.0);

// Drives the private slot directly: connecting through Vehicle::coordinateChanged
// would require simulating position messages
void sendPosition(TrajectoryPoints& trajectory, const QGeoCoordinate& coordinate)
{
    QVERIFY(QMetaObject::invokeMethod(&trajectory, "_vehicleCoordinateChanged", Q_ARG(QGeoCoordinate, coordinate)));
}

}  // namespace

void TrajectoryPointsTest::_firstPointAndDecimation()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    TrajectoryPoints trajectory(&vehicle);
    QSignalSpy addedSpy(&trajectory, &TrajectoryPoints::pointAdded);
    QSignalSpy updatedSpy(&trajectory, &TrajectoryPoints::updateLastPoint);

    // The very first point always appends
    sendPosition(trajectory, kStart);
    QCOMPARE(addedSpy.count(), 1);
    QCOMPARE(trajectory.list().count(), 1);

    // Motion below the distance tolerance is ignored entirely
    sendPosition(trajectory, kStart.atDistanceAndAzimuth(1.0, 90));
    QCOMPARE(addedSpy.count(), 1);
    QCOMPARE(updatedSpy.count(), 0);

    // The second point always appends (no previous direction yet)
    const QGeoCoordinate east1 = kStart.atDistanceAndAzimuth(5.0, 90);
    sendPosition(trajectory, east1);
    QCOMPARE(addedSpy.count(), 2);

    // Colinear continuation moves the last point instead of appending
    const QGeoCoordinate east2 = east1.atDistanceAndAzimuth(5.0, 90);
    sendPosition(trajectory, east2);
    QCOMPARE(addedSpy.count(), 2);
    QCOMPARE(updatedSpy.count(), 1);
    QCOMPARE(trajectory.list().count(), 2);
    QCOMPARE(updatedSpy.last().first().value<QGeoCoordinate>(), east2);

    // A heading change appends a corner point
    sendPosition(trajectory, east2.atDistanceAndAzimuth(5.0, 0));
    QCOMPARE(addedSpy.count(), 3);
    QCOMPARE(trajectory.list().count(), 3);
}

void TrajectoryPointsTest::_verticalMotion()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    TrajectoryPoints trajectory(&vehicle);
    QSignalSpy addedSpy(&trajectory, &TrajectoryPoints::pointAdded);
    QSignalSpy updatedSpy(&trajectory, &TrajectoryPoints::updateLastPoint);

    // Horizontal leg first
    sendPosition(trajectory, kStart);
    const QGeoCoordinate east = kStart.atDistanceAndAzimuth(5.0, 90);
    sendPosition(trajectory, east);
    QCOMPARE(addedSpy.count(), 2);

    // Climb below the (3D) distance tolerance: ignored
    QGeoCoordinate smallClimb = east;
    smallClimb.setAltitude(east.altitude() + 1.5);
    sendPosition(trajectory, smallClimb);
    QCOMPARE(addedSpy.count(), 2);
    QCOMPARE(updatedSpy.count(), 0);

    // Straight-up climb: purely vertical motion must generate a corner point
    // (the old horizontal-only distance gate dropped it entirely)
    QGeoCoordinate climb1 = east;
    climb1.setAltitude(east.altitude() + 5.0);
    sendPosition(trajectory, climb1);
    QCOMPARE(addedSpy.count(), 3);

    // Continued climb is colinear: moves the last point
    QGeoCoordinate climb2 = climb1;
    climb2.setAltitude(climb1.altitude() + 5.0);
    sendPosition(trajectory, climb2);
    QCOMPARE(addedSpy.count(), 3);
    QCOMPARE(updatedSpy.count(), 1);

    // Leveling off is another direction change: corner point
    sendPosition(trajectory, climb2.atDistanceAndAzimuth(5.0, 90));
    QCOMPARE(addedSpy.count(), 4);
    QCOMPARE(trajectory.list().count(), 4);
}

void TrajectoryPointsTest::_nanAltitude()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    TrajectoryPoints trajectory(&vehicle);
    QSignalSpy addedSpy(&trajectory, &TrajectoryPoints::pointAdded);
    QSignalSpy updatedSpy(&trajectory, &TrajectoryPoints::updateLastPoint);

    // No altitude fix: NaN altitude deltas must degrade to horizontal-only
    // behavior instead of poisoning the distance gate
    const QGeoCoordinate start(kStart.latitude(), kStart.longitude());
    sendPosition(trajectory, start);
    const QGeoCoordinate east1 = start.atDistanceAndAzimuth(5.0, 90);
    sendPosition(trajectory, east1);
    QCOMPARE(addedSpy.count(), 2);

    const QGeoCoordinate east2 = east1.atDistanceAndAzimuth(5.0, 90);
    sendPosition(trajectory, east2);
    QCOMPARE(updatedSpy.count(), 1);

    // NaN -> finite altitude transition stays horizontal (delta treated as 0)
    QGeoCoordinate withAltitude = east2.atDistanceAndAzimuth(5.0, 90);
    withAltitude.setAltitude(50.0);
    sendPosition(trajectory, withAltitude);
    QCOMPARE(addedSpy.count(), 2);
    QCOMPARE(updatedSpy.count(), 2);
}

void TrajectoryPointsTest::_flightDistance3D()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    TrajectoryPoints trajectory(&vehicle);
    Fact* const distanceFact = vehicle.flightDistance();
    distanceFact->setRawValue(0.0);

    // The first point accumulates no distance
    sendPosition(trajectory, kStart);
    QCOMPARE(distanceFact->rawValue().toDouble(), 0.0);

    // Flight distance is through-the-air (3D): a pure 10m climb counts fully
    QGeoCoordinate up = kStart;
    up.setAltitude(kStart.altitude() + 10.0);
    sendPosition(trajectory, up);
    QCOMPARE_LT(std::abs(distanceFact->rawValue().toDouble() - 10.0), 0.01);

    // 3-4-5 diagonal: 4m horizontal + 3m up accumulates 5m
    sendPosition(trajectory, up.atDistanceAndAzimuth(4.0, 90, 3.0));
    QCOMPARE_LT(std::abs(distanceFact->rawValue().toDouble() - 15.0), 0.02);
}

void TrajectoryPointsTest::_clearResets()
{
    Vehicle vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
    TrajectoryPoints trajectory(&vehicle);
    QSignalSpy addedSpy(&trajectory, &TrajectoryPoints::pointAdded);
    QSignalSpy clearedSpy(&trajectory, &TrajectoryPoints::pointsCleared);

    sendPosition(trajectory, kStart);
    sendPosition(trajectory, kStart.atDistanceAndAzimuth(5.0, 90));
    QCOMPARE(trajectory.list().count(), 2);

    trajectory.clear();
    QCOMPARE(clearedSpy.count(), 1);
    QCOMPARE(trajectory.list().count(), 0);

    // Fresh start after clear: first point appends immediately again
    sendPosition(trajectory, kStart);
    QCOMPARE(addedSpy.count(), 3);
    QCOMPARE(trajectory.list().count(), 1);
}

UT_REGISTER_TEST(TrajectoryPointsTest, TestLabel::Unit, TestLabel::Vehicle)
