#pragma once

#include <memory>

#include <QtPositioning/QGeoCoordinate>

#include "TransectStyleComplexItemTestBase.h"

class MultiSignalSpy;
class CorridorScanComplexItem;

class CorridorScanComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT

public:
    CorridorScanComplexItemTest();

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testPathChanges();
    void _testItemGeneration();
    void _testItemCount();

private:
    void _waitForReadyForSave();
    QList<MAV_CMD> _createExpectedCommands(bool hasTurnaround, bool useConditionGate);
    void _testItemGenerationWorker(bool imagesInTurnaround, bool hasTurnaround, bool useConditionGate,
                                   const QList<MAV_CMD>& expectedCommands);

    std::unique_ptr<MultiSignalSpy> _multiSpyCorridorPolygon;
    CorridorScanComplexItem* _corridorItem = nullptr;
    QList<QGeoCoordinate> _polyLineVertices;

    static constexpr int _expectedTransectCount = 2;
    static constexpr double _corridorLineSegmentDistance = 200.0;
    static constexpr double _corridorWidth = 50.0;
};
