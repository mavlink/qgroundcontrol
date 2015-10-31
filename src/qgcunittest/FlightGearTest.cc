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

#include "FlightGearTest.h"
#include "QGCFlightGearLink.h"

/// @file
///     @brief FlightGearUnitTest HIL Simulation class
///
///     @author Don Gagne <don@thegagnes.com>

FlightGearUnitTest::FlightGearUnitTest(void)
{
    
}

/// @brief The QGCFlightGearLink::parseUIArguments method is fairly involved so we have a unit
//          test for it.
void FlightGearUnitTest::_parseUIArguments_test(void)
{
    typedef struct {
        const char* args;
        const char* expectedListAsChar;
        bool        expectedReturn;
    } ParseUIArgumentsTestCase_t;
    
    static const ParseUIArgumentsTestCase_t rgTestCases[] = {
        { "foo", "foo", true },
        { "foo bar", "foo|bar", true },
        { "--foo --bar", "--foo|--bar", true },
        { "foo=bar", "foo=bar", true },
        { "foo=bar bar=baz", "foo=bar|bar=baz", true },
        { "foo=\"bar\"", "foo=bar", true },
        { "foo=\"bar\" bar=\"baz\"", "foo=bar|bar=baz", true },
        { "foo=\"b ar\"", "foo=b ar", true },
        { "foo=\"b ar\" bar=\"b az\"", "foo=b ar|bar=b az", true },
        { "foo\"", NULL, false },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        const ParseUIArgumentsTestCase_t* testCase = &rgTestCases[i];
        QStringList returnedArgList;
        bool actualReturn = QGCFlightGearLink::parseUIArguments(testCase->args, returnedArgList);
        QCOMPARE(actualReturn, testCase->expectedReturn);
        if (actualReturn) {
            QStringList expectedArgList = QString(testCase->expectedListAsChar).split("|");
            if (returnedArgList != expectedArgList) {
                qDebug() << "About to fail: Returned" << returnedArgList << "Expected" << expectedArgList;
            }
            QVERIFY(returnedArgList == expectedArgList);
        }
    }
}
