#pragma once

#include "UnitTest.h"

class BluetoothLiveAdapterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _testLiveAdapterSelectionAndState();
    void _testLiveAdapterQueryApis();
    void _testLiveScanLifecycle();
};
