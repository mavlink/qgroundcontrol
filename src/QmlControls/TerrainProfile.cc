#include "TerrainProfile.h"
#include "MissionController.h"
#include "QmlObjectListModel.h"
#include "FlightPathSegment.h"
#include "ComplexMissionItem.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(TerrainProfileLog, "Terrain.TerrainProfile")

namespace {

// QtGraphs keeps drawing a LineSeries that is emptied after showing a line, in another series' style
// (the collision line going from a hit to none left a second flight line). A single point draws no
// line, so a series is never handed zero points.
void setSeriesPoints(QXYSeries* series, QList<QPointF> points)
{
    if (!series) {
        return;
    }
    if (points.isEmpty()) {
        points.append(QPointF(0, 0));
    }
    // Using clear/append instead of replace works around bugs in QtGraphs where you end up with parts of old
    // series data showing.
    series->clear();
    series->append(points);
}

}  // namespace

TerrainProfile::TerrainProfile(QQuickItem* parent)
    : QQuickItem(parent)
{
    connect(this, &QQuickItem::heightChanged, this, &TerrainProfile::_updateProfile);

    connect(this, &TerrainProfile::_updateSignal, this, &TerrainProfile::_updateProfile, Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&TerrainProfile::_updateSignal));
}

void TerrainProfile::setVisibleWidth(double visibleWidth)
{
    if (qFuzzyCompare(_visibleWidth, visibleWidth)) {
        return;
    }
    _visibleWidth = visibleWidth;
    emit visibleWidthChanged();
}

void TerrainProfile::componentComplete(void)
{
    QQuickItem::componentComplete();
}

void TerrainProfile::setMissionController(MissionController* missionController)
{
    if (missionController != _missionController) {
        _missionController =    missionController;
        _visualItems =          _missionController->visualItems();

        emit missionControllerChanged();

        connect(_missionController, &MissionController::visualItemsReset,           this, &TerrainProfile::_newVisualItems);
        connect(this,               &TerrainProfile::visibleWidthChanged,           this, &TerrainProfile::_updateSignal, Qt::QueuedConnection);
        connect(_missionController, &MissionController::recalcTerrainProfile,       this, &TerrainProfile::_updateSignal, Qt::QueuedConnection);

        emit _updateSignal();
    }
}

void TerrainProfile::_newVisualItems(void)
{
    _visualItems = _missionController->visualItems();
    emit _updateSignal();
}

void TerrainProfile::_updateSegmentCounts(FlightPathSegment* segment, int& cFlightProfileSegments,
                                          int& cTerrainProfilePoints, int& cMissingTerrainSegments,
                                          int& cTerrainCollisionSegments, double& minProfileHeight,
                                          double& maxProfileHeight)
{
    if (_shouldAddFlightProfileSegment(segment)) {
        if (segment->segmentType() == FlightPathSegment::SegmentTypeTerrainFrame) {
            cFlightProfileSegments += segment->amslTerrainHeights().count() - 1;
        } else {
            cFlightProfileSegments++;
        }
    }

    if (_shouldAddMissingTerrainSegment(segment)) {
        cMissingTerrainSegments += 1;
    } else {
        cTerrainProfilePoints += segment->amslTerrainHeights().count();
        for (int i=0; i<segment->amslTerrainHeights().count(); i++) {
            minProfileHeight = std::fmin(minProfileHeight, segment->amslTerrainHeights()[i].value<double>());
            maxProfileHeight = std::fmax(maxProfileHeight, segment->amslTerrainHeights()[i].value<double>());
        }
    }
    // A spline can overshoot its waypoint altitudes, so its samples take part in the vertical bounds too
    for (const QVariant& sample : segment->splinePath()) {
        const double sampleAlt = sample.value<QGeoCoordinate>().altitude();
        if (!qIsNaN(sampleAlt)) {
            minProfileHeight = std::fmin(minProfileHeight, sampleAlt);
            maxProfileHeight = std::fmax(maxProfileHeight, sampleAlt);
        }
    }
    if (segment->terrainCollision()) {
        cTerrainCollisionSegments++;
    }
}

void TerrainProfile::_updateProfile(void)
{
    if (!_missionController || !_visualItems) {
        return;
    }

    int    cTerrainProfilePoints =     0;
    int    cMissingTerrainSegments =   0;
    int    cFlightProfileSegments =    0;
    int    cTerrainCollisionSegments = 0;
    double minProfileHeight = qQNaN();
    double maxProfileHeight = qQNaN();

    for (int viIndex=0; viIndex<_visualItems->count(); viIndex++) {
        VisualMissionItem*  visualItem =    _visualItems->value<VisualMissionItem*>(viIndex);
        ComplexMissionItem* complexItem =   _visualItems->value<ComplexMissionItem*>(viIndex);

        if (visualItem->simpleFlightPathSegment()) {
            FlightPathSegment* segment = visualItem->simpleFlightPathSegment();
            _updateSegmentCounts(segment, cFlightProfileSegments, cTerrainProfilePoints, cMissingTerrainSegments,
                                 cTerrainCollisionSegments, minProfileHeight, maxProfileHeight);
        }

        if (complexItem) {
            for (int segmentIndex=0; segmentIndex<complexItem->flightPathSegments()->count(); segmentIndex++) {
                FlightPathSegment* segment = complexItem->flightPathSegments()->value<FlightPathSegment*>(segmentIndex);
                _updateSegmentCounts(segment, cFlightProfileSegments, cTerrainProfilePoints, cMissingTerrainSegments,
                                     cTerrainCollisionSegments, minProfileHeight, maxProfileHeight);
            }
        }
    }

    _minAMSLAlt = std::fmin(_missionController->minAMSLAltitude(), minProfileHeight);
    _maxAMSLAlt = std::fmax(_missionController->maxAMSLAltitude(), maxProfileHeight);

    double amslAltRange = _maxAMSLAlt - _minAMSLAlt;
    double amslAltRangeBuffer = amslAltRange * 0.1;
    _maxAMSLAlt += amslAltRangeBuffer;
    if (_minAMSLAlt > 0.0) {
        _minAMSLAlt -= amslAltRangeBuffer;
        _minAMSLAlt = std::fmax(_minAMSLAlt, 0.0);
    }

    const double missionTotalDistance = _missionController->missionTotalDistance();
    _pixelsPerMeter = (missionTotalDistance > 0.0) ? (_visibleWidth / missionTotalDistance) : 0.0;

    static int counter = 0;
    qCDebug(TerrainProfileLog).noquote() << "counter" << counter++ << "\n\tmissionController minAMSLAltitude"
                                         << _missionController->minAMSLAltitude()
                                         << "\n\tmissionController maxAMSLAltitude"
                                         << _missionController->maxAMSLAltitude() << "\n\tcFlightProfileSegments"
                                         << cFlightProfileSegments << "\n\tcTerrainProfilePoints"
                                         << cTerrainProfilePoints << "\n\tcMissingTerrainSegments"
                                         << cMissingTerrainSegments << "\n\tcTerrainCollisionSegments"
                                         << cTerrainCollisionSegments << "\n\t_minAMSLAlt" << _minAMSLAlt
                                         << "\n\t_maxAMSLAlt" << _maxAMSLAlt << "\n\tmaxProfileHeight"
                                         << maxProfileHeight;

    setImplicitWidth(_visibleWidth);
    setWidth(implicitWidth());

    emit pixelsPerMeterChanged();
    emit minAMSLAltChanged();
    emit maxAMSLAltChanged();
    emit profileChanged();
}

void TerrainProfile::updateSeries(QXYSeries* terrainSeries, QXYSeries* flightSeries, QXYSeries* missingSeries, QXYSeries* collisionSeries)
{
    if (!_missionController || !_visualItems) {
        return;
    }

    QList<QPointF> terrainPoints;
    QList<QPointF> flightPoints;
    QList<QPointF> missingPoints;
    QList<QPointF> collisionPoints;

    double currentDistance = 0;

    for (int viIndex=0; viIndex<_visualItems->count(); viIndex++) {
        VisualMissionItem*  visualItem =    _visualItems->value<VisualMissionItem*>(viIndex);
        ComplexMissionItem* complexItem =   _visualItems->value<ComplexMissionItem*>(viIndex);

        if (complexItem) {
            if (complexItem->flightPathSegments()->count() == 0) {
                currentDistance += complexItem->complexDistance();
            } else {
                for (int segmentIndex=0; segmentIndex<complexItem->flightPathSegments()->count(); segmentIndex++) {
                    FlightPathSegment* segment = complexItem->flightPathSegments()->value<FlightPathSegment*>(segmentIndex);

                    _addTerrainPoints   (segment, currentDistance, terrainPoints);
                    _addFlightPoints    (segment, currentDistance, flightPoints);
                    _addMissingPoints   (segment, currentDistance, missingPoints);
                    _addCollisionPoints (segment, currentDistance, collisionPoints);

                    currentDistance += segment->totalDistance();
                }
            }
        }

        if (visualItem->simpleFlightPathSegment()) {
            FlightPathSegment* segment = visualItem->simpleFlightPathSegment();

            _addTerrainPoints   (segment, currentDistance, terrainPoints);
            _addFlightPoints    (segment, currentDistance, flightPoints);
            _addMissingPoints   (segment, currentDistance, missingPoints);
            _addCollisionPoints (segment, currentDistance, collisionPoints);

            currentDistance += segment->totalDistance();
        }
    }

    setSeriesPoints(terrainSeries, terrainPoints);
    setSeriesPoints(flightSeries, flightPoints);
    setSeriesPoints(missingSeries, missingPoints);
    setSeriesPoints(collisionSeries, collisionPoints);

    qCDebug(TerrainProfileLog).noquote() << QStringLiteral("updateSeries terrainPoints:%1 flightPoints:%2 missingPoints:%3 collisionPoints:%4")
        .arg(terrainPoints.count()).arg(flightPoints.count()).arg(missingPoints.count()).arg(collisionPoints.count());
}

void TerrainProfile::_addTerrainPoints(FlightPathSegment* segment, double currentDistance, QList<QPointF>& points)
{
    if (_shouldAddMissingTerrainSegment(segment)) {
        if (!points.isEmpty()) {
            points.append(QPointF(qQNaN(), qQNaN()));
        }
        return;
    }

    double terrainDistance = 0;
    for (int heightIndex=0; heightIndex<segment->amslTerrainHeights().count(); heightIndex++) {
        if (heightIndex == 0) {
            // First point at start of segment
        } else if (heightIndex == segment->amslTerrainHeights().count() - 2) {
            terrainDistance += segment->finalDistanceBetween();
        } else {
            terrainDistance += segment->distanceBetween();
        }

        const double amslTerrainHeight = segment->amslTerrainHeights()[heightIndex].value<double>() * _verticalScale;
        points.append(QPointF((currentDistance + terrainDistance) * _horizontalScale, amslTerrainHeight));
    }
}

void TerrainProfile::_addFlightPoints(FlightPathSegment* segment, double currentDistance, QList<QPointF>& points)
{
    if (!_shouldAddFlightProfileSegment(segment)) {
        if (!points.isEmpty()) {
            points.append(QPointF(currentDistance * _horizontalScale, qQNaN()));
        }
        return;
    }

    if (segment->segmentType() == FlightPathSegment::SegmentTypeTerrainFrame) {
        _addTerrainFrameAltitudePoints(segment, currentDistance, points);
    } else {
        _addFlightAltitudePoints(segment, currentDistance, points);
    }
}

// The flown altitude over each terrain sample of a terrain-frame leg
void TerrainProfile::_addTerrainFrameAltitudePoints(FlightPathSegment* segment, double currentDistance,
                                                    QList<QPointF>& points)
{
    // A spline with no horizontal extent has a terrain sample only at each end, so the altitudes in between come
    // from its own samples
    if (segment->splineSampleDistances().count() > 1 && segment->totalDistance() <= 0) {
        _addFlightAltitudePoints(segment, currentDistance, points);
        return;
    }

    double terrainDistance = 0;
    for (int heightIndex = 0; heightIndex < segment->amslTerrainHeights().count(); heightIndex++) {
        if (heightIndex == 0) {
            // First point
        } else if (heightIndex == segment->amslTerrainHeights().count() - 2) {
            terrainDistance += segment->finalDistanceBetween();
        } else {
            terrainDistance += segment->distanceBetween();
        }

        points.append(QPointF((currentDistance + terrainDistance) * _horizontalScale,
                              segment->terrainFrameAltitudeAt(heightIndex, terrainDistance) * _verticalScale));
    }
}

// The flown altitude along the leg: at every spline sample for a spline leg, otherwise a straight ramp
void TerrainProfile::_addFlightAltitudePoints(FlightPathSegment* segment, double currentDistance,
                                              QList<QPointF>& points)
{
    const QList<double>& sampleDistances = segment->splineSampleDistances();
    if (sampleDistances.count() > 1) {
        // Each sample's own altitude, which also covers a vertical spline whose samples all share one distance
        for (int i = 0; i < sampleDistances.count(); i++) {
            const double sampleAlt = segment->splinePath()[i].value<QGeoCoordinate>().altitude();
            const double altitude = qIsNaN(sampleAlt) ? segment->amslAltitudeAtDistance(sampleDistances[i]) : sampleAlt;
            points.append(
                QPointF((currentDistance + sampleDistances[i]) * _horizontalScale, altitude * _verticalScale));
        }
    } else {
        points.append(QPointF(currentDistance * _horizontalScale, segment->coord1AMSLAlt() * _verticalScale));
        points.append(QPointF((currentDistance + segment->totalDistance()) * _horizontalScale, segment->coord2AMSLAlt() * _verticalScale));
    }
}

void TerrainProfile::_addMissingPoints(FlightPathSegment* segment, double currentDistance, QList<QPointF>& points)
{
    if (_shouldAddMissingTerrainSegment(segment)) {
        if (!points.isEmpty()) {
            points.append(QPointF(currentDistance * _horizontalScale, qQNaN()));
        }
        const double minAlt = _minAMSLAlt * _verticalScale;
        points.append(QPointF(currentDistance * _horizontalScale, minAlt));
        points.append(QPointF((currentDistance + segment->totalDistance()) * _horizontalScale, minAlt));
    }
}

void TerrainProfile::_addCollisionPoints(FlightPathSegment* segment, double currentDistance, QList<QPointF>& points)
{
    if (segment->terrainCollision()) {
        if (!points.isEmpty()) {
            points.append(QPointF(currentDistance * _horizontalScale, qQNaN()));
        }
        if (segment->segmentType() == FlightPathSegment::SegmentTypeTerrainFrame) {
            _addTerrainFrameAltitudePoints(segment, currentDistance, points);
        } else {
            _addFlightAltitudePoints(segment, currentDistance, points);
        }
    }
}

bool TerrainProfile::_shouldAddFlightProfileSegment(FlightPathSegment* segment)
{
    bool shouldAdd = !qIsNaN(segment->coord1AMSLAlt()) && !qIsNaN(segment->coord2AMSLAlt());
    if (segment->segmentType() == FlightPathSegment::SegmentTypeTerrainFrame) {
        shouldAdd &= segment->amslTerrainHeights().count() != 0;
    }
    return shouldAdd;
}

bool TerrainProfile::_shouldAddMissingTerrainSegment(FlightPathSegment* segment)
{
    return segment->amslTerrainHeights().count() == 0;
}
