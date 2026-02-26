#pragma once

#include "UnitTest.h"

class QGCSignalTransitionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testTransitionMachineAccessor();
    void _testTransitionMachineAccessorAbstract();
};
