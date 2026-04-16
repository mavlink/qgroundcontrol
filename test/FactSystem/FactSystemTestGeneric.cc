#include "FactSystemTestGeneric.h"

#include "QGCMAVLink.h"

/// FactSystem Unit Test for Generic autopilot

void FactSystemTestGeneric::init()
{
    UnitTest::init();
    _init(MAV_AUTOPILOT_GENERIC);
}

UT_REGISTER_TEST(FactSystemTestGeneric, TestLabel::Integration, TestLabel::Vehicle)
