#include "VideoManagerRestartTest.h"

#include <QtTest/QSignalSpy>

#include "VideoManager.h"
#include "VideoReceiver.h"

namespace {

// VideoManager delays a requested restart by 1 s; a longer window shows that no restart follows.
constexpr int kNoRestartWindowMs = 1200;

enum class Trigger
{
    RequestedRestart,
    ReceiverStoppedItself,
    StartFailed,
};

/// Completes start and stop synchronously, as QtMultimediaReceiver does.
class FakeReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    FakeReceiver()
    {
        setName(QStringLiteral("videoContent"));
        setUri(QStringLiteral("udp://0.0.0.0:5600"));
    }

    void start(uint32_t /*timeout*/) override { emit onStartComplete(STATUS_OK); }

    void stop() override
    {
        ++stops;
        emit onStopComplete(started() ? STATUS_OK : STATUS_INVALID_STATE);
    }

    void startDecoding(VideoSinkHandle /*sink*/) override {}

    void stopDecoding() override {}

    void startRecording(const QString& /*videoFile*/, FILE_FORMAT /*format*/) override {}

    void stopRecording() override {}

    int stops = 0;
};

}  // namespace

void VideoManagerRestartTest::_testRestartsOnlyAfterRequestedStop_data()
{
    QTest::addColumn<int>("trigger");
    QTest::addColumn<bool>("expectRestart");

    QTest::newRow("restart requested") << static_cast<int>(Trigger::RequestedRestart) << true;
    QTest::newRow("receiver stopped itself") << static_cast<int>(Trigger::ReceiverStoppedItself) << false;
    QTest::newRow("start failed") << static_cast<int>(Trigger::StartFailed) << false;
}

void VideoManagerRestartTest::_testRestartsOnlyAfterRequestedStop()
{
    QFETCH(int, trigger);
    QFETCH(bool, expectRestart);

    VideoManager videoManager;
    FakeReceiver receiver;
    videoManager._connectReceiverLifecycle(&receiver);
    QSignalSpy startSpy(&receiver, &VideoReceiver::onStartComplete);

    switch (static_cast<Trigger>(trigger)) {
        case Trigger::RequestedRestart:
            receiver.start(0);
            QVERIFY(receiver.started());
            videoManager._restartVideo(&receiver);
            break;
        case Trigger::ReceiverStoppedItself:
            receiver.start(0);
            QVERIFY(receiver.started());
            emit receiver.onStopComplete(VideoReceiver::STATUS_FAIL);
            break;
        case Trigger::StartFailed:
            emit receiver.onStartComplete(VideoReceiver::STATUS_FAIL);
            break;
    }
    QVERIFY(!receiver.started());

    if (expectRestart) {
        QVERIFY_SIGNAL_COUNT_WAIT(startSpy, startSpy.count() + 1, TestTimeout::mediumMs());
        QVERIFY(receiver.started());
    } else {
        QVERIFY_NO_SIGNAL_WAIT(startSpy, kNoRestartWindowMs);
    }
}

void VideoManagerRestartTest::_testStopVideoStopsReceiverWaitingToReconnect()
{
    VideoManager videoManager;
    FakeReceiver receiver;
    videoManager._connectReceiverLifecycle(&receiver);
    videoManager._videoReceivers.append(&receiver);

    receiver.start(0);
    emit receiver.onStopComplete(VideoReceiver::STATUS_FAIL);
    QVERIFY(!receiver.started());

    videoManager.stopVideo();
    QCOMPARE(receiver.stops, 1);
}

UT_REGISTER_TEST(VideoManagerRestartTest, TestLabel::Unit)

#include "VideoManagerRestartTest.moc"
