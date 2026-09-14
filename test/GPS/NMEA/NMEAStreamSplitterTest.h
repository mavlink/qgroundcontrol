#pragma once

#include "UnitTest.h"

class NMEAStreamSplitterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _delayedChunksPreserveReceipts();
    void _preloadedInput();
    void _closeDuringDelivery();
    void _independentReads();
    void _queuedSentenceOwnsItsBytes();
    void _mixedBinaryAndFragmentedSentences();
    void _slowConsumerIsBounded();
    void _sourceDestructionClosesOutputs();
    void _destructionDuringDelivery();
};
