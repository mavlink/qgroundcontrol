#pragma once

#include "UnitTest.h"

class Viewer3DManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitialState();
    void _testSetDisplayMode();
    void _testSetDisplayModeNoop();
};
