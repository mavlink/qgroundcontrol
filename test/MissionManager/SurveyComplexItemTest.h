#pragma once

#include <memory>

#include <QtGui/QPolygonF>
#include <QtPositioning/QGeoCoordinate>

#include "TransectStyleComplexItemTestBase.h"

class SurveyComplexItem;
class QGCMapPolygon;
class MultiSignalSpy;

/// Unit test for SurveyComplexItem
class SurveyComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT

public:
    SurveyComplexItemTest();

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testGridAngle();
    void _testEntryLocation();
    void _testItemGeneration();
    void _testItemCount();
    void _testHoverCaptureItemGeneration();
    void _testCameraShots();
    void _testMaxTransectCount();
    void _testSplitConcavePolygon();
    void _testSplitConcaveTurnarounds();
    void _testSplitConcaveVisualIndices();
    void _testSplitConcaveCameraShots();
    void _testSplitConcaveSaveLoad();
    void _testSplitConcaveLegacyLoad();

private:
    double _clampGridAngle180(double gridAngle);
    static QPolygonF _concaveTestPolygon();
    static QPointF _toLocalPoint(const QGeoCoordinate& coordinate);
    void _setLocalSurveyPolygon(const QPolygonF& localPolygon);
    void _setupSplitConcaveSurvey();
    QList<MAV_CMD> _createExpectedCommands(bool hasTurnaround, bool useConditionGate);
    void _testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate,
                                   const QList<MAV_CMD>& expectedCommands);

    std::unique_ptr<MultiSignalSpy> _multiSpy;
    SurveyComplexItem* _surveyItem = nullptr;
    QGCMapPolygon* _mapPolygon = nullptr;
    QList<QGeoCoordinate> _polyVertices;

    static const int _expectedTransectCount = 2;
};
