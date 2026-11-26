#pragma once

#include "UnitTest.h"

/// What the receivers built on FFmpeg open for a stream URI.
class VideoStreamUriTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFFmpegSource_data();
    void _testFFmpegSource();
    void _testFFmpegSourceRefusesUnplayableUri_data();
    void _testFFmpegSourceRefusesUnplayableUri();
};
