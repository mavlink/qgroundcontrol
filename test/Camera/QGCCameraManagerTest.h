#pragma once

#include "BaseClasses/VehicleTest.h"

class QGCCameraManagerTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _testCameraList();
    void _testLostCameraCleanupWithPendingRequest();
};
