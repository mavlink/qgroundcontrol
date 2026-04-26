#pragma once

#include "UnitTest.h"

class GStreamerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _testRedirectGLibLogging();
    void _testVerifyRequiredPlugins();
    void _testEnvironmentSetup();
    void _testCompleteInit();
    void _testStreamDeviceFeedsSequentialBytes();
    void _testStreamDeviceMatchesSequentialMediaPlayerContract();
    void _testIngestSessionReportsBusError();
    void _testRemuxPipelineReportsMissingPluginMessage();
    void _testNativeRecorderSelectedForIngestSources();
    void _testRecordingPolicyReportsSelectedBackend();
    void _testNativeRecorderWritesFinitePipeline();
    void _testNativeRecorderRejectsPipelineWithoutVideoPad();
    void _testNativeRecorderReportsMissingPluginMessage();
    void _testIngestSessionCanRecordSharedIngest();
    void _testPipelineSmokeTest();
    void _testRuntimeVersionCheck();
    void _testGray8PipelineEndToEnd();
};
