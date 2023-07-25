/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TransectStyleComplexItemTestBase.h"
#include "MultiSignalSpyV2.h"
#include "CorridorScanComplexItem.h"
#include "PlanMasterController.h"

#include <QGeoCoordinate>

class TestTransectStyleItem;

class TransectStyleComplexItemTest : public TransectStyleComplexItemTestBase
{
    Q_OBJECT
    
public:
    TransectStyleComplexItemTest(void);

protected:
    void init(void) final;
    void cleanup(void) final;

private slots:
    void _testDirty             (void);
    void _testRebuildTransects  (void);
    void _testDistanceSignalling(void);
    void _testAltitudes         (void);
    void _testFollowTerrain     (void);

private:
    MultiSignalSpyV2*       _multiSpy =             nullptr;
    TestTransectStyleItem*  _transectStyleItem =    nullptr;
};

class TestTransectStyleItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    TestTransectStyleItem(PlanMasterController* masterController);

    void adjustSurveAreaPolygon(void);

    // Overrides from ComplexMissionItem
    QString patternName         (void) const final { return QString(); }
    QString mapVisualQML        (void) const final { return QString(); }
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final { Q_UNUSED(complexObject); Q_UNUSED(sequenceNumber); Q_UNUSED(errorString); return false; }

    // Overrides from VisualMissionItem
    void    save                (QJsonArray&  missionItems) final { Q_UNUSED(missionItems); }
    bool    specifiesCoordinate (void) const final { return true; }
    double  additionalTimeDelay (void) const final { return 0; }

    bool rebuildTransectsPhase1Called;
    bool recalcComplexDistanceCalled;
    bool recalcCameraShotsCalled;

private slots:
    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1    (void) final;
    void _recalcCameraShots         (void) final;
};
