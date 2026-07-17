#pragma once

#include "UnitTest.h"

/// Tests for LogReplayLinkController state handling (issue #13251 bug 1: log replay
/// status bar not resetting when the replay link goes away).
class LogReplayLinkControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateTracksLinkSignals();
    void _testResetWhenLinkDisconnects();
    void _testDetachedLinkSignalsIgnored();
};
