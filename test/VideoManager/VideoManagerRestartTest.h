#pragma once

#include "UnitTest.h"

class VideoManagerRestartTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testRestartStartsOrRestarts();
    void _testStopVideoDeactivatesReceivers();
};
