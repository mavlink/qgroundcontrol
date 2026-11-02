#pragma once

#include "UnitTest.h"

class VideoScreenshotTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _testJpegCarriesExif();
    void _testPositionWithoutAltitude();
    void _testNoExifWithoutValues();
    void _testOtherFormatsWithoutExif();
};
