#pragma once

#include "UnitTest.h"

class ScaleBarUnitsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _scaleTextFollowsUnitsChange_data();
    void _scaleTextFollowsUnitsChange();
};
