/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
