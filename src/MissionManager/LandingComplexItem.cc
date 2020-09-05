/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LandingComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "SimpleMissionItem.h"
#include "PlanMasterController.h"
#include "FlightPathSegment.h"
#include "TakeoffMissionItem.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(LandingComplexItemLog, "LandingComplexItemLog")

LandingComplexItem::LandingComplexItem(PlanMasterController* masterController, bool flyView, QObject* parent)
    : ComplexMissionItem        (masterController, flyView, parent)
{
    _isIncomplete = false;

    // The following is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &LandingComplexItem::_updateFlightPathSegmentsSignal, this, &LandingComplexItem::_updateFlightPathSegmentsDontCallDirectly,   Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&LandingComplexItem::_updateFlightPathSegmentsSignal));
}

void LandingComplexItem::_init(void)
{
    connect(landingDistance(),  &Fact::valueChanged,                            this, &LandingComplexItem::_recalcFromHeadingAndDistanceChange);
    connect(landingHeading(),   &Fact::valueChanged,                            this, &LandingComplexItem::_recalcFromHeadingAndDistanceChange);

    connect(loiterRadius(),     &Fact::valueChanged,                            this, &LandingComplexItem::_recalcFromRadiusChange);
    connect(this,               &LandingComplexItem::loiterClockwiseChanged,    this, &LandingComplexItem::_recalcFromRadiusChange);

    connect(this,               &LandingComplexItem::loiterCoordinateChanged,   this, &LandingComplexItem::_recalcFromCoordinateChange);
    connect(this,               &LandingComplexItem::landingCoordinateChanged,  this, &LandingComplexItem::_recalcFromCoordinateChange);
}

void LandingComplexItem::setLandingHeadingToTakeoffHeading()
{
    TakeoffMissionItem* takeoffMissionItem = _missionController->takeoffMissionItem();
    if (takeoffMissionItem && takeoffMissionItem->specifiesCoordinate()) {
        qreal heading = takeoffMissionItem->launchCoordinate().azimuthTo(takeoffMissionItem->coordinate());
        landingHeading()->setRawValue(heading);
    }
}

double LandingComplexItem::complexDistance(void) const
{
    return loiterCoordinate().distanceTo(loiterTangentCoordinate()) + loiterTangentCoordinate().distanceTo(landingCoordinate());
}

// Never call this method directly. If you want to update the flight segments you emit _updateFlightPathSegmentsSignal()
void LandingComplexItem::_updateFlightPathSegmentsDontCallDirectly(void)
{
    if (_cTerrainCollisionSegments != 0) {
        _cTerrainCollisionSegments = 0;
        emit terrainCollisionChanged(false);
    }

    _flightPathSegments.beginReset();
    _flightPathSegments.clearAndDeleteContents();
    _appendFlightPathSegment(loiterCoordinate(),        amslEntryAlt(), loiterTangentCoordinate(),  amslEntryAlt());
    _appendFlightPathSegment(loiterTangentCoordinate(), amslEntryAlt(), landingCoordinate(),        amslEntryAlt());
    _appendFlightPathSegment(landingCoordinate(),       amslEntryAlt(), landingCoordinate(),        amslExitAlt());
    _flightPathSegments.endReset();

    if (_cTerrainCollisionSegments != 0) {
        emit terrainCollisionChanged(true);
    }

    _masterController->missionController()->recalcTerrainProfile();
}

void LandingComplexItem::setLandingCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != _landingCoordinate) {
        _landingCoordinate = coordinate;
        if (_landingCoordSet) {
            emit exitCoordinateChanged(coordinate);
            emit landingCoordinateChanged(coordinate);
        } else {
            _ignoreRecalcSignals = true;
            emit exitCoordinateChanged(coordinate);
            emit landingCoordinateChanged(coordinate);
            _ignoreRecalcSignals = false;
            _landingCoordSet = true;
            _recalcFromHeadingAndDistanceChange();
            emit landingCoordSetChanged(true);
        }
    }
}

void LandingComplexItem::setLoiterCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != _loiterCoordinate) {
        _loiterCoordinate = coordinate;
        emit coordinateChanged(coordinate);
        emit loiterCoordinateChanged(coordinate);
    }
}

QPointF LandingComplexItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void LandingComplexItem::_recalcFromHeadingAndDistanceChange(void)
{
    // Fixed:
    //      land
    //      heading
    //      distance
    //      radius
    // Adjusted:
    //      loiter
    //      loiter tangent
    //      glide slope

    if (!_ignoreRecalcSignals && _landingCoordSet) {
        // These are our known values
        double radius = loiterRadius()->rawValue().toDouble();
        double landToTangentDistance = landingDistance()->rawValue().toDouble();
        double heading = landingHeading()->rawValue().toDouble();

        // Heading is from loiter to land, hence +180
        _loiterTangentCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToTangentDistance, heading + 180);

        // Loiter coord is 90 degrees counter clockwise from tangent coord
        _loiterCoordinate = _loiterTangentCoordinate.atDistanceAndAzimuth(radius, heading - 180 - 90);
        _loiterCoordinate.setAltitude(loiterAltitude()->rawValue().toDouble());

        _ignoreRecalcSignals = true;
        emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
        emit loiterCoordinateChanged(_loiterCoordinate);
        emit coordinateChanged(_loiterCoordinate);
        _calcGlideSlope();
        _ignoreRecalcSignals = false;
    }
}

void LandingComplexItem::_recalcFromRadiusChange(void)
{
    // Fixed:
    //      land
    //      loiter tangent
    //      distance
    //      radius
    //      heading
    // Adjusted:
    //      loiter

    if (!_ignoreRecalcSignals) {
        // These are our known values
        double radius  = loiterRadius()->rawValue().toDouble();
        double landToTangentDistance = landingDistance()->rawValue().toDouble();
        double heading = landingHeading()->rawValue().toDouble();

        double landToLoiterDistance = _landingCoordinate.distanceTo(_loiterCoordinate);
        if (landToLoiterDistance < radius) {
            // Degnenerate case: Move tangent to loiter point
            _loiterTangentCoordinate = _loiterCoordinate;

            double heading = _landingCoordinate.azimuthTo(_loiterTangentCoordinate);

            _ignoreRecalcSignals = true;
            landingHeading()->setRawValue(heading);
            emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
            _ignoreRecalcSignals = false;
        } else {
            double landToLoiterDistance = qSqrt(qPow(radius, 2) + qPow(landToTangentDistance, 2));
            double angleLoiterToTangent = qRadiansToDegrees(qAsin(radius/landToLoiterDistance)) * (_loiterClockwise ? -1 : 1);

            _loiterCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToLoiterDistance, heading + 180 + angleLoiterToTangent);
            _loiterCoordinate.setAltitude(loiterAltitude()->rawValue().toDouble());

            _ignoreRecalcSignals = true;
            emit loiterCoordinateChanged(_loiterCoordinate);
            emit coordinateChanged(_loiterCoordinate);
            _ignoreRecalcSignals = false;
        }
    }
}

void LandingComplexItem::_recalcFromCoordinateChange(void)
{
    // Fixed:
    //      land
    //      loiter
    //      radius
    // Adjusted:
    //      loiter tangent
    //      heading
    //      distance
    //      glide slope

    if (!_ignoreRecalcSignals && _landingCoordSet) {
        // These are our known values
        double radius = loiterRadius()->rawValue().toDouble();
        double landToLoiterDistance = _landingCoordinate.distanceTo(_loiterCoordinate);
        double landToLoiterHeading = _landingCoordinate.azimuthTo(_loiterCoordinate);

        double landToTangentDistance;
        if (landToLoiterDistance < radius) {
            // Degenerate case, set tangent to loiter coordinate
            _loiterTangentCoordinate = _loiterCoordinate;
            landToTangentDistance = _landingCoordinate.distanceTo(_loiterTangentCoordinate);
        } else {
            double loiterToTangentAngle = qRadiansToDegrees(qAsin(radius/landToLoiterDistance)) * (_loiterClockwise ? 1 : -1);
            landToTangentDistance = qSqrt(qPow(landToLoiterDistance, 2) - qPow(radius, 2));

            _loiterTangentCoordinate = _landingCoordinate.atDistanceAndAzimuth(landToTangentDistance, landToLoiterHeading + loiterToTangentAngle);

        }

        double heading = _loiterTangentCoordinate.azimuthTo(_landingCoordinate);

        _ignoreRecalcSignals = true;
        landingHeading()->setRawValue(heading);
        landingDistance()->setRawValue(landToTangentDistance);
        emit loiterTangentCoordinateChanged(_loiterTangentCoordinate);
        _calcGlideSlope();
        _ignoreRecalcSignals = false;
    }
}


