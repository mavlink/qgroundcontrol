/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef MissionManagerTest_H
#define MissionManagerTest_H

#include "UnitTest.h"
#include "MockLink.h"
#include "MissionManager.h"
#include "MultiSignalSpy.h"
#include "MissionControllerManagerTest.h"

#include <QGeoCoordinate>

class MissionManagerTest : public MissionControllerManagerTest
{
    Q_OBJECT
    
public:
    MissionManagerTest(void);
    
private slots:
    void _testWriteFailureHandlingPX4(void);
    void _testWriteFailureHandlingAPM(void);
    void _testReadFailureHandlingPX4(void);
    void _testReadFailureHandlingAPM(void);

private:
    void _roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode);
    void _writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode);
    void _testWriteFailureHandlingWorker(void);
    void _testReadFailureHandlingWorker(void);
    
    static const MissionControllerManagerTest::TestCase_t   _rgTestCases[];
    static const size_t                                     _cTestCases;
};

#endif
