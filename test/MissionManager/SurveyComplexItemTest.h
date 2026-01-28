#pragma once

#include "TransectStyleComplexItemTestBase.h"

#include <QtPositioning/QGeoCoordinate>

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

private:
    double _clampGridAngle180(double gridAngle);
    QList<MAV_CMD> _createExpectedCommands(bool hasTurnaround, bool useConditionGate);
    void _testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate, const QList<MAV_CMD>& expectedCommands);

    MultiSignalSpy* _multiSpy = nullptr;
    SurveyComplexItem* _surveyItem = nullptr;
    QGCMapPolygon* _mapPolygon = nullptr;
    QList<QGeoCoordinate> _polyVertices;

    static constexpr int _expectedTransectCount = 2;
};
