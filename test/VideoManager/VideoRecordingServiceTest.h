#pragma once

#include <memory>

#include "UnitTest.h"

class FakeVideoReceiver;
class QTemporaryDir;
class VideoRecordingService;
class VideoStreamController;

namespace VideoTestHelpers {
class FactRestorer;
}

class VideoRecordingServiceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testUniqueBaseName();
    void _testRecordsActiveStreams();
    void _testNothingToRecord();
    void _testRecordingFailureReachesUser();
    void _testPruneStorage();

private:
    void _activate(VideoStreamController* stream);

    std::unique_ptr<VideoTestHelpers::FactRestorer> _facts;
    std::unique_ptr<QTemporaryDir> _saveDir;
    QString _videoDir;
    std::unique_ptr<QObject> _owner;
    VideoStreamController* _primaryStream = nullptr;
    VideoStreamController* _thermalStream = nullptr;
    VideoRecordingService* _service = nullptr;
    FakeVideoReceiver* _primary = nullptr;
    FakeVideoReceiver* _thermal = nullptr;
};
