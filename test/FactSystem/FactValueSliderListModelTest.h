#pragma once

#include "UnitTest.h"

class FactValueSliderListModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _resetInitialValueBasic_test();
    void _rowCountMatchesTotalValues_test();
    void _valueAtInitialIndex_test();
    void _valueAtModelIndexBoundaries_test();
    void _valueIndexAtModelIndex_test();
    void _roleNames_test();
    void _resetClearsOldRows_test();
    void _integerIncrement_test();
};
