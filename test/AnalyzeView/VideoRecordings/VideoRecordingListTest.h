#pragma once

#include "UnitTest.h"

class VideoRecordingListTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testListsRecordingsNewestFirst();
    void _testRemove();
};
