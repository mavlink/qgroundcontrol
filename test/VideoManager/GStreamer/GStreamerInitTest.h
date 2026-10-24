#pragma once

#include "UnitTest.h"

/// GStreamer init policy: when GStreamer's log is bridged into Qt logging, which missing elements disable video, and
/// the ranks a user set with GST_PLUGIN_FEATURE_RANK.
class GStreamerInitTest : public UnitTest
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testNativeDebugLogRequested_data();
    void _testNativeDebugLogRequested();
    void _testNativeDebugLogIsNotBridged();
    void _testMissingPluginFeatures_data();
    void _testMissingPluginFeatures();
    void _testUserFeatureRankKept_data();
    void _testUserFeatureRankKept();
#else
    void _testGStreamerDisabled();
#endif
};
