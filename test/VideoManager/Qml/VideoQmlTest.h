#pragma once

#include "VideoManagerTestBase.h"

/// The video's QML: VideoView with its tiles and overlays, the Fly View's video around it, and the Video indicator
/// with the settings and statistics in its drawer, each showing a VideoManager whose sources play into
/// FakeVideoReceivers.
class VideoQmlTest : public VideoManagerTestBase
{
    Q_OBJECT

private slots:
    void _testDecodingSettingsGroupRestartsStreams();
    void _testStreamSettingsGroupFollowsLowLatency();
    void _testPlaceholderTellsWhyThereIsNoVideo_data();
    void _testPlaceholderTellsWhyThereIsNoVideo();
    void _testQmlVideoOutputRegistersItself();
    void _testStreamStatsReachQml();
    void _testStatsGroupShowsCamera();
    void _testIndicatorReportsMeasuredSource();
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
    void _testVideoViewReportsClicksAndDrags();
    void _testThermalDecodesOnlyWhileShown();
    void _testThermalOverlayHasThePicturesShape();
};
