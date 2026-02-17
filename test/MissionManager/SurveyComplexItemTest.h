#pragma once

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

#if 1
private slots:
    void _testDirty();
    void _testGridAngle();
    void _testEntryLocation();
    void _testItemGeneration();
    void _testItemCount();
    void _testHoverCaptureItemGeneration();
#else
    // Handy mechanism to to a single test
private slots:
    void _testItemCount();

private:
    void _testDirty();
    void _testGridAngle();
    void _testEntryLocation();
    void _testItemGeneration();
    void _testHoverCaptureItemGeneration();
#endif

private:
    double _clampGridAngle180(double gridAngle);
    QList<MAV_CMD> _createExpectedCommands(bool hasTurnaround, bool useConditionGate);
    void _testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate,
                                   const QList<MAV_CMD>& expectedCommands);

    MultiSignalSpy* _multiSpy = nullptr;
    SurveyComplexItem* _surveyItem = nullptr;
    QGCMapPolygon* _mapPolygon = nullptr;
    QList<QGeoCoordinate> _polyVertices;

    static const int _expectedTransectCount = 2;
};
