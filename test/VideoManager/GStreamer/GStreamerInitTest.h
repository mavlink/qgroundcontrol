#pragma once

#include "UnitTest.h"

/// GStreamer init policy: how GLib's and GStreamer's logs reach Qt logging, which missing elements disable video, and
/// the decoder ranks the decoder setting applies, except those a user set with GST_PLUGIN_FEATURE_RANK.
class GStreamerInitTest : public UnitTest
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testNativeDebugLogRequested_data();
    void _testNativeDebugLogRequested();
    void _testNativeDebugLogIsNotBridged();
    void _testConfigureDebugLoggingIsIdempotent();
    void _testRedirectGLibLogging();
    void _testMissingPluginFeatures_data();
    void _testMissingPluginFeatures();
    void _testUserFeatureRankKept_data();
    void _testUserFeatureRankKept();
    void _testSetCodecPrioritiesDefault();
    void _testSetCodecPrioritiesDefaultPrefersMatchingD3DDecoder();
    void _testSetCodecPrioritiesSkipsAbsentD3DDecoders();
    void _testSetCodecPrioritiesSoftware();
    void _testSetCodecPrioritiesHardware();
#else
    void _testGStreamerDisabled();
#endif
};
