#pragma once

#include "UnitTest.h"

class RTCMFramerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _frameAccess_data();
    void _frameAccess();
    void _frameViewAndReset();
    void _implicitAdvance_data();
    void _implicitAdvance();
};
