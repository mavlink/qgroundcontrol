#pragma once

#include "UnitTest.h"

class LogManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _buildEntry();
    void _captureMessages();
    void _captureByCategory();
    void _hasCapturedWarning();
    void _hasCapturedCritical();
    void _hasCapturedUncategorized();
    void _categoryLogLevelNames();
};
