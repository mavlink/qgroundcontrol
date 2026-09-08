#pragma once

#include "UnitTest.h"

class SerialGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testReadAbortsWhenStopRequested();
    void _testWriteAbortsWhenStopRequested();
};
