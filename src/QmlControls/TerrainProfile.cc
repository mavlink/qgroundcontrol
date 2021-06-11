/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainProfile.h"
#include "MissionController.h"
#include "QmlObjectListModel.h"
#include "FlightPathSegment.h"
#include "SimpleMissionItem.h"
#include "ComplexMissionItem.h"

#include <QSGSimpleRectNode>

QGC_LOGGING_CATEGORY(TerrainProfileLog, "TerrainProfileLog")

TerrainProfile::TerrainProfile(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);

    connect(this, &QQuickItem::heightChanged,           this, &QQuickItem::update);
    connect(this, &TerrainProfile::visibleWidthChanged, this, &QQuickItem::update);

    // This collapse multiple _updateSignals in a row to a single update
    connect(this, &TerrainProfile::_updateSignal, this, &QQuickItem::update, Qt::QueuedConnection);
    qgcApp()->addCompressedSignal(QMetaMethod::fromSignal(&TerrainProfile::_updateSignal));
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

        connect(_missionController, &MissionController::visualItemsChanged,         this, &TerrainProfile::_newVisualItems);

        connect(this,               &TerrainProfile::visibleWidthChanged,           this, &TerrainProfile::_updateSignal, Qt::QueuedConnection);
        connect(_missionController, &MissionController::recalcTerrainProfile,       this, &TerrainProfile::_updateSignal, Qt::QueuedConnection);
    }
}

void TerrainProfile::_newVisualItems(void)
{
    _visualItems = _missionController->visualItems();
    emit _updateSignal();
}

void TerrainProfile::_createGeometry(QSGGeometryNode*& geometryNode, QSGGeometry*& geometry, QSGGeometry::DrawingMode drawingMode, const QColor& color)
{
    QSGFlatColorMaterial* terrainMaterial = new QSGFlatColorMaterial;
    terrainMaterial->setColor(color);

    geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
    geometry->setDrawingMode(drawingMode);
    geometry->setLineWidth(2);

    geometryNode = new QSGGeometryNode;
    geometryNode->setFlag(QSGNode::OwnsGeometry);
    geometryNode->setFlag(QSGNode::OwnsMaterial);
    geometryNode->setFlag(QSGNode::OwnedByParent);
    geometryNode->setMaterial(terrainMaterial);
    geometryNode->setGeometry(geometry);
}

void TerrainProfile::_updateSegmentCounts(FlightPathSegment* segment, int& cFlightProfileSegments, int& cTerrainProfilePoints, int& cMissingTerrainSegments, int& cTerrainCollisionSegments, double& minTerrainHeight, double& maxTerrainHeight)
{
    if (_shouldAddFlightProfileSegment(segment)) {
        if (segment->segmentType() == FlightPathSegment::SegmentTypeTerrainFrame) {
            // We show a full above terrain profile for flight segment
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

void TerrainProfile::_addTerrainProfileSegment(FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* terrainVertices, int& terrainProfileVertexIndex)
{
    double terrainDistance = 0;
    for (int heightIndex=0; heightIndex<segment->amslTerrainHeights().count(); heightIndex++) {
        // Move along the x axis which is distance
        if (heightIndex == 0) {
            // The first point in the segment is at the position of the last point. So nothing to do here.
        } else if (heightIndex == segment->amslTerrainHeights().count() - 2) {
            // The distance between the last two heights differs with each terrain query
            terrainDistance += segment->finalDistanceBetween();
        } else {
            // The distance between all terrain heights except for the last is the same
            terrainDistance += segment->distanceBetween();
        }

        // Move along the y axis which is a view or terrain height as a percentage between the min/max AMSL altitude for all segments
        double amslTerrainHeight    = segment->amslTerrainHeights()[heightIndex].value<double>();
        double terrainHeightPercent = (amslTerrainHeight - _minAMSLAlt) / amslAltRange;

        float x = (currentDistance + terrainDistance) * _pixelsPerMeter;
        float y = height() - (terrainHeightPercent * height());
        terrainVertices[terrainProfileVertexIndex++].set(x, y);
    }
}

void TerrainProfile::_addMissingTerrainSegment(FlightPathSegment* segment, double currentDistance, QSGGeometry::Point2D* missingTerrainVertices, int& missingterrainProfileVertexIndex)
{
    if (_shouldAddMissingTerrainSegment(segment)) {
        float x = currentDistance * _pixelsPerMeter;
        float y = height();
        missingTerrainVertices[missingterrainProfileVertexIndex++].set(x, y);
        missingTerrainVertices[missingterrainProfileVertexIndex++].set(x + (segment->totalDistance() * _pixelsPerMeter), y);
    }
}

void TerrainProfile::_addTerrainCollisionSegment(FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* terrainCollisionVertices, int& terrainCollisionVertexIndex)
{
    if (segment->terrainCollision()) {
        double amslCoord1Height =       segment->coord1AMSLAlt();
        double amslCoord2Height =       segment->coord2AMSLAlt();
        double coord1HeightPercent =    (amslCoord1Height - _minAMSLAlt) / amslAltRange;
        double coord2HeightPercent =    (amslCoord2Height - _minAMSLAlt) / amslAltRange;

        float x = currentDistance * _pixelsPerMeter;
        float y = height() - (coord1HeightPercent * height());

        terrainCollisionVertices[terrainCollisionVertexIndex++].set(x, y);

        x += segment->totalDistance() * _pixelsPerMeter;
        y = height() - (coord2HeightPercent * height());

        terrainCollisionVertices[terrainCollisionVertexIndex++].set(x, y);
    }
}

void TerrainProfile::_addFlightProfileSegment(FlightPathSegment* segment, double currentDistance, double amslAltRange, QSGGeometry::Point2D* flightProfileVertices, int& flightProfileVertexIndex)
{
    if (!_shouldAddFlightProfileSegment(segment)) {
        return;
    }

    if (segment->segmentType() == FlightPathSegment::SegmentTypeTerrainFrame) {
        double terrainDistance = 0;
        double distanceToSurface = segment->coord1AMSLAlt() - segment->amslTerrainHeights().first().value<double>();
        for (int heightIndex=0; heightIndex<segment->amslTerrainHeights().count(); heightIndex++) {
            // Move along the x axis which is distance
            if (heightIndex == 0) {
                // The first point in the segment is at the position of the last point. So nothing to do here.
            } else if (heightIndex == segment->amslTerrainHeights().count() - 2) {
                // The distance between the last two heights differs with each terrain query
                terrainDistance += segment->finalDistanceBetween();
            } else {
                // The distance between all terrain heights except for the last is the same
                terrainDistance += segment->distanceBetween();
            }

            if (heightIndex > 1) {
                // Add first coord of segment
                auto previousVertex = flightProfileVertices[flightProfileVertexIndex-1];
                flightProfileVertices[flightProfileVertexIndex++].set(previousVertex.x, previousVertex.y);
            }

            // Add second coord of segment (or very first one)
            double amslTerrainHeight    = segment->amslTerrainHeights()[heightIndex].value<double>() + distanceToSurface;
            double terrainHeightPercent = (amslTerrainHeight - _minAMSLAlt) / amslAltRange;

            float x = (currentDistance + terrainDistance) * _pixelsPerMeter;
            float y = height() - (terrainHeightPercent * height());
            flightProfileVertices[flightProfileVertexIndex++].set(x, y);

        }
    } else {
        double amslCoord1Height =       segment->coord1AMSLAlt();
        double amslCoord2Height =       segment->coord2AMSLAlt();
        double coord1HeightPercent =    (amslCoord1Height - _minAMSLAlt) / amslAltRange;
        double coord2HeightPercent =    (amslCoord2Height - _minAMSLAlt) / amslAltRange;

        float x = currentDistance * _pixelsPerMeter;
        float y = height() - (coord1HeightPercent * height());

        flightProfileVertices[flightProfileVertexIndex++].set(x, y);

        x += segment->totalDistance() * _pixelsPerMeter;
        y = height() - (coord2HeightPercent * height());

        flightProfileVertices[flightProfileVertexIndex++].set(x, y);
    }
}

QSGNode* TerrainProfile::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData* /*updatePaintNodeData*/)
{
    QSGNode*        rootNode =                  static_cast<QSGNode *>(oldNode);
    QSGGeometry*    terrainProfileGeometry =    nullptr;
    QSGGeometry*    missingTerrainGeometry =    nullptr;
    QSGGeometry*    flightProfileGeometry =     nullptr;
    QSGGeometry*    terrainCollisionGeometry =  nullptr;
    int             cTerrainProfilePoints =     0;
    int             cMissingTerrainSegments =   0;
    int             cFlightProfileSegments =    0;
    int             cTerrainCollisionSegments = 0;
    double          minTerrainHeight =          qQNaN();
    double          maxTerrainHeight =          qQNaN();

    // First we need to determine:
    //  - how many terrain profile vertices we need
    //  - how many missing terrain segments there are
    //  - how many flight profile segments we need
    //  - how many terrain collision segments there are
    //  - what is the total distance so we can calculate pixels per meter

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

    // The profile view min/max is setup to include a full terrain profile as well as the flight path segments.
    _minAMSLAlt = std::fmin(_missionController->minAMSLAltitude(), minTerrainHeight);
    _maxAMSLAlt = std::fmax(_missionController->maxAMSLAltitude(), maxTerrainHeight);

    // We add a buffer to the min/max alts such that the visuals don't draw lines right at the edges of the display
    double amslAltRange = _maxAMSLAlt - _minAMSLAlt;
    double amslAltRangeBuffer = amslAltRange * 0.1;
    _maxAMSLAlt += amslAltRangeBuffer;
    if (_minAMSLAlt > 0.0) {
        _minAMSLAlt -= amslAltRangeBuffer;
        _minAMSLAlt = std::fmax(_minAMSLAlt, 0.0);
    }
    amslAltRange = _maxAMSLAlt - _minAMSLAlt;

    static int counter = 0;
    qCDebug(TerrainProfileLog) << "missionController min/max" << _missionController->minAMSLAltitude() << _missionController->maxAMSLAltitude();
    qCDebug(TerrainProfileLog) << QStringLiteral("updatePaintNode counter:%1 cFlightProfileSegments:%2 cTerrainProfilePoints:%3 cMissingTerrainSegments:%4 cTerrainCollisionSegments:%5 _minAMSLAlt:%6 _maxAMSLAlt:%7 maxTerrainHeight:%8")
                                  .arg(counter++).arg(cFlightProfileSegments).arg(cTerrainProfilePoints).arg(cMissingTerrainSegments).arg(cTerrainCollisionSegments).arg(_minAMSLAlt).arg(_maxAMSLAlt).arg(maxTerrainHeight);

    _pixelsPerMeter = _visibleWidth / _missionController->missionDistance();

    // Instantiate nodes
    if (!rootNode) {
        rootNode = new QSGNode;

        QSGGeometryNode* terrainProfileNode =   nullptr;
        QSGGeometryNode* missingTerrainNode =   nullptr;
        QSGGeometryNode* flightProfileNode =    nullptr;
        QSGGeometryNode* terrainCollisionNode = nullptr;

        _createGeometry(terrainProfileNode,     terrainProfileGeometry,     QSGGeometry::DrawLineStrip, "green");
        _createGeometry(missingTerrainNode,     missingTerrainGeometry,     QSGGeometry::DrawLines,     "yellow");
        _createGeometry(flightProfileNode,      flightProfileGeometry,      QSGGeometry::DrawLines,     "orange");
        _createGeometry(terrainCollisionNode,   terrainCollisionGeometry,   QSGGeometry::DrawLines,     "red");

        rootNode->appendChildNode(terrainProfileNode);
        rootNode->appendChildNode(missingTerrainNode);
        rootNode->appendChildNode(flightProfileNode);
        rootNode->appendChildNode(terrainCollisionNode);
    }

    // Allocate space for the vertices

    QSGNode* node = rootNode->childAtIndex(0);
    terrainProfileGeometry = static_cast<QSGGeometryNode*>(node)->geometry();
    terrainProfileGeometry->allocate(cTerrainProfilePoints);
    node->markDirty(QSGNode::DirtyGeometry);

    node = rootNode->childAtIndex(1);
    missingTerrainGeometry = static_cast<QSGGeometryNode*>(node)->geometry();
    missingTerrainGeometry->allocate(cMissingTerrainSegments * 2);
    node->markDirty(QSGNode::DirtyGeometry);

    node = rootNode->childAtIndex(2);
    flightProfileGeometry = static_cast<QSGGeometryNode*>(node)->geometry();
    flightProfileGeometry->allocate(cFlightProfileSegments * 2);
    node->markDirty(QSGNode::DirtyGeometry);

    node = rootNode->childAtIndex(3);
    terrainCollisionGeometry = static_cast<QSGGeometryNode*>(node)->geometry();
    terrainCollisionGeometry->allocate(cTerrainCollisionSegments * 2);
    node->markDirty(QSGNode::DirtyGeometry);

    int                     flightProfileVertexIndex =          0;
    int                     terrainProfileVertexIndex =         0;
    int                     missingterrainProfileVertexIndex =  0;
    int                     terrainCollisionVertexIndex =       0;
    double                  currentDistance =                   0;
    QSGGeometry::Point2D*   flightProfileVertices =             flightProfileGeometry->vertexDataAsPoint2D();
    QSGGeometry::Point2D*   terrainProfileVertices =            terrainProfileGeometry->vertexDataAsPoint2D();
    QSGGeometry::Point2D*   missingTerrainVertices =            missingTerrainGeometry->vertexDataAsPoint2D();
    QSGGeometry::Point2D*   terrainCollisionVertices =          terrainCollisionGeometry->vertexDataAsPoint2D();

    // This step places the vertices for display into the nodes
    for (int viIndex=0; viIndex<_visualItems->count(); viIndex++) {
        VisualMissionItem*  visualItem =    _visualItems->value<VisualMissionItem*>(viIndex);
        ComplexMissionItem* complexItem =   _visualItems->value<ComplexMissionItem*>(viIndex);

        if (complexItem) {
            if (complexItem->flightPathSegments()->count() == 0) {
                currentDistance += complexItem->complexDistance();
            } else {
                for (int segmentIndex=0; segmentIndex<complexItem->flightPathSegments()->count(); segmentIndex++) {
                    FlightPathSegment* segment = complexItem->flightPathSegments()->value<FlightPathSegment*>(segmentIndex);

                    _addFlightProfileSegment    (segment, currentDistance, amslAltRange,    flightProfileVertices,      flightProfileVertexIndex);
                    _addTerrainProfileSegment   (segment, currentDistance, amslAltRange,    terrainProfileVertices,     terrainProfileVertexIndex);
                    _addMissingTerrainSegment   (segment, currentDistance,                  missingTerrainVertices,     missingterrainProfileVertexIndex);
                    _addTerrainCollisionSegment (segment, currentDistance, amslAltRange,    terrainCollisionVertices,   terrainCollisionVertexIndex);

                    currentDistance += segment->totalDistance();
                }
            }
        }

        if (visualItem->simpleFlightPathSegment()) {
            FlightPathSegment* segment = visualItem->simpleFlightPathSegment();

            _addFlightProfileSegment    (segment, currentDistance, amslAltRange,    flightProfileVertices,      flightProfileVertexIndex);
            _addTerrainProfileSegment   (segment, currentDistance, amslAltRange,    terrainProfileVertices,     terrainProfileVertexIndex);
            _addMissingTerrainSegment   (segment, currentDistance,                  missingTerrainVertices,     missingterrainProfileVertexIndex);
            _addTerrainCollisionSegment (segment, currentDistance, amslAltRange,    terrainCollisionVertices,   terrainCollisionVertexIndex);

            currentDistance += segment->totalDistance();
        }
    }

    setImplicitWidth(_visibleWidth/*(_totalDistance * pixelsPerMeter) + (_horizontalMargin * 2)*/);
    setWidth(implicitWidth());

    emit implicitWidthChanged();
    emit widthChanged();
    emit pixelsPerMeterChanged();
    emit minAMSLAltChanged();
    emit maxAMSLAltChanged();

    return rootNode;
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
