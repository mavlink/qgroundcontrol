/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FactSystemTestPX4.h"
#include "QGCMAVLink.h"

/// FactSystem Unit Test for PX4 autpilot
FactSystemTestPX4::FactSystemTestPX4(void)
{

}

void FactSystemTestPX4::init(void)
{
    UnitTest::init();
    _init(MAV_AUTOPILOT_PX4);
}
