#pragma once

#include "UnitTest.h"

class QGCCheckBoxSliderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _unconstrainedLabelStaysOnOneLine_test();
    void _narrowLabelWrapsBesideIndicator_test();
    void _noTextIsIndicatorOnly_test();
};
