#pragma once

#include "UnitTest.h"

class MAVLinkChartControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _constructionTest();
    void _defaultRangeValuesTest();
    void _chartFieldsInitiallyEmptyTest();
    void _setRangeXIndexTest();
    void _setRangeXIndexSameValueNoSignalTest();
    void _setRangeYIndexTest();
    void _setRangeYIndexSameValueNoSignalTest();
    void _setInspectorControllerTest();
};
