/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
