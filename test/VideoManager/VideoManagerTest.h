#pragma once

#include <memory>

#include "UnitTest.h"

class FakeVideoReceiver;
class VideoManager;
class VideoSettings;
class VideoStreamController;

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
    void _testStreamSettingsGroupFollowsLowLatency();
    void _testStopVideoCancelsEverything();
    void _testAutoStreamLeavesSettingsAlone();
    void _testThermalOnlyChangeReconfiguresThermal();
    UT_PARAMETERIZED_TEST(_testOtherStreamsPlayInTiles);
    void _testStateChangeWhileTileGoesIsTakenUp();
    void _testStreamInTileBecomesPrimaryAsItIs();
    void _testTilesStopAndStartWithTheVideo();
    UT_PARAMETERIZED_TEST(_testCameraSourcePlaysVideoSourceOrTileCamera);
    void _testSelectedCameraTileTakesMainPicture();
    void _testDisableWhenDisarmed_data();
    void _testDisableWhenDisarmed();
    void _testPlaceholderTellsWhyThereIsNoVideo_data();
    void _testPlaceholderTellsWhyThereIsNoVideo();
    void _testVideoOutputRegistration();
    void _testBackgroundSuspendsDisplay();
    void _testQmlVideoOutputRegistersItself();
    void _testScreenshotsComeFromStreamsOnDisplay();
    void _testScreenshotFailuresReachUser_data();
    void _testScreenshotFailuresReachUser();
    void _testAspectRatioWithoutAdvertisedResolution();
    void _testAdvertisedRotationTurnsVideo_data();
    void _testAdvertisedRotationTurnsVideo();
    void _testSourceErrorsReachUser();
    void _testReceiverSetWhileOldOneGoes();
    void _testReconnectingFollowsLostStream();
    void _testLinkRegainedReconnectsStreams();
    void _testUvcCameraFormatReachesCamera();
    void _testUvcControlsReachCamera();
    void _testRecordingEncodingReachesEverySource();
    void _testStreamStatsReachQml();
    void _testIndicatorReportsMeasuredSource();
    void _testSourceFallingShortDegradesVideo_data();
    void _testSourceFallingShortDegradesVideo();
    void _testSourceStatsShowMeasuredSources();
    void _testVideoViewFitsVideo_data();
    void _testVideoViewFitsVideo();
    void _testTelemetryOverlayFollowsTelemetry();
    void _testCameraPinchZoomSteps();
    void _testVideoViewPlacesThermalPip();
    void _testVideoViewShowsOtherSourcesInTiles();
    void _testTappedTileBecomesMainPicture();
    void _testTilesOutlastViewOnItsWayToFullSize();
    void _testFlyViewLeavesTapsOnTilesToThem();
    void _testThermalDecodesOnlyWhileShown();
    void _testThermalPlaysOnlyWhileInUse();
    void _testStreamUriAndTransport_data();
    void _testStreamUriAndTransport();

private:
    /// Runs the queued stream updates.
    static void _settle();
    void _resetCounters();
    /// The receiver of @p source, which the test's factory made.
    static FakeVideoReceiver* _receiverOf(const VideoStreamController* source);

    VideoSettings* _videoSettings = nullptr;
    std::unique_ptr<VideoTestHelpers::FactRestorer> _facts;
    std::unique_ptr<VideoManager> _videoManager;
    FakeVideoReceiver* _primary = nullptr;
    FakeVideoReceiver* _thermal = nullptr;
};
