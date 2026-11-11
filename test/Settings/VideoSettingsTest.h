#pragma once

#include "UnitTest.h"

class VideoSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _videoFitEnumMatchesMetaData();
    void _videoSourcesFollowCameras();
    void _absentStoredCameraStaysSelected();
    void _overriddenVideoSourcesAreKept();
};
