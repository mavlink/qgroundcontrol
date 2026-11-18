#pragma once

#include "GStreamerTestBase.h"

/// The GStreamer facade without a running pipeline: URI checks, decoder classification, recording sinks and init.
class GStreamerTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testIsHardwareDecoderFactory_data();
    void _testIsHardwareDecoderFactory();
    void _testDecoderFamilyForFactory_data();
    void _testDecoderFamilyForFactory();
    void _testAvailableDecoderFamilies();
    void _testStoredDecoderOption_data();
    void _testStoredDecoderOption();
    void _testDecoderPriorityWarnings();
    void _testLogDecoderRanks();
    void _testPipelineDotOmitsElementProperties();
    void _testCompleteInit();
    void _testCompleteInitNamesBlacklistedPlugins();
    void _testVideoBackendForwards();
    void _testVideoBackendRunsInOrder();
    void _testRecordingSinkAcceptsElementaryStreams_data();
    void _testRecordingSinkAcceptsElementaryStreams();
    void _testRuntimeVersionCheck();
};
