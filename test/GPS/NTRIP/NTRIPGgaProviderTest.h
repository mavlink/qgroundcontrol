#pragma once

#include "UnitTest.h"

class NTRIPGgaProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSourceClearedOnStopAndFreshStart();
    void testDefaultRTKBaseProvider();
};
