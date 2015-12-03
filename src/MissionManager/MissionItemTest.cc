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

const MissionItemTest::ItemInfo_t MissionItemTest::_rgItemInfo[] = {
    { MAV_CMD_NAV_WAYPOINT,     MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_UNLIM, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_TURNS, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_TIME,  MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LAND,         MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_TAKEOFF,      MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_CONDITION_DELAY,  MAV_FRAME_MISSION },
    { MAV_CMD_DO_JUMP,          MAV_FRAME_MISSION },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesWaypoint[] = {
    { "Altitude:",      70.1234567 },
    { "Hold:",          10.1234567 },
    { "Accept radius:", 20.1234567 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesLoiterUnlim[] = {
    { "Altitude:",  70.1234567 },
    { "Radius:",    30.1234567 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesLoiterTurns[] = {
    { "Altitude:",  70.1234567 },
    { "Radius:",    30.1234567 },
    { "Turns:",     10.1234567 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesLoiterTime[] = {
    { "Altitude:",  70.1234567 },
    { "Radius:",    30.1234567 },
    { "Hold:",      10.1234567 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesLand[] = {
    { "Altitude:",  70.1234567 },
    { "Abort Alt:", 10.1234567 },
    { "Heading:",   40.1234567 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesTakeoff[] = {
    { "Altitude:",  70.1234567 },
    { "Heading:",   40.1234567 },
    { "Pitch:",     10.1234567 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesConditionDelay[] = {
    { "Hold:", 10.1234567 },
};

const MissionItemTest::FactValue_t MissionItemTest::_rgFactValuesDoJump[] = {
    { "Item #:",    10.1234567 },
    { "Repeat:",    20.1234567 },
};

const MissionItemTest::ItemExpected_t MissionItemTest::_rgItemExpected[] = {
    { sizeof(MissionItemTest::_rgFactValuesWaypoint)/sizeof(MissionItemTest::_rgFactValuesWaypoint[0]),             MissionItemTest::_rgFactValuesWaypoint },
    { sizeof(MissionItemTest::_rgFactValuesLoiterUnlim)/sizeof(MissionItemTest::_rgFactValuesLoiterUnlim[0]),       MissionItemTest::_rgFactValuesLoiterUnlim },
    { sizeof(MissionItemTest::_rgFactValuesLoiterTurns)/sizeof(MissionItemTest::_rgFactValuesLoiterTurns[0]),       MissionItemTest::_rgFactValuesLoiterTurns },
    { sizeof(MissionItemTest::_rgFactValuesLoiterTime)/sizeof(MissionItemTest::_rgFactValuesLoiterTime[0]),         MissionItemTest::_rgFactValuesLoiterTime },
    { sizeof(MissionItemTest::_rgFactValuesLand)/sizeof(MissionItemTest::_rgFactValuesLand[0]),                     MissionItemTest::_rgFactValuesLand },
    { sizeof(MissionItemTest::_rgFactValuesTakeoff)/sizeof(MissionItemTest::_rgFactValuesTakeoff[0]),               MissionItemTest::_rgFactValuesTakeoff },
    { sizeof(MissionItemTest::_rgFactValuesConditionDelay)/sizeof(MissionItemTest::_rgFactValuesConditionDelay[0]), MissionItemTest::_rgFactValuesConditionDelay },
    { sizeof(MissionItemTest::_rgFactValuesDoJump)/sizeof(MissionItemTest::_rgFactValuesDoJump[0]),                 MissionItemTest::_rgFactValuesDoJump },
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
        
        MissionItem* item = new MissionItem(1,
                                            info->command,
                                            info->frame,
                                            10.1234567,
                                            20.1234567,
                                            30.1234567,
                                            40.1234567,
                                            50.1234567,
                                            60.1234567,
                                            70.1234567,
                                            true,
                                            false);

        // Validate the saving is working correctly

        QString savedItemString;
        QTextStream saveStream(&savedItemString, QIODevice::WriteOnly);
        item->save(saveStream);

        // Param floats to string with 18 digits or precision
        QString paramStrings = "10.1234567000000002\t"
                                "20.1234566999999984\t"
                                "30.1234566999999984\t"
                                "40.1234566999999984\t"
                                "50.1234566999999984\t"
                                "60.1234566999999984\t"
                                "70.1234567000000055";
        QString expectedItemString = QString("1\t0\t%1\t%2\t%3\t1\r\n").arg(info->frame).arg(info->command).arg(paramStrings);
        QCOMPARE(savedItemString, expectedItemString);
        
        // Validate that the fact values are correctly returned
        size_t factCount = 0;
        for (int i=0; i<item->textFieldFacts()->count(); i++) {
            Fact* fact = qobject_cast<Fact*>(item->textFieldFacts()->get(i));
            
            bool found = false;
            for (size_t j=0; j<expected->cFactValues; j++) {
                const FactValue_t* factValue = &expected->rgFactValues[j];
                
                if (factValue->name == fact->name()) {
                    QCOMPARE(fact->rawValue().toDouble(), factValue->value);
                    factCount ++;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                qDebug() << fact->name();
            }
            QVERIFY(found);
        }
        QCOMPARE(factCount, expected->cFactValues);
        
        // Validate that loading is working correctly
        MissionItem* loadedItem = new MissionItem();
        QTextStream loadStream(&savedItemString, QIODevice::ReadOnly);
        QVERIFY(loadedItem->load(loadStream));
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

void MissionItemTest::_testDefaultValues(void)
{
    MissionItem item;

    item.setCommand(MAV_CMD_NAV_WAYPOINT);
    item.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(item.param2(), 3.0);
    QCOMPARE(item.param7(), MissionItem::defaultAltitude);
}
