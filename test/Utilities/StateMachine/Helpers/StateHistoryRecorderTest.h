#pragma once

#include "UnitTest.h"

class StateHistoryRecorderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testManualEntriesRequireEnabledAndTrim();
    void _testLastEntriesAndEntriesForState();
    void _testStateEntryExitRecording();
};
