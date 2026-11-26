#pragma once

#include "UnitTest.h"

class QGCWorkDirectoryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testLifetime();
    void _testRemoveStale();
};
