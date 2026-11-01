#pragma once

#include "UnitTest.h"

/// GStreamer init policy: how GLib's and GStreamer's logs reach Qt logging, which missing elements disable video, when
/// udpsrc's unlock race is patched, the decoder ranks the decoder setting plans for each platform and applies, except
/// those a user set with GST_PLUGIN_FEATURE_RANK, and what the boot self-test makes of a failing plugin scanner and of
/// a bundled runtime's plugins.
class GStreamerInitTest : public UnitTest
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testNativeDebugLogRequested_data();
    void _testNativeDebugLogRequested();
    void _testNativeDebugLogIsNotBridged();
    void _testConfigureDebugLoggingIsIdempotent();
    void _testConfigureDebugLoggingDefaultsToDisabled();
    void _testRedirectGLibLogging();
    void _testExternalPluginLoaderFailed();
    void _testMissingPluginFeatures_data();
    void _testMissingPluginFeatures();
    void _testUdpSrcUnlockSerializedBefore1_24_10();
    void _testBundledPluginIssues_data();
    void _testBundledPluginIssues();
    void _testUserFeatureRankKept_data();
    void _testUserFeatureRankKept();
    void _testPlanRanks_data();
    void _testPlanRanks();
    void _testScopedDecoderOptionRestoresThePreviousOption();
    void _testSetCodecPrioritiesDefault();
    void _testSetCodecPrioritiesDefaultPrefersMatchingD3DDecoder();
    void _testSetCodecPrioritiesSkipsAbsentD3DDecoders();
    void _testSetCodecPrioritiesSoftware();
    void _testSetCodecPrioritiesReplacesThePreviousOption();
    void _testSetCodecPrioritiesFamilyOutranksOtherDecoders();
    void _testSetCodecPrioritiesHardware();
#else
    void _testGStreamerDisabled();
#endif
};
