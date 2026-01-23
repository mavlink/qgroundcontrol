#include "PlanStatistics.h"
#include "GeoUtilities.h"
#include "QGCGeo.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "MissionItem.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"
#include "TransectStyleComplexItem.h"

#include <limits>
#include <cmath>

PlanStatistics::PlanStatistics() = default;

void PlanStatistics::clear()
{
    _flightPath.clear();
    _totalDistance = 0.0;
    _waypointCount = 0;
    _minAltitude = 0.0;
    _maxAltitude = 0.0;
    _surveyArea = 0.0;
    _boundingBox = QGeoRectangle();
    _turns.clear();
    _maxTurnAngle = 0.0;
    _avgTurnAngle = 0.0;
    _segments.clear();
    _shortestSegment = 0.0;
    _longestSegment = 0.0;
    _avgSegment = 0.0;
}

void PlanStatistics::_analyzeFlightPath(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();
    _minAltitude = std::numeric_limits<double>::max();
    _maxAltitude = std::numeric_limits<double>::lowest();

    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(
            vehicle, QGCMAVLink::VehicleClassGeneric, item->command());

        if (uiInfo != nullptr) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude();

            if (uiInfo->isTakeoffCommand() && !vehicle->fixedWing()) {
                QGeoCoordinate coord = homeCoord;
                coord.setAltitude(item->param7() + altAdjustment);
                _flightPath.append(coord);
                _waypointCount++;
            }

            if (uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
                QGeoCoordinate coord = item->coordinate();
                double altitude = coord.altitude() + altAdjustment;
                coord.setAltitude(altitude);
                _flightPath.append(coord);
                _waypointCount++;

                if (altitude < _minAltitude) {
                    _minAltitude = altitude;
                }
                if (altitude > _maxAltitude) {
                    _maxAltitude = altitude;
                }
            }
        }
    }

    // Calculate total distance
    if (_flightPath.count() >= 2) {
        for (int i = 1; i < _flightPath.count(); i++) {
            _totalDistance += QGCGeo::geodesicDistance(_flightPath[i - 1], _flightPath[i]);
        }
    }

    // Calculate bounding box
    _boundingBox = GeoUtilities::boundingBox(_flightPath);
}

void PlanStatistics::_analyzeTurns()
{
    _turns.clear();
    _maxTurnAngle = 0.0;
    double turnAngleSum = 0.0;

    if (_flightPath.count() < 3) {
        return;
    }

    for (int i = 1; i < _flightPath.count() - 1; i++) {
        TurnInfo info;
        info.waypointIndex = i;
        info.angleDegrees = GeoUtilities::turnAngle(_flightPath[i - 1], _flightPath[i], _flightPath[i + 1]);
        info.requiredRadiusM = GeoUtilities::requiredTurnRadius(_flightPath[i - 1], _flightPath[i], _flightPath[i + 1]);
        info.feasible = GeoUtilities::isTurnFeasible(_flightPath[i - 1], _flightPath[i], _flightPath[i + 1], _minTurnRadius);

        _turns.append(info);
        turnAngleSum += info.angleDegrees;

        if (info.angleDegrees > _maxTurnAngle) {
            _maxTurnAngle = info.angleDegrees;
        }
    }

    if (!_turns.isEmpty()) {
        _avgTurnAngle = turnAngleSum / _turns.count();
    }
}

void PlanStatistics::_analyzeSegments()
{
    _segments.clear();
    _shortestSegment = std::numeric_limits<double>::max();
    _longestSegment = 0.0;
    double segmentSum = 0.0;

    if (_flightPath.count() < 2) {
        _shortestSegment = 0.0;
        return;
    }

    for (int i = 0; i < _flightPath.count() - 1; i++) {
        SegmentInfo info;
        info.startIndex = i;
        info.endIndex = i + 1;
        info.distanceMeters = QGCGeo::geodesicDistance(_flightPath[i], _flightPath[i + 1]);
        info.headingDegrees = QGCGeo::geodesicAzimuth(_flightPath[i], _flightPath[i + 1]);

        _segments.append(info);
        segmentSum += info.distanceMeters;

        if (info.distanceMeters < _shortestSegment) {
            _shortestSegment = info.distanceMeters;
        }
        if (info.distanceMeters > _longestSegment) {
            _longestSegment = info.distanceMeters;
        }
    }

    if (!_segments.isEmpty()) {
        _avgSegment = segmentSum / _segments.count();
    }
}

void PlanStatistics::_analyzeSurveyAreas(QmlObjectListModel* visualItems)
{
    _surveyArea = 0.0;

    for (int i = 0; i < visualItems->count(); i++) {
        auto* transectItem = visualItems->value<TransectStyleComplexItem*>(i);
        if (transectItem != nullptr) {
            QGCMapPolygon* polygon = transectItem->surveyAreaPolygon();
            if (polygon != nullptr && polygon->count() >= 3) {
                QList<QGeoCoordinate> vertices;
                for (int j = 0; j < polygon->count(); j++) {
                    vertices.append(polygon->vertexCoordinate(j));
                }
                _surveyArea += QGCGeo::polygonArea(vertices);
            }
        }
    }
}

void PlanStatistics::analyze(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    clear();
    _analyzeFlightPath(vehicle, rgMissionItems);
    _analyzeTurns();
    _analyzeSegments();
    _analyzeSurveyAreas(visualItems);
}

int PlanStatistics::infeasibleTurnCount() const
{
    int count = 0;
    for (const TurnInfo& turn : _turns) {
        if (!turn.feasible) {
            count++;
        }
    }
    return count;
}

QGeoCoordinate PlanStatistics::center() const
{
    return _boundingBox.center();
}

QString PlanStatistics::summary() const
{
    QStringList lines;

    lines.append(QObject::tr("=== Plan Statistics ==="));
    lines.append(QObject::tr("Waypoints: %1").arg(_waypointCount));
    lines.append(QObject::tr("Total Distance: %1 m (%2 km)")
                     .arg(_totalDistance, 0, 'f', 1)
                     .arg(_totalDistance / 1000.0, 0, 'f', 2));

    if (_waypointCount > 0) {
        lines.append(QObject::tr("Altitude Range: %1 - %2 m")
                         .arg(_minAltitude, 0, 'f', 1)
                         .arg(_maxAltitude, 0, 'f', 1));
    }

    if (_surveyArea > 0) {
        double hectares = _surveyArea / 10000.0;
        lines.append(QObject::tr("Survey Area: %1 m² (%2 ha)")
                         .arg(_surveyArea, 0, 'f', 1)
                         .arg(hectares, 0, 'f', 2));
    }

    if (!_segments.isEmpty()) {
        lines.append(QObject::tr("Segment Stats: shortest=%1m, longest=%2m, avg=%3m")
                         .arg(_shortestSegment, 0, 'f', 1)
                         .arg(_longestSegment, 0, 'f', 1)
                         .arg(_avgSegment, 0, 'f', 1));
    }

    if (!_turns.isEmpty()) {
        int infeasible = infeasibleTurnCount();
        lines.append(QObject::tr("Turns: %1 total, max=%2°, avg=%3°")
                         .arg(_turns.count())
                         .arg(_maxTurnAngle, 0, 'f', 1)
                         .arg(_avgTurnAngle, 0, 'f', 1));
        if (infeasible > 0) {
            lines.append(QObject::tr("WARNING: %1 infeasible turns (min radius %2m)")
                             .arg(infeasible)
                             .arg(_minTurnRadius, 0, 'f', 0));
        }
    }

    if (_boundingBox.isValid()) {
        lines.append(QObject::tr("Bounding Box: %1° to %2° lat, %3° to %4° lon")
                         .arg(_boundingBox.bottomLeft().latitude(), 0, 'f', 6)
                         .arg(_boundingBox.topRight().latitude(), 0, 'f', 6)
                         .arg(_boundingBox.bottomLeft().longitude(), 0, 'f', 6)
                         .arg(_boundingBox.topRight().longitude(), 0, 'f', 6));
    }

    return lines.join(QStringLiteral("\n"));
}

QString PlanStatistics::turnAnalysisSummary() const
{
    if (_turns.isEmpty()) {
        return QObject::tr("No turns in flight path");
    }

    QStringList lines;
    lines.append(QObject::tr("=== Turn Analysis (min radius: %1m) ===").arg(_minTurnRadius, 0, 'f', 0));
    lines.append(QObject::tr("Index\tAngle\tReq.Radius\tFeasible"));
    lines.append(QObject::tr("-----\t-----\t----------\t--------"));

    for (const TurnInfo& turn : _turns) {
        lines.append(QStringLiteral("%1\t%2°\t%3m\t%4")
                         .arg(turn.waypointIndex)
                         .arg(turn.angleDegrees, 0, 'f', 1)
                         .arg(turn.requiredRadiusM, 0, 'f', 1)
                         .arg(turn.feasible ? QObject::tr("Yes") : QObject::tr("NO")));
    }

    return lines.join(QStringLiteral("\n"));
}
