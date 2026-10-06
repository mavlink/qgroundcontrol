#pragma once

#include "UnitTest.h"

class FactControlsUnitsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _factTextFieldFocusOutKeepsRawValue_test();
    void _factSliderTracksUnitsChangeAfterDrag_test();
    void _factSliderRepositionsOnUnitsChange_test();
};
