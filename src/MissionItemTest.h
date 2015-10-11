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

#ifndef MissionItemTest_H
#define MissionItemTest_H

#include "UnitTest.h"
#include "TCPLink.h"
#include "MultiSignalSpy.h"

#include <QGeoCoordinate>

/// @file
///     @brief FlightGear HIL Simulation unit tests
///
///     @author Don Gagne <don@thegagnes.com>

class MissionItemTest : public UnitTest
{
    Q_OBJECT
    
public:
    MissionItemTest(void);
    
private slots:
    void _test(void);
    
private:
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
        const char* name;
        double      value;
    } FactValue_t;
    
    typedef struct {
        const char*         streamString;
        size_t              cFactValues;
        const FactValue_t*  rgFactValues;
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
