#pragma once

#include "VideoManagerTestBase.h"

class VideoManagerTest : public VideoManagerTestBase
{
    Q_OBJECT

private slots:
    void _testSettingReachesReceiver_data();
    void _testSettingReachesReceiver();
    void _testSettingChangesCoalesce();
    void _testRestartingSettingsRestartStreams_data();
    void _testRestartingSettingsRestartStreams();
    void _testStopVideoCancelsEverything();
    void _testAutoStreamLeavesSettingsAlone();
    void _testThermalOnlyChangeReconfiguresThermal();
    UT_PARAMETERIZED_TEST(_testOtherStreamsPlayInTiles);
    void _testStateChangeWhileTileGoesIsTakenUp();
    void _testStreamInTileBecomesPrimaryAsItIs();
    void _testTilesStopAndStartWithTheVideo();
    UT_PARAMETERIZED_TEST(_testCameraSourcePlaysVideoSourceOrTileCamera);
    void _testSelectedCameraTileTakesMainPicture();
    void _testStateChangesBeforeItsSignals();
    void _testDisableWhenDisarmed_data();
    void _testDisableWhenDisarmed();
    void _testVideoOutputRegistration();
    void _testBackgroundSuspendsDisplay();
    void _testScreenshotsComeFromStreamsOnDisplay();
    void _testScreenshotFailuresReachUser_data();
    void _testScreenshotFailuresReachUser();
    void _testForcedAspectRatioAppliesToConfiguredStream();
    void _testScreenshotFollowsForcedAspectRatio();
    void _testAdvertisedRotationTurnsVideo_data();
    void _testAdvertisedRotationTurnsVideo();
    void _testCameraTransformTurnsItsPicture();
    void _testSourceErrorsReachUser();
    void _testReceiverSetWhileOldOneGoes();
    void _testReconnectingFollowsLostStream();
    void _testLinkRegainedReconnectsStreams();
    void _testNetworkRegainedReconnectsStreams();
    void _testUvcCameraFormatReachesCamera();
    void _testUvcControlsReachCamera();
    void _testSourceFallingShortDegradesVideo_data();
    void _testSourceFallingShortDegradesVideo();
    void _testThermalPlaysOnlyWhileInUse();
    void _testStreamUriAndTransport_data();
    void _testStreamUriAndTransport();
};
