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

#ifndef SimpleMissionItemTest_H
#define SimpleMissionItemTest_H

#include "UnitTest.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"

#include <QGeoCoordinate>

/// Unit test for SimpleMissionItem
class SimpleMissionItemTest : public UnitTest
{
    Q_OBJECT
    
public:
    SimpleMissionItemTest(void);
    
private slots:
    void _testSignals(void);
    void _testEditorFacts(void);
    void _testDefaultValues(void);
    
private:

    typedef struct {
        MAV_CMD        command;
        MAV_FRAME      frame;
    } ItemInfo_t;
    
    typedef struct {
        const char* name;
        double      value;
    } FactValue_t;
    
    typedef struct {
        size_t              cFactValues;
        const FactValue_t*  rgFactValues;
        bool                relativeAltCheckbox;
    } ItemExpected_t;

    static const ItemInfo_t     _rgItemInfo[];
    static const ItemExpected_t _rgItemExpected[];
    static const FactValue_t    _rgFactValuesWaypoint[];
    static const FactValue_t    _rgFactValuesLoiterUnlim[];
    static const FactValue_t    _rgFactValuesLoiterTurns[];
    static const FactValue_t    _rgFactValuesLoiterTime[];
    static const FactValue_t    _rgFactValuesLand[];
    static const FactValue_t    _rgFactValuesTakeoff[];
    static const FactValue_t    _rgFactValuesConditionDelay[];
    static const FactValue_t    _rgFactValuesDoJump[];
};

#endif
