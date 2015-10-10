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

class MissionManagerTest : public UnitTest
{
    Q_OBJECT
    
public:
    MissionManagerTest(void);
    
private slots:
    void init(void);
    void cleanup(void);
    
    void _testWriteFailureHandling(void);
    void _testReadFailureHandling(void);
    
private:
    void _checkInProgressValues(bool inProgress);
    void _roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MissionManager::ErrorCode_t errorCode, bool failFirstTimeOnly);
    void _writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MissionManager::ErrorCode_t errorCode, bool failFirstTimeOnly);
    
    void _readEmptyVehicle(void);
    
    MockLink*       _mockLink;
    MissionManager* _missionManager;
    
    enum {
        canEditChangedSignalIndex = 0,
        newMissionItemsAvailableSignalIndex,
        inProgressChangedSignalIndex,
        errorSignalIndex,
        maxSignalIndex
    };
    
    enum {
        canEditChangedSignalMask =              1 << canEditChangedSignalIndex,
        newMissionItemsAvailableSignalMask =    1 << newMissionItemsAvailableSignalIndex,
        inProgressChangedSignalMask =           1 << inProgressChangedSignalIndex,
        errorSignalMask =                       1 << errorSignalIndex,
    };

    MultiSignalSpy*     _multiSpy;
    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];
    
    typedef struct {
        int            sequenceNumber;
        QGeoCoordinate coordinate;
        int            command;
        double         param1;
        double         param2;
        double         param3;
        double         param4;
        bool           autocontinue;
        bool           isCurrentItem;
        int            frame;
    } ItemInfo_t;
    
    typedef struct {
        const char*         itemStream;
        const ItemInfo_t    expectedItem;
    } TestCase_t;

    static const TestCase_t _rgTestCases[];
    static const int        _signalWaitTime = MissionManager::_ackTimeoutMilliseconds * MissionManager::_maxRetryCount * 2;
};

#endif
