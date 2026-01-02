/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FactSystemTestGeneric.h"
#include "QGCMAVLink.h"

/// FactSystem Unit Test for PX4 autpilot
FactSystemTestGeneric::FactSystemTestGeneric(void)
{

}

void FactSystemTestGeneric::init(void)
{
    UnitTest::init();
    _init(MAV_AUTOPILOT_GENERIC);
}
