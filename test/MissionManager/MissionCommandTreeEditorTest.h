#pragma once

#include "UnitTest.h"
#include "QGCMAVLink.h"

/// This unit test is meant to be used stand-alone to generate images for each mission item editor for review
class MissionCommandTreeEditorTest : public UnitTest
{
    Q_OBJECT

public:
    MissionCommandTreeEditorTest(void);

private slots:
    void testEditors(void);

private:
    void _testEditorsWorker(QGCMAVLink::FirmwareClass_t firmwareClass, QGCMAVLink::VehicleClass_t vehicleClass);
};
