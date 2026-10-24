#include "VideoManagerRestartTest.h"

// VideoReceiver.h converts QPointer<QQuickItem> inline, which needs the complete type.
#include <QtQuick/QQuickItem>

#include "VideoManager.h"
#include "VideoReceiver.h"

namespace {

/// Counts the restarts VideoManager asks for; the receiver reconnects on its own.
class FakeReceiver : public VideoReceiver
{
public:
    FakeReceiver()
    {
        setName(QStringLiteral("videoContent"));
        setConfig(Config{.uri = QStringLiteral("udp://0.0.0.0:5600")});
    }

    int restarts = 0;

protected:
    void onConfigChanged(const Config& /*previous*/) override {}

    void onActiveChanged() override {}

    void onRestartRequested() override { ++restarts; }

    void onVideoOutputChanged() override {}

    void onStartRecording(const QString& /*file*/, FileFormat /*format*/) override {}

    void onStopRecording() override {}
};

}  // namespace

void VideoManagerRestartTest::_testRestartStartsOrRestarts()
{
    VideoManager videoManager;
    FakeReceiver receiver;

    videoManager._restartVideo(&receiver);
    QVERIFY(receiver.active());
    QCOMPARE(receiver.restarts, 0);

    videoManager._restartVideo(&receiver);
    QCOMPARE(receiver.restarts, 1);
}

void VideoManagerRestartTest::_testStopVideoDeactivatesReceivers()
{
    VideoManager videoManager;
    FakeReceiver receiver;
    videoManager._videoReceivers.append(&receiver);
    receiver.setActive(true);

    videoManager.stopVideo();
    QVERIFY(!receiver.active());
    videoManager._videoReceivers.clear();
}

UT_REGISTER_TEST(VideoManagerRestartTest, TestLabel::Unit)
