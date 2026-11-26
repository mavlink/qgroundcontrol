#pragma once

#include <memory>

#include "UnitTest.h"

class FakeVideoReceiver;
class VideoManager;
class VideoSettings;
class VideoStreamController;

namespace VideoTestHelpers {
class FactRestorer;
}

/// What VideoManagerTest and VideoQmlTest start each test with: a VideoManager whose primary and thermal streams play
/// into FakeVideoReceivers, and the video settings it reads, which are restored after the test.
class VideoManagerTestBase : public UnitTest
{
    Q_OBJECT

public:
    // Out of line, where the members' types are complete.
    ~VideoManagerTestBase() override;

protected slots:
    void init() override;
    void cleanup() override;

protected:
    /// Runs the queued stream updates.
    static void _settle();
    void _resetCounters();
    /// The receiver of @p source, which the test's factory made.
    static FakeVideoReceiver* _receiverOf(const VideoStreamController* source);

    VideoSettings* _videoSettings = nullptr;
    std::unique_ptr<VideoTestHelpers::FactRestorer> _facts;
    std::unique_ptr<VideoManager> _videoManager;
    FakeVideoReceiver* _primary = nullptr;
    FakeVideoReceiver* _thermal = nullptr;
};
