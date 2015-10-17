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

#include "MissionItemTest.h"
#include "MissionItem.h"

UT_REGISTER_TEST(MissionItemTest)

const MissionItemTest::ItemInfo_t MissionItemTest::_rgItemInfo[] = {
    { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_WAYPOINT,         10.0, 20.0, 30.0, 1.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_UNLIM,     10.0, 20.0, 30.0, 1.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TURNS,     10.0, 20.0, 30.0, 1.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TIME,      10.0, 20.0, 30.0, 1.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LAND,             10.0, 20.0, 30.0, 1.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_TAKEOFF,          10.0, 20.0, 30.0, 1.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_CONDITION_DELAY,      10.0, 20.0, 30.0, 1.0, true, false, MAV_FRAME_MISSION },
    { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_DO_JUMP,              10.0, 20.0, 30.0, 1.0, true, false, MAV_FRAME_MISSION },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesWaypoint[] = {
    { "Altitude:",  -30.0 },
    { "Hold:",      10.0 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesLoiterUnlim[] = {
    { "Altitude:",  -30.0 },
    { "Radius:",    30.0 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesLoiterTurns[] = {
    { "Altitude:",  -30.0 },
    { "Radius:",    30.0 },
    { "Turns:",     10.0 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesLoiterTime[] = {
    { "Altitude:",  -30.0 },
    { "Radius:",    30.0 },
    { "Seconds:",   10.0 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesLand[] = {
    { "Altitude:",  -30.0 },
    { "Heading:",   1.0 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesTakeoff[] = {
    { "Altitude:",  -30.0 },
    { "Heading:",   1.0 },
    { "Pitch:",     10.0 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesConditionDelay[] = {
    { "Seconds:",   10.0 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesDoJump[] = {
    { "Seq #:",   10.0 },
    { "Repeat:",  20.0 },
};

const MissionItemTest::ItemExpected_t MissionItemTest::_rgItemExpected[] = {
    { "1\t0\t3\t16\t10\t20\t30\t1\t-10\t-20\t-30\t1\r\n",  sizeof(MissionItemTest::_rgFactValuesWaypoint)/sizeof(MissionItemTest::_rgFactValuesWaypoint[0]),             MissionItemTest::_rgFactValuesWaypoint },
    { "1\t0\t3\t17\t10\t20\t30\t1\t-10\t-20\t-30\t1\r\n",  sizeof(MissionItemTest::_rgFactValuesLoiterUnlim)/sizeof(MissionItemTest::_rgFactValuesLoiterUnlim[0]),       MissionItemTest::_rgFactValuesLoiterUnlim },
    { "1\t0\t3\t18\t10\t20\t30\t1\t-10\t-20\t-30\t1\r\n",  sizeof(MissionItemTest::_rgFactValuesLoiterTurns)/sizeof(MissionItemTest::_rgFactValuesLoiterTurns[0]),       MissionItemTest::_rgFactValuesLoiterTurns },
    { "1\t0\t3\t19\t10\t20\t30\t1\t-10\t-20\t-30\t1\r\n",  sizeof(MissionItemTest::_rgFactValuesLoiterTime)/sizeof(MissionItemTest::_rgFactValuesLoiterTime[0]),         MissionItemTest::_rgFactValuesLoiterTime },
    { "1\t0\t3\t21\t10\t20\t30\t1\t-10\t-20\t-30\t1\r\n",  sizeof(MissionItemTest::_rgFactValuesLand)/sizeof(MissionItemTest::_rgFactValuesLand[0]),                     MissionItemTest::_rgFactValuesLand },
    { "1\t0\t3\t22\t10\t20\t30\t1\t-10\t-20\t-30\t1\r\n",  sizeof(MissionItemTest::_rgFactValuesTakeoff)/sizeof(MissionItemTest::_rgFactValuesTakeoff[0]),               MissionItemTest::_rgFactValuesTakeoff },
    { "1\t0\t2\t112\t10\t20\t30\t1\t-10\t-20\t-30\t1\r\n", sizeof(MissionItemTest::_rgFactValuesConditionDelay)/sizeof(MissionItemTest::_rgFactValuesConditionDelay[0]), MissionItemTest::_rgFactValuesConditionDelay },
    { "1\t0\t2\t177\t10\t20\t30\t1\t-10\t-20\t-30\t1\r\n", sizeof(MissionItemTest::_rgFactValuesDoJump)/sizeof(MissionItemTest::_rgFactValuesDoJump[0]),                 MissionItemTest::_rgFactValuesDoJump },
};

MissionItemTest::MissionItemTest(void)
{
    
}

void MissionItemTest::_test(void)
{
    for (size_t i=0; i<sizeof(_rgItemInfo)/sizeof(_rgItemInfo[0]); i++) {
        const ItemInfo_t* info = &_rgItemInfo[i];
        const ItemExpected_t* expected = &_rgItemExpected[i];
        
        qDebug() << "Command:" << info->command;
        
        MissionItem* item = new MissionItem(NULL,
                                            info->sequenceNumber,
                                            info->coordinate,
                                            info->command,
                                            info->param1,
                                            info->param2,
                                            info->param3,
                                            info->param4,
                                            info->autocontinue,
                                            info->isCurrentItem,
                                            info->frame);
        
        // Validate the saving is working correctly
        QString savedItemString;
        QTextStream saveStream(&savedItemString, QIODevice::WriteOnly);
        item->save(saveStream);
        QCOMPARE(savedItemString, QString(expected->streamString));
        
        // Validate that the fact values are correctly returned
        size_t factCount = 0;
        for (int i=0; i<item->textFieldFacts()->count(); i++) {
            Fact* fact = qobject_cast<Fact*>(item->textFieldFacts()->get(i));
            
            bool found = false;
            for (size_t j=0; j<expected->cFactValues; j++) {
                const FactValue_t* factValue = &expected->rgFactValues[j];
                
                if (factValue->name == fact->name()) {
                    qDebug() << factValue->name;
                    if (strcmp(factValue->name,  "Heading:") == 0) {
                        QCOMPARE(fact->value().toDouble() * (M_PI / 180.0), item->_yawRadians());
                    } else {
                        QCOMPARE(fact->value().toDouble(), factValue->value);
                    }
                    factCount ++;
                    found = true;
                    break;
                }
            }
            
            QVERIFY(found);
        }
        qDebug() << info->command;
        QCOMPARE(factCount, expected->cFactValues);
        
        // Validate that loading is working correctly
        MissionItem* loadedItem = new MissionItem();
        QTextStream loadStream(&savedItemString, QIODevice::ReadOnly);
        QVERIFY(loadedItem->load(loadStream));
        //qDebug() << savedItemString;
        QCOMPARE(loadedItem->coordinate().latitude(), item->coordinate().latitude());
        QCOMPARE(loadedItem->coordinate().longitude(), item->coordinate().longitude());
        QCOMPARE(loadedItem->coordinate().altitude(), item->coordinate().altitude());
        QCOMPARE(loadedItem->command(), item->command());
        QCOMPARE(loadedItem->param1(), item->param1());
        QCOMPARE(loadedItem->param2(), item->param2());
        QCOMPARE(loadedItem->param3(), item->param3());
        QCOMPARE(loadedItem->param4(), item->param4());
        QCOMPARE(loadedItem->autoContinue(), item->autoContinue());
        QCOMPARE(loadedItem->isCurrentItem(), item->isCurrentItem());
        QCOMPARE(loadedItem->frame(), item->frame());

        delete item;
        delete loadedItem;
    }
}
