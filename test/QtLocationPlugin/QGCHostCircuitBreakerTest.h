#pragma once

#include "UnitTest.h"

class QGCHostCircuitBreakerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testEmptyHostAlwaysAllowed();
    void _testBelowThresholdStaysClosed();
    void _testFifthFailureOpens();
    void _testSuccessResetsFailureCounter();
};
