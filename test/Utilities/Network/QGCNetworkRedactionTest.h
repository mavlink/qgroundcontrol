#pragma once

#include "UnitTest.h"

class QGCNetworkRedactionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testRedactedUrlForLogging();
};
