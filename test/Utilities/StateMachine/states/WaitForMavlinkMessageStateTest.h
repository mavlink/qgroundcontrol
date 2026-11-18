#pragma once

#include "StateMachineTest.h"

class WaitForMavlinkMessageStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testCompletesOnFirstMatchingMessage);
};
