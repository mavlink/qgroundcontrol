#pragma once

#include "UnitTest.h"

class SubtitleTrackTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testReadsWriterLayout();
    void _testStyleAlignmentWithoutPosition();
    void _testSourceChanges();
};
