#pragma once

#include "UnitTest.h"

class VideoManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _videoOutputQmlTypeAvailableInUnitTestMode_test();
    void _saveImageFromQml_test();
    void _saveImageRejectsEmptyInput_test();
};
