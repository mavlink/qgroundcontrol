#pragma once

#include "UnitTest.h"

class VideoSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _videoFitEnumMatchesMetaData();
    void _videoSourcesFollowCameras();
    UT_PARAMETERIZED_TEST(_absentStoredCameraStaysSelected);
    void _overriddenVideoSourcesAreKept();
};
