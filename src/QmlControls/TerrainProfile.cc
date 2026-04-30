#include "TerrainProfile.h"
#include "MissionController.h"
#include "QmlObjectListModel.h"
#include "FlightPathSegment.h"
#include "ComplexMissionItem.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(TerrainProfileLog, "Terrain.TerrainProfile")

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

void TerrainProfile::_updateSegmentCounts(FlightPathSegment* segment, int& cFlightProfileSegments, int& cTerrainProfilePoints, int& cMissingTerrainSegments, int& cTerrainCollisionSegments, double& minTerrainHeight, double& maxTerrainHeight)
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
            minTerrainHeight = std::fmin(minTerrainHeight, segment->amslTerrainHeights()[i].value<double>());
            maxTerrainHeight = std::fmax(maxTerrainHeight, segment->amslTerrainHeights()[i].value<double>());
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
    double minTerrainHeight =          qQNaN();
    double maxTerrainHeight =          qQNaN();

    for (int viIndex=0; viIndex<_visualItems->count(); viIndex++) {
        VisualMissionItem*  visualItem =    _visualItems->value<VisualMissionItem*>(viIndex);
        ComplexMissionItem* complexItem =   _visualItems->value<ComplexMissionItem*>(viIndex);

        if (visualItem->simpleFlightPathSegment()) {
            FlightPathSegment* segment = visualItem->simpleFlightPathSegment();
            _updateSegmentCounts(segment, cFlightProfileSegments, cTerrainProfilePoints, cMissingTerrainSegments, cTerrainCollisionSegments, minTerrainHeight, maxTerrainHeight);
        }

        if (complexItem) {
            for (int segmentIndex=0; segmentIndex<complexItem->flightPathSegments()->count(); segmentIndex++) {
                FlightPathSegment* segment = complexItem->flightPathSegments()->value<FlightPathSegment*>(segmentIndex);
                _updateSegmentCounts(segment, cFlightProfileSegments, cTerrainProfilePoints, cMissingTerrainSegments, cTerrainCollisionSegments, minTerrainHeight, maxTerrainHeight);
            }
        }
    }

    _minAMSLAlt = std::fmin(_missionController->minAMSLAltitude(), minTerrainHeight);
    _maxAMSLAlt = std::fmax(_missionController->maxAMSLAltitude(), maxTerrainHeight);

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
    qCDebug(TerrainProfileLog) << "missionController min/max" << _missionController->minAMSLAltitude() << _missionController->maxAMSLAltitude();
    qCDebug(TerrainProfileLog) << QStringLiteral("updateProfile counter:%1 cFlightProfileSegments:%2 cTerrainProfilePoints:%3 cMissingTerrainSegments:%4 cTerrainCollisionSegments:%5 _minAMSLAlt:%6 _maxAMSLAlt:%7 maxTerrainHeight:%8")
                                  .arg(counter++).arg(cFlightProfileSegments).arg(cTerrainProfilePoints).arg(cMissingTerrainSegments).arg(cTerrainCollisionSegments).arg(_minAMSLAlt).arg(_maxAMSLAlt).arg(maxTerrainHeight);

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

    if (terrainSeries) {
        terrainSeries->replace(terrainPoints);
    }
    if (flightSeries) {
        flightSeries->replace(flightPoints);
    }
    if (missingSeries) {
        missingSeries->replace(missingPoints);
    }
    if (collisionSeries) {
        collisionSeries->replace(collisionPoints);
    }

    qCDebug(TerrainProfileLog) << QStringLiteral("updateSeries terrain:%1 flight:%2 missing:%3 collision:%4")
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
        double terrainDistance = 0;
        double distanceToSurface = segment->coord1AMSLAlt() - segment->amslTerrainHeights().first().value<double>();
        for (int heightIndex=0; heightIndex<segment->amslTerrainHeights().count(); heightIndex++) {
            if (heightIndex == 0) {
                // First point
            } else if (heightIndex == segment->amslTerrainHeights().count() - 2) {
                terrainDistance += segment->finalDistanceBetween();
            } else {
                terrainDistance += segment->distanceBetween();
            }

            const double amslTerrainHeight = (segment->amslTerrainHeights()[heightIndex].value<double>() + distanceToSurface) * _verticalScale;
            points.append(QPointF((currentDistance + terrainDistance) * _horizontalScale, amslTerrainHeight));
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
        points.append(QPointF(currentDistance * _horizontalScale, segment->coord1AMSLAlt() * _verticalScale));
        points.append(QPointF((currentDistance + segment->totalDistance()) * _horizontalScale, segment->coord2AMSLAlt() * _verticalScale));
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
