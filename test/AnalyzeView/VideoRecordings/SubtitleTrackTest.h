#pragma once

#include "UnitTest.h"

class SubtitleTrackTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testReadsWriterLayout();
    void _testStyleAlignmentWithoutPosition();
    void _testSourceChanges();
    void _testSourceReplacesShownEvents();
    void _testImplausibleValues_data();
    void _testImplausibleValues();
    void _testOversizedFileIsNotRead();
};
