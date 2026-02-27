#include "FactSystemTestGeneric.h"

#include "QGCMAVLink.h"

/// FactSystem Unit Test for PX4 autpilot

void FactSystemTestGeneric::init()
{
    UnitTest::init();
    _init(MAV_AUTOPILOT_GENERIC);
}

UT_REGISTER_TEST(FactSystemTestGeneric, TestLabel::Integration, TestLabel::Vehicle)
