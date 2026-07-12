#include "VideoManagerInitTest.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtQuick/QQuickWindow>

#include "Fixtures/RAIIFixtures.h"
#include "SettingsManager.h"
#include "VideoManager.h"
#include "VideoReceiver.h"
#include "VideoSettings.h"

namespace {

class TestVideoReceiver final : public VideoReceiver
{
public:
    void start(uint32_t) override {}

    void stop() override {}

    void startDecoding(VideoSinkHandle) override {}

    void stopDecoding() override {}

    void startRecording(const QString&, FILE_FORMAT) override {}

    void stopRecording() override {}

    void takeScreenshot(const QString&) override {}
};

}  // namespace

void VideoManagerInitTest::init()
{
    UnitTest::init();

    static const QRegularExpression sGStreamerCriticalRe(
        QStringLiteral("cannot register existing type|"
                       "g_type_add_interface_static.*G_TYPE_IS_INSTANTIATABLE|"
                       "g_once_init_leave.*result != 0|"
                       "GStreamer initialization failed"));
    // These backend startup diagnostics are platform/runtime dependent and not
    // tied to a single deterministic call site in this fixture.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtCriticalMsg, sGStreamerCriticalRe);
}

void VideoManagerInitTest::_testQmlReadyBeforeBackendReady()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager._mainWindow = &mainWindow;

    int createReceiversCount = 0;
    videoManager._createVideoReceiversForTest = [&createReceiversCount]() {
        ++createReceiversCount;
    };

    videoManager._initState = VideoManager::InitState::Pending;

    videoManager._initAfterQmlIsReady();
    QCOMPARE(videoManager._initState, VideoManager::InitState::QmlReady);
    QCOMPARE(createReceiversCount, 0);

    videoManager._onBackendInitComplete(true);
    QCOMPARE(videoManager._initState, VideoManager::InitState::Running);
    QCOMPARE(createReceiversCount, 1);

    expectLogMessage("Video.VideoManager", QtWarningMsg, QRegularExpression(QStringLiteral("_onBackendInitComplete: unexpected state")));
    videoManager._onBackendInitComplete(true);
    verifyExpectedLogMessage();
    QCOMPARE(createReceiversCount, 1);
}

void VideoManagerInitTest::_testBackendReadyBeforeQmlReady()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager._mainWindow = &mainWindow;

    int createReceiversCount = 0;
    videoManager._createVideoReceiversForTest = [&createReceiversCount]() {
        ++createReceiversCount;
    };

    videoManager._initState = VideoManager::InitState::Pending;

    videoManager._onBackendInitComplete(true);
    QCOMPARE(videoManager._initState, VideoManager::InitState::BackendReady);
    QCOMPARE(createReceiversCount, 0);

    videoManager._initAfterQmlIsReady();
    QCOMPARE(videoManager._initState, VideoManager::InitState::Running);
    QCOMPARE(createReceiversCount, 1);

    expectLogMessage("Video.VideoManager", QtWarningMsg, QRegularExpression(QStringLiteral("_initAfterQmlIsReady: unexpected state")));
    videoManager._initAfterQmlIsReady();
    verifyExpectedLogMessage();
    QCOMPARE(createReceiversCount, 1);
}

void VideoManagerInitTest::_testBackendInitFailure()
{
    VideoManager videoManager;
    QQuickWindow mainWindow;
    videoManager._mainWindow = &mainWindow;

    int createReceiversCount = 0;
    videoManager._createVideoReceiversForTest = [&createReceiversCount]() {
        ++createReceiversCount;
    };

    videoManager._initState = VideoManager::InitState::Pending;

    expectLogMessage("Video.VideoManager", QtCriticalMsg, QRegularExpression(QStringLiteral("video initialization failed")));
    videoManager._onBackendInitComplete(false);
    verifyExpectedLogMessage();
    QCOMPARE(videoManager._initState, VideoManager::InitState::Failed);
    QCOMPARE(createReceiversCount, 0);

    expectLogMessage("Video.VideoManager", QtWarningMsg, QRegularExpression(QStringLiteral("QML ready but video init failed")));
    videoManager._initAfterQmlIsReady();
    verifyExpectedLogMessage();
    QCOMPARE(videoManager._initState, VideoManager::InitState::Failed);
    QCOMPARE(createReceiversCount, 0);
}

void VideoManagerInitTest::_testNetworkVideoSettingsPropagation()
{
    TestFixtures::SettingsFixture fixture;
    VideoSettings* settings = SettingsManager::instance()->videoSettings();
    QVERIFY(settings);

    fixture.setFactValue(settings->videoSource(), QString::fromLatin1(VideoSettings::videoSourceHTTPMJPEG));
    fixture.setFactValue(settings->httpMjpegUrl(), QStringLiteral("http://192.0.2.1:8080/video_feed"));
    fixture.setFactValue(settings->networkVideoAuthType(), VideoSettings::NetworkVideoAuthNone);
    fixture.setFactValue(settings->networkVideoUsername(), QString());
    fixture.setFactValue(settings->networkVideoOrigin(), QString());
    fixture.setFactValue(settings->networkVideoCaCertificateFile(), QString());
    fixture.setFactValue(settings->streamEnabled(), true);

    const auto clearSecret = qScopeGuard([settings] { settings->clearNetworkVideoSecret(); });
    VideoManager videoManager;
    TestVideoReceiver receiver;
    receiver.setName(QStringLiteral("video"));

    QVERIFY(videoManager._updateSettings(&receiver));
    QCOMPARE(receiver.uri(), QStringLiteral("http://192.0.2.1:8080/video_feed"));
    VideoReceiver::NetworkSourceConfig networkConfig = receiver.networkSourceConfig();
    QVERIFY(networkConfig.authentication == VideoReceiver::NetworkSourceConfig::Authentication::None);
    QVERIFY(networkConfig.secret.isEmpty());

    settings->httpMjpegUrl()->setRawValue(QStringLiteral("https://camera.example/video_feed"));
    settings->networkVideoAuthType()->setRawValue(VideoSettings::NetworkVideoAuthBearer);
    QCOMPARE(settings->setNetworkVideoSecret(QStringLiteral("session-token")), QString());
    QVERIFY(videoManager._updateSettings(&receiver));
    QCOMPARE(receiver.uri(), QStringLiteral("https://camera.example/video_feed"));
    networkConfig.clearSecret();
    networkConfig = receiver.networkSourceConfig();
    QVERIFY(networkConfig.authentication == VideoReceiver::NetworkSourceConfig::Authentication::Bearer);
    QCOMPARE(networkConfig.secret, QByteArrayLiteral("session-token"));

    settings->httpMjpegUrl()->setRawValue(QStringLiteral("http://192.0.2.1:8080/video_feed"));
    expectLogMessage("Video.VideoManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Network video configuration rejected")));
    QVERIFY(videoManager._updateSettings(&receiver));
    verifyExpectedLogMessage();
    QVERIFY(receiver.uri().isEmpty());
    networkConfig.clearSecret();
    networkConfig = receiver.networkSourceConfig();
    QVERIFY(networkConfig.authentication == VideoReceiver::NetworkSourceConfig::Authentication::None);
    QVERIFY(networkConfig.secret.isEmpty());
    networkConfig.clearSecret();
}

#else

void VideoManagerInitTest::init()
{
    UnitTest::init();
    QSKIP("GStreamer not enabled");
}

void VideoManagerInitTest::_testQmlReadyBeforeBackendReady()
{
    QSKIP("GStreamer not enabled");
}

void VideoManagerInitTest::_testBackendReadyBeforeQmlReady()
{
    QSKIP("GStreamer not enabled");
}

void VideoManagerInitTest::_testBackendInitFailure()
{
    QSKIP("GStreamer not enabled");
}

void VideoManagerInitTest::_testNetworkVideoSettingsPropagation()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(VideoManagerInitTest, TestLabel::Unit)
