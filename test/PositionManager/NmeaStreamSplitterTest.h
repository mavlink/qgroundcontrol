#pragma once

#include "UnitTest.h"

class NmeaStreamSplitterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testPipesCreated();
    void testDataReachesPositionPipe();
    void testDataReachesSatellitePipe();
    void testReadyReadEmittedOnFeed();
    void testCanReadLineWithNewline();
    void testCanReadLineWithoutNewline();
    void testReadLineSingleSentence();
    void testMultipleSentencesSplitCorrectly();
    void testPartialThenCompleteSentence();
    void testEmptyDataDoesNotCrash();
    void testBytesAvailableReflectsBuffer();
};
