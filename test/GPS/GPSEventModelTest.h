#pragma once

#include "UnitTest.h"

class GPSEventModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testAppend();
    void testMaxSize();
    void testClear();
    void testRoleNames();
    void testDataRoles();
};
