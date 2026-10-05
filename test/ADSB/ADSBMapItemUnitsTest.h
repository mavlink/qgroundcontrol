#pragma once

#include "UnitTest.h"

class ADSBMapItemUnitsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _altitudeLabelFollowsUnitsChange_data();
    void _altitudeLabelFollowsUnitsChange();
};
