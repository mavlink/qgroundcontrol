#pragma once

#include "UnitTest.h"

class MapEngineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testReportsSourceChange_data();
    void _testReportsSourceChange();
};
