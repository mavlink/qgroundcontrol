#pragma once

#include "VisualMissionItemTest.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCMAVLink.h"

class SimpleMissionItem;

/// Unit test for SimpleMissionItem

class SimpleMissionItemTest : public VisualMissionItemTest
{
    Q_OBJECT

public:
    SimpleMissionItemTest() = default;

    void init() override;
    void cleanup() override;

private slots:
    void _testSignals();
    void _testEditorFacts();
    void _testDefaultValues();
    void _testCameraSectionDirty();
    void _testSpeedSectionDirty();
    void _testCameraSection();
    void _testSpeedSection();
    void _testAltitudePropogation();

private:
    void _testEditorFactsWorker(QGCMAVLink::VehicleClass_t vehicleClass, QGCMAVLink::VehicleClass_t vtolMode);
    bool _classMatch(QGCMAVLink::VehicleClass_t vehicleClass, QGCMAVLink::VehicleClass_t testClass);

    SimpleMissionItem* _simpleItem = nullptr;
    MultiSignalSpy* _spySimpleItem = nullptr;
    MultiSignalSpy* _spyVisualItem = nullptr;
};
