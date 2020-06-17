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

#include <QPolygonF>

QGC_LOGGING_CATEGORY(LandingComplexItemLog, "LandingComplexItemLog")

LandingComplexItem::LandingComplexItem(PlanMasterController* masterController, bool flyView, QObject* parent)
    : ComplexMissionItem        (masterController, flyView, parent)
{
    _isIncomplete = false;

    // The follow is used to compress multiple recalc calls in a row to into a single call.
    connect(this, &LandingComplexItem::_updateFlightPathSegmentsSignal, this, &LandingComplexItem::_updateFlightPathSegmentsDontCallDirectly,   Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&LandingComplexItem::_updateFlightPathSegmentsSignal));
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
