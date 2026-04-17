#pragma once

#include "StateMachineTest.h"

class StateHistoryRecorderTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testManualEntriesRequireEnabledAndTrim();
    void _testLastEntriesAndEntriesForState();
    void _testStateEntryExitRecording();
};
