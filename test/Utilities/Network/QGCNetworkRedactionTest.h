#pragma once

#include "UnitTest.h"

class QGCNetworkRedactionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPreservesStreamIdentity();
    void _testRemovesUserInfo();
    void _testRedactsQueryValues();
    void _testHandlesRelativeAndInvalidInput();
    void _testQUrlOverload();
};
