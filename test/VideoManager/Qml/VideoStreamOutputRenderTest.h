#pragma once

#include "UnitTest.h"

/// What Qt Quick draws of a VideoStreamOutput's picture, read back from the scene. VideoQmlTest checks where the
/// output puts its picture, which doesn't tell whether Qt draws it there. Skips where Qt Quick renders in software,
/// which draws no video: only the Linux test environment asks for OpenGL (test/VideoManager/CMakeLists.txt).
class VideoStreamOutputRenderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPictureIsDrawnWhereFitted_data();
    void _testPictureIsDrawnWhereFitted();
};
