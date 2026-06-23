#pragma once

#include "QGCMAVLinkTypes.h"
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
    void _testCalcAboveTerrainSaveLoad();

private:
    void _testEditorFactsWorker(QGCMAVLinkTypes::VehicleClass_t vehicleClass, QGCMAVLinkTypes::VehicleClass_t vtolMode);
    bool _classMatch(QGCMAVLinkTypes::VehicleClass_t vehicleClass, QGCMAVLinkTypes::VehicleClass_t testClass);

    SimpleMissionItem* _simpleItem = nullptr;
    std::unique_ptr<MultiSignalSpy> _spySimpleItem;
    std::unique_ptr<MultiSignalSpy> _spyVisualItem;
};
