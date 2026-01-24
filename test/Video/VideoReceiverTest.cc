#include "VideoReceiverTest.h"
#include "VideoReceiver.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

/// Mock implementation of VideoReceiver for testing the base class functionality.
class MockVideoReceiver : public VideoReceiver
{
    Q_OBJECT
public:
    explicit MockVideoReceiver(QObject *parent = nullptr) : VideoReceiver(parent) {}

    // Implement pure virtual methods with no-ops
    void start(uint32_t timeout) override { Q_UNUSED(timeout); emit onStartComplete(STATUS_OK); }
    void stop() override { emit onStopComplete(STATUS_OK); }
    void startDecoding(void *sink) override { Q_UNUSED(sink); emit onStartDecodingComplete(STATUS_OK); }
    void stopDecoding() override { emit onStopDecodingComplete(STATUS_OK); }
    void startRecording(const QString &videoFile, FILE_FORMAT format) override {
        Q_UNUSED(videoFile); Q_UNUSED(format);
        emit onStartRecordingComplete(STATUS_OK);
    }
    void stopRecording() override { emit onStopRecordingComplete(STATUS_OK); }
    void takeScreenshot(const QString &imageFile) override {
        Q_UNUSED(imageFile);
        emit onTakeScreenshotComplete(STATUS_OK);
    }
};

void VideoReceiverTest::_testFileFormatValidation()
{
    // Test valid file formats
    QVERIFY(VideoReceiver::isValidFileFormat(VideoReceiver::FILE_FORMAT_MKV));
    QVERIFY(VideoReceiver::isValidFileFormat(VideoReceiver::FILE_FORMAT_MOV));
    QVERIFY(VideoReceiver::isValidFileFormat(VideoReceiver::FILE_FORMAT_MP4));

    // Test boundary values
    QVERIFY(VideoReceiver::isValidFileFormat(VideoReceiver::FILE_FORMAT_MIN));
    QVERIFY(VideoReceiver::isValidFileFormat(VideoReceiver::FILE_FORMAT_MAX));

    // Test invalid values (cast to bypass type safety for testing)
    QVERIFY(!VideoReceiver::isValidFileFormat(static_cast<VideoReceiver::FILE_FORMAT>(-1)));
    QVERIFY(!VideoReceiver::isValidFileFormat(static_cast<VideoReceiver::FILE_FORMAT>(VideoReceiver::FILE_FORMAT_MAX + 1)));
    QVERIFY(!VideoReceiver::isValidFileFormat(static_cast<VideoReceiver::FILE_FORMAT>(100)));
}

void VideoReceiverTest::_testStatusValidation()
{
    // Test valid statuses
    QVERIFY(VideoReceiver::isValidStatus(VideoReceiver::STATUS_OK));
    QVERIFY(VideoReceiver::isValidStatus(VideoReceiver::STATUS_FAIL));
    QVERIFY(VideoReceiver::isValidStatus(VideoReceiver::STATUS_INVALID_STATE));
    QVERIFY(VideoReceiver::isValidStatus(VideoReceiver::STATUS_INVALID_URL));
    QVERIFY(VideoReceiver::isValidStatus(VideoReceiver::STATUS_NOT_IMPLEMENTED));

    // Test boundary values
    QVERIFY(VideoReceiver::isValidStatus(VideoReceiver::STATUS_MIN));
    QVERIFY(VideoReceiver::isValidStatus(VideoReceiver::STATUS_MAX));

    // Test invalid values
    QVERIFY(!VideoReceiver::isValidStatus(static_cast<VideoReceiver::STATUS>(-1)));
    QVERIFY(!VideoReceiver::isValidStatus(static_cast<VideoReceiver::STATUS>(VideoReceiver::STATUS_MAX + 1)));
    QVERIFY(!VideoReceiver::isValidStatus(static_cast<VideoReceiver::STATUS>(100)));
}

void VideoReceiverTest::_testIsThermal()
{
    MockVideoReceiver receiver;

    // Default name - not thermal
    QVERIFY(!receiver.isThermal());

    // Set non-thermal name
    receiver.setName("videoContent");
    QVERIFY(!receiver.isThermal());

    // Set thermal name
    receiver.setName("thermalVideo");
    QVERIFY(receiver.isThermal());

    // Set back to non-thermal
    receiver.setName("someOtherVideo");
    QVERIFY(!receiver.isThermal());
}

void VideoReceiverTest::_testPropertySettersEmitSignals()
{
    MockVideoReceiver receiver;

    // Test name change signal
    {
        QSignalSpy spy(&receiver, &VideoReceiver::nameChanged);
        receiver.setName("testName");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), QString("testName"));

        // Setting same value should not emit
        receiver.setName("testName");
        QCOMPARE(spy.count(), 1);
    }

    // Test started change signal
    {
        QSignalSpy spy(&receiver, &VideoReceiver::startedChanged);
        receiver.setStarted(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toBool(), true);

        // Setting same value should not emit
        receiver.setStarted(true);
        QCOMPARE(spy.count(), 1);

        // Setting different value should emit
        receiver.setStarted(false);
        QCOMPARE(spy.count(), 2);
    }

    // Test lowLatency change signal
    {
        QSignalSpy spy(&receiver, &VideoReceiver::lowLatencyChanged);
        receiver.setLowLatency(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toBool(), true);
    }
}

void VideoReceiverTest::_testUriChanges()
{
    MockVideoReceiver receiver;

    QSignalSpy spy(&receiver, &VideoReceiver::uriChanged);

    // Initial URI should be empty
    QVERIFY(receiver.uri().isEmpty());

    // Set RTSP URI
    receiver.setUri("rtsp://192.168.1.1:554/live");
    QCOMPARE(receiver.uri(), QString("rtsp://192.168.1.1:554/live"));
    QCOMPARE(spy.count(), 1);

    // Set UDP URI
    receiver.setUri("udp://0.0.0.0:5600");
    QCOMPARE(receiver.uri(), QString("udp://0.0.0.0:5600"));
    QCOMPARE(spy.count(), 2);

    // Set empty URI (disable)
    receiver.setUri(QString());
    QVERIFY(receiver.uri().isEmpty());
    QCOMPARE(spy.count(), 3);

    // Setting same empty value should not emit
    receiver.setUri(QString());
    QCOMPARE(spy.count(), 3);
}

void VideoReceiverTest::_testLowLatencyMode()
{
    MockVideoReceiver receiver;

    // Default should be false
    QVERIFY(!receiver.lowLatency());

    // Enable low latency
    receiver.setLowLatency(true);
    QVERIFY(receiver.lowLatency());

    // Disable low latency
    receiver.setLowLatency(false);
    QVERIFY(!receiver.lowLatency());
}

#include "VideoReceiverTest.moc"
