#pragma once

#include "UnitTest.h"

class LocalHttpTestServerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFragmentedRequestHeader();
};
