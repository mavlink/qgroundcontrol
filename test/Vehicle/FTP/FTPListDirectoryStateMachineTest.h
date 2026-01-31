#pragma once

#include "UnitTest.h"

class FTPListDirectoryStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCreation();
    void _testMachineName();
    void _testStart();
    void _testCancel();
    void _testDoubleStartPrevented();
    void _testComponentIdHandling();
};
