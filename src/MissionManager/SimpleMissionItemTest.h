/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "VisualMissionItemTest.h"
#include "SimpleMissionItem.h"

/// Unit test for SimpleMissionItem

typedef struct {
    MAV_CMD        command;
    MAV_FRAME      frame;
} ItemInfo_t;

typedef struct {
    const char*                 name;
    QGCMAVLink::VehicleClass_t  vehicleClass;
    bool                        nanValue;
    int                         paramIndex;
} FactValue_t;

typedef struct {
    size_t                          cFactValues;
    const FactValue_t*              rgFactValues;
    double                          altValue;
    QGroundControlQmlGlobal::AltMode altMode;
} ItemExpected_t;

class SimpleMissionItemTest : public VisualMissionItemTest
{
    Q_OBJECT
    
public:
    SimpleMissionItemTest(void);

    void init   (void) override;
    void cleanup(void) override;

private slots:
    void _testSignals               (void);
    void _testEditorFacts           (void);
    void _testDefaultValues         (void);
    void _testCameraSectionDirty    (void);
    void _testSpeedSectionDirty     (void);
    void _testCameraSection         (void);
    void _testSpeedSection          (void);
    void _testAltitudePropogation   (void);

private:
    enum {
        commandChangedIndex = 0,
        altitudeModeChangedIndex,
        friendlyEditAllowedChangedIndex,
        headingDegreesChangedIndex,
        rawEditChangedIndex,
        cameraSectionChangedIndex,
        speedSectionChangedIndex,
        maxSignalIndex,
    };

    enum {
        commandChangedMask =                        1 << commandChangedIndex,
        altitudeModeChangedMask =                  1 << altitudeModeChangedIndex,
        friendlyEditAllowedChangedMask =            1 << friendlyEditAllowedChangedIndex,
        headingDegreesChangedMask =                 1 << headingDegreesChangedIndex,
        rawEditChangedMask =                        1 << rawEditChangedIndex,
        cameraSectionChangedMask =                  1 << cameraSectionChangedIndex,
        speedSectionChangedMask =                   1 << speedSectionChangedIndex,
    };

    static const size_t cSimpleItemSignals = maxSignalIndex;
    const char*         rgSimpleItemSignals[cSimpleItemSignals];

    void _testEditorFactsWorker (QGCMAVLink::VehicleClass_t vehicleClass, QGCMAVLink::VehicleClass_t vtolMode, const ItemExpected_t* rgExpected);
    bool _classMatch            (QGCMAVLink::VehicleClass_t vehicleClass, QGCMAVLink::VehicleClass_t testClass);

    SimpleMissionItem*  _simpleItem;
    MultiSignalSpy*     _spySimpleItem;
    MultiSignalSpy*     _spyVisualItem;
};
