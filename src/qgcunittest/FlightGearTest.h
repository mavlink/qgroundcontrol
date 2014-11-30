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

#ifndef TCPLINKTEST_H
#define TCPLINKTEST_H

#include "UnitTest.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"

/// @file
///     @brief FlightGear HIL Simulation unit tests
///
///     @author Don Gagne <don@thegagnes.com>

class FlightGearUnitTest : public UnitTest
{
    Q_OBJECT
    
public:
    FlightGearUnitTest(void);
    
private slots:
    void _parseUIArguments_test(void);
};

#endif
