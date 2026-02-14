#pragma once

#include "UnitTest.h"

class ConditionalStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testConditionalStateExecute();
    void _testConditionalStateSkip();
};
