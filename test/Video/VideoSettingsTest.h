#pragma once

#include "UnitTest.h"

/// Unit tests for VideoSettings.
/// Tests video source constants and stream configuration.
class VideoSettingsTest : public UnitTest
{
    Q_OBJECT

public:
    VideoSettingsTest() = default;

private slots:
    void _testVideoSourceConstants();
    void _testStreamSourceIdentification();
};
