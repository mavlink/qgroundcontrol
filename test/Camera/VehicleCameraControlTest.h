#pragma once

#include "UnitTest.h"

class MockLink;
class Vehicle;

class VehicleCameraControlTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void init() override;
    void cleanup() override;

    UT_PARAMETERIZED_TEST(_testCameraCapFlags);

private:
    MockLink* _mockLink = nullptr;
    Vehicle* _vehicle = nullptr;
};
