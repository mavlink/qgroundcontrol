#pragma once

#include "UnitTest.h"

class ScheduledTaskTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _replacementAndReentrantScheduling();
    void _dependencyLifetime_data();
    void _dependencyLifetime();
    void _productionDefersCallbacks();
};
