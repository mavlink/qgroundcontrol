/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
