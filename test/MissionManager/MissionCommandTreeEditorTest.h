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
