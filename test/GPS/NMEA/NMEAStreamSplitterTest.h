#pragma once

#include "UnitTest.h"

class NMEAStreamSplitterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _independentReads();
    void _slowConsumerIsBounded();
    void _sourceDestructionClosesOutputs();
    void _destructionDuringDelivery();
};
