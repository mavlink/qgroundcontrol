#pragma once

#include <memory>

#include "UnitTest.h"

class FakeVideoReceiver;
class VideoManager;
class VideoSettings;

namespace VideoTestHelpers {
class FactRestorer;
}

class VideoManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testSettingReachesReceiver_data();
    void _testSettingReachesReceiver();
    void _testSettingChangesCoalesce();
    void _testRestartingSettingsRestartStreams_data();
    void _testRestartingSettingsRestartStreams();
    void _testDecodingSettingsGroupRestartsStreams();
    void _testStreamEnabledControlsVideo();
    void _testStopVideoCancelsEverything();
    void _testAutoStreamLeavesSettingsAlone();
    void _testThermalOnlyChangeReconfiguresThermal();
    void _testDisableWhenDisarmed_data();
    void _testDisableWhenDisarmed();
    void _testVideoOutputRegistration();
    void _testQmlVideoOutputRegistersItself();
    void _testScreenshotsComeFromStreamsOnDisplay();
    void _testStreamErrorsReachUser();
    void _testStreamStatsReachQml();
    void _testStreamFormatText();
    void _testVideoViewFitsVideo_data();
    void _testVideoViewFitsVideo();
    void _testVideoViewPlacesThermalPip();
    void _testStreamUriAndTransport_data();
    void _testStreamUriAndTransport();

private:
    /// Runs the queued stream updates.
    static void _settle();
    void _resetCounters();

    VideoSettings* _videoSettings = nullptr;
    std::unique_ptr<VideoTestHelpers::FactRestorer> _facts;
    std::unique_ptr<VideoManager> _videoManager;
    FakeVideoReceiver* _primary = nullptr;
    FakeVideoReceiver* _thermal = nullptr;
};
