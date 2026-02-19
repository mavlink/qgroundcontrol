#pragma once

#include "UnitTest.h"

class QGCVideoStreamInfoTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _aspectRatioFromResolution_test();
    void _aspectRatioDefaultsToOneForNullResolution_test();
    void _updateNoChange_test();
    void _updateChangedFieldsEmitsSignal_test();
};

