#include "FactSystemTestPX4.h"

#include "QGCMAVLink.h"

/// FactSystem Unit Test for PX4 autpilot
void FactSystemTestPX4::init()
{
    UnitTest::init();
    _init(MAV_AUTOPILOT_PX4);
}

UT_REGISTER_TEST(FactSystemTestPX4, TestLabel::Integration, TestLabel::Vehicle)
