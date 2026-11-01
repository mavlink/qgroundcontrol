#pragma once

#include "UnitTest.h"

class VideoManagerInitTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _testReceiversNeedBackendAndWindow_data();
    void _testReceiversNeedBackendAndWindow();
    void _testBackendInitFailure();
    void _testWaitForBackendReady();
    void _testBackendSettingReplacesReceivers();
};
