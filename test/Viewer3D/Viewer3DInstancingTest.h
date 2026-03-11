#pragma once

#include "UnitTest.h"

class Viewer3DInstancing;

class Viewer3DInstancingTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitialState();
    void _testAddEntry();
    void _testAddMultipleEntries();
    void _testClear();
    void _testClearEmitsCountChanged();
    void _testGetInstanceBuffer();
    void _testGetInstanceBufferCaching();
    void _testGetInstanceBufferNullCount();
    void _testAddLineSegment();
    void _testAddLineSegmentVertical();
    void _testAddLineSegmentDegenerate();
    void _testAddLineSegmentDiagonal();
    void _testSelectedIndexDefault();
    void _testSelectedIndexHighlight();
    void _testSelectedIndexOutOfRange();
    void _testSelectedIndexSameValueNoop();
};
