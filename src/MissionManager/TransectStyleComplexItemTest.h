/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"
#include "MultiSignalSpy.h"
#include "CorridorScanComplexItem.h"

#include <QGeoCoordinate>

class TransectStyleItem;

class TransectStyleComplexItemTest : public UnitTest
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
    void _testAltMode           (void);

private:
    void _setSurveyAreaPolygon  (void);
    void _adjustSurveAreaPolygon(void);

    enum {
        // These signals are from TransectStyleComplexItem
        cameraShotsChangedIndex = 0,
        timeBetweenShotsChangedIndex,
        visualTransectPointsChangedIndex,
        coveredAreaChangedIndex,
        // These signals are from ComplexItem
        dirtyChangedIndex,
        complexDistanceChangedIndex,
        greatestDistanceToChangedIndex,
        additionalTimeDelayChangedIndex,
        // These signals are from VisualMissionItem
        lastSequenceNumberChangedIndex,
        maxSignalIndex
    };

    enum {
        // These signals are from TransectStyleComplexItem
        cameraShotsChangedMask =                1 << cameraShotsChangedIndex,
        timeBetweenShotsChangedMask =           1 << timeBetweenShotsChangedIndex,
        visualTransectPointsChangedMask =       1 << visualTransectPointsChangedIndex,
        coveredAreaChangedMask =                1 << coveredAreaChangedIndex,
        // These signals are from ComplexItem
        dirtyChangedMask =                      1 << dirtyChangedIndex,
        complexDistanceChangedMask =            1 << complexDistanceChangedIndex,
        greatestDistanceToChangedMask =         1 << greatestDistanceToChangedIndex,
        additionalTimeDelayChangedMask =        1 << additionalTimeDelayChangedIndex,
        // These signals are from VisualMissionItem
        lastSequenceNumberChangedMask =         1 << lastSequenceNumberChangedIndex,
    };

    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];

    Vehicle*                _offlineVehicle;
    MultiSignalSpy*         _multiSpy;
    QList<QGeoCoordinate>   _polygonVertices;
    TransectStyleItem*      _transectStyleItem;
};

class TransectStyleItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    TransectStyleItem(Vehicle* vehicle, QObject* parent = nullptr);

    // Overrides from ComplexMissionItem
    QString mapVisualQML        (void) const final { return QString(); }
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final { Q_UNUSED(complexObject); Q_UNUSED(sequenceNumber); Q_UNUSED(errorString); return false; }

    // Overrides from VisualMissionItem
    void    save                (QJsonArray&  missionItems) final { Q_UNUSED(missionItems); }
    bool    specifiesCoordinate (void) const final { return true; }
    void    appendMissionItems  (QList<MissionItem*>& items, QObject* missionItemParent) final { Q_UNUSED(items); Q_UNUSED(missionItemParent); }
    void    applyNewAltitude    (double newAltitude) final { Q_UNUSED(newAltitude); }
    double  additionalTimeDelay (void) const final { return 0; }

    bool rebuildTransectsPhase1Called;
    bool recalcComplexDistanceCalled;
    bool recalcCameraShotsCalled;

private slots:
    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1    (void) final;
    void _recalcComplexDistance     (void) final;
    void _recalcCameraShots         (void) final;
};
