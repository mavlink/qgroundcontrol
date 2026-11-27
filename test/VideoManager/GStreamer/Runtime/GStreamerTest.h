#pragma once

#include "GStreamerTestBase.h"

/// The GStreamer facade without a running pipeline: decoder classification, init and the VideoBackend that forwards to
/// it.
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
    void _testLogDecoderRanks();
    void _testCompleteInitNamesBlacklistedPlugins();
    void _testVideoBackendForwards();
    void _testVideoBackendRunsInOrder();
};
