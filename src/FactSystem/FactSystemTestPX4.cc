/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FactSystemTestPX4.h"
#include "LinkManager.h"
#include "MockLink.h"
#include "AutoPilotPluginManager.h"
#include "UASManager.h"
#include "QGCApplication.h"
#include "QGCQuickWidget.h"

#include <QQuickItem>

UT_REGISTER_TEST(FactSystemTestPX4)

/// FactSystem Unit Test for PX4 autpilot
FactSystemTestPX4::FactSystemTestPX4(void)
{
    
}

void FactSystemTestPX4::init(void)
{
    UnitTest::init();
    _init(MAV_AUTOPILOT_PX4);
}
