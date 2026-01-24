#pragma once

#include "TestFixtures.h"
#include "QGCMAVLink.h"

/// This unit test is meant to be used stand-alone to generate images for each mission item editor for review.
/// Uses OfflineTest since it doesn't require a vehicle connection.
class MissionCommandTreeEditorTest : public OfflineTest
{
    Q_OBJECT

public:
    MissionCommandTreeEditorTest() = default;

private slots:
    void testEditors();

private:
    void _testEditorsWorker(QGCMAVLink::FirmwareClass_t firmwareClass, QGCMAVLink::VehicleClass_t vehicleClass);
};
