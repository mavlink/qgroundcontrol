#pragma once

#include "UnitTest.h"

class VideoManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _videoUriOverrideDoesNotChangeSettings_test();
    void _videoOutputQmlTypeAvailableInUnitTestMode_test();
};
