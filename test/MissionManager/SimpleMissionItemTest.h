#pragma once

#include "QGCMAVLink.h"
#include "QGroundControlQmlGlobal.h"
#include "MultiSignalSpy.h"
#include "VisualMissionItemTest.h"

#include <memory>

class SimpleMissionItem;

/// Unit test for SimpleMissionItem

class SimpleMissionItemTest : public VisualMissionItemTest
{
    Q_OBJECT

public:
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
    std::unique_ptr<MultiSignalSpy> _spySimpleItem;
    std::unique_ptr<MultiSignalSpy> _spyVisualItem;
};
