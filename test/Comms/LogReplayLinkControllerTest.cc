#include "LogReplayLinkControllerTest.h"

#include "LogReplayLink.h"
#include "LogReplayLinkController.h"

// Signals are emitted on the links directly rather than playing back a real log file:
// the controller only reacts to link signals, so this keeps the tests deterministic.

void LogReplayLinkControllerTest::_testStateTracksLinkSignals()
{
    SharedLinkConfigurationPtr config = std::make_shared<LogReplayConfiguration>(QStringLiteral("LogReplayLinkControllerTest"));
    LogReplayLink link(config);
    LogReplayLinkController controller;

    controller.setLink(&link);
    QCOMPARE(controller.link(), &link);

    // The very first playhead update at t=0 must not be dropped by the dedup guard
    // (0 previously doubled as both the initial sentinel and a legitimate time value).
    emit link.currentLogTimeSecs(0);
    QCOMPARE(controller.property("playheadTime").toString(), QStringLiteral("00m:00s"));

    emit link.logFileStats(3661);
    emit link.playbackStarted();
    emit link.playbackPercentCompleteChanged(0.5);
    emit link.currentLogTimeSecs(90);

    QCOMPARE(controller.property("totalTime").toString(), QStringLiteral("01h:01m:01s"));
    QVERIFY(controller.isPlaying());
    QCOMPARE(controller.percentComplete(), 0.5);
    QCOMPARE(controller.property("playheadTime").toString(), QStringLiteral("01m:30s"));

    controller.setLink(nullptr);
}

void LogReplayLinkControllerTest::_testResetWhenLinkDisconnects()
{
    SharedLinkConfigurationPtr config = std::make_shared<LogReplayConfiguration>(QStringLiteral("LogReplayLinkControllerTest"));
    LogReplayLink link(config);
    LogReplayLinkController controller;

    controller.setLink(&link);

    emit link.logFileStats(3661);
    emit link.playbackStarted();
    emit link.playbackPercentCompleteChanged(0.5);
    emit link.currentLogTimeSecs(90);

    // Link disconnect must reset all controller state (issue #13251 bug 1)
    emit link.disconnected();

    QCOMPARE(controller.link(), nullptr);
    QVERIFY(!controller.isPlaying());
    QCOMPARE(controller.percentComplete(), 0.0);
    QVERIFY(controller.property("totalTime").toString().isEmpty());
    QVERIFY(controller.property("playheadTime").toString().isEmpty());

    // Re-attach: a new session starting at t=0 must update the playhead label even
    // though the detach reset cleared it (0 must not be treated as "no change").
    controller.setLink(&link);
    emit link.currentLogTimeSecs(0);
    QCOMPARE(controller.property("playheadTime").toString(), QStringLiteral("00m:00s"));

    // Replaying the same log time as before the detach must also update the label
    // (stale _playheadSecs guard).
    emit link.currentLogTimeSecs(90);
    QCOMPARE(controller.property("playheadTime").toString(), QStringLiteral("01m:30s"));

    controller.setLink(nullptr);
}

void LogReplayLinkControllerTest::_testDetachedLinkSignalsIgnored()
{
    SharedLinkConfigurationPtr config = std::make_shared<LogReplayConfiguration>(QStringLiteral("LogReplayLinkControllerTest"));
    LogReplayLink link(config);
    LogReplayLinkController controller;

    controller.setLink(&link);
    controller.setLink(nullptr);

    // A detached link's signals must no longer affect the controller (regression:
    // setLink previously only disconnected controller -> link, leaving the
    // link -> controller connections live).
    emit link.logFileStats(3661);
    emit link.playbackStarted();
    emit link.playbackPercentCompleteChanged(0.5);
    emit link.currentLogTimeSecs(90);

    QCOMPARE(controller.link(), nullptr);
    QVERIFY(!controller.isPlaying());
    QCOMPARE(controller.percentComplete(), 0.0);
    QVERIFY(controller.property("totalTime").toString().isEmpty());
    QVERIFY(controller.property("playheadTime").toString().isEmpty());
}

UT_REGISTER_TEST(LogReplayLinkControllerTest, TestLabel::Unit, TestLabel::Comms)
