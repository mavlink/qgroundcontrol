#pragma once

#include "StateMachineTest.h"

class ConditionalStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testConditionalStateExecute();
    void _testConditionalStateSkip();
};
