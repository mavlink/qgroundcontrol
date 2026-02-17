#pragma once

#include "QGCMAVLink.h"
#include "UnitTest.h"

/// This unit test is meant to be used stand-alone to generate images for each mission item editor for review
class MissionCommandTreeEditorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testEditors();

private:
    void _testEditorsWorker(QGCMAVLink::FirmwareClass_t firmwareClass, QGCMAVLink::VehicleClass_t vehicleClass);
};
