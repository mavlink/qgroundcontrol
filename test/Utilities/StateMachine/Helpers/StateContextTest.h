#pragma once

#include "UnitTest.h"

class StateContextTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testSetAndGet();
    void _testGetOr();
    void _testContains();
    void _testContainsType();
    void _testRemove();
    void _testClear();
    void _testKeys();
    void _testTypeMismatch();
    void _testVariantApi();
    void _testContextFromState();
    void _testDataPassingBetweenStates();
    void _testStateContextAccessor();
};
