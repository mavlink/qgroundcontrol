#pragma once

#include "StateMachineTest.h"

class ShowAppMessageStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testShowAppMessageStateCreation();
    void _testShowAppMessageStateAdvance();
};
