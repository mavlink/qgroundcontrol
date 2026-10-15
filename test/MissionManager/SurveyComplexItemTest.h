#pragma once

#include <memory>

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
    void _testMaxTransectCount();

private:
    double _clampGridAngle180(double gridAngle);
    QList<MAV_CMD> _createExpectedCommands(bool hasTurnaround, bool useConditionGate);
    void _testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate,
                                   const QList<MAV_CMD>& expectedCommands);

    std::unique_ptr<MultiSignalSpy> _multiSpy;
    SurveyComplexItem* _surveyItem = nullptr;
    QGCMapPolygon* _mapPolygon = nullptr;
    QList<QGeoCoordinate> _polyVertices;

    static const int _expectedTransectCount = 2;
};

class SurveyComplexItemObstacleTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testObstacleAvoidance_data();
    void _testObstacleAvoidance();
    void _testObstacleRouteEfficiency_data();
    void _testObstacleRouteEfficiency();
    void _testIgnoredObstacles_data();
    void _testIgnoredObstacles();
    void _testObstacleSegmentLimit();
    void _testHoverCaptureDelay_data();
    void _testHoverCaptureDelay();
    void _testObstacleChanges();
    void _testBlockedSurvey_data();
    void _testBlockedSurvey();
    void _testObstacleLoad();

private:
    SurveyComplexItem* _surveyItem = nullptr;
    QList<QGeoCoordinate> _polyVertices;
};
