#pragma once

#include "TransectStyleComplexItemTestBase.h"
#include "TransectStyleComplexItem.h"

#include <QGeoCoordinate>

class TestTransectStyleItem;
class MultiSignalSpy;
class PlanMasterController;

class TransectStyleComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT

public:
    TransectStyleComplexItemTest() = default;

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _testDirty();
    void _testRebuildTransects();
    void _testDistanceSignalling();
    void _testAltitudes();
    // void _testFollowTerrain();

private:
    MultiSignalSpy* _multiSpy = nullptr;
    TestTransectStyleItem* _transectStyleItem = nullptr;
};

class TestTransectStyleItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    TestTransectStyleItem(PlanMasterController* masterController);

    void adjustSurveAreaPolygon();

    // Overrides from ComplexMissionItem
    QString patternName() const final { return QString(); }
    QString mapVisualQML() const final { return QString(); }
    bool load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final { Q_UNUSED(complexObject); Q_UNUSED(sequenceNumber); Q_UNUSED(errorString); return false; }

    // Overrides from VisualMissionItem
    void save(QJsonArray& missionItems) final { Q_UNUSED(missionItems); }
    bool specifiesCoordinate() const final { return true; }
    double additionalTimeDelay() const final { return 0; }

    bool rebuildTransectsPhase1Called;
    bool recalcComplexDistanceCalled;
    bool recalcCameraShotsCalled;

private slots:
    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1() final;
    void _recalcCameraShots() final;
};
