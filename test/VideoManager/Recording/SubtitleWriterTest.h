#pragma once

#include "BaseClasses/VehicleTest.h"

class SubtitleWriterTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

private slots:
    void _testLayoutFollowsEachRecording();
    void _testFollowsActiveVehicle();
    void _testReadsBack();
};
