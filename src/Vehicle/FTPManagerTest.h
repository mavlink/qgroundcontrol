/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class FTPManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _performTestCases(void);

private:
    typedef struct {
        const char* file;
    } TestCase_t;

    void _testCaseWorker(const TestCase_t& testCase);

    static const TestCase_t _rgTestCases[];
};
