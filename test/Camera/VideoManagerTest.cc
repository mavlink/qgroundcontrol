#include "VideoManagerTest.h"

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include "Fixtures/RAIIFixtures.h"
#include "SettingsManager.h"
#include "VideoManager.h"
#include "VideoSettings.h"

void VideoManagerTest::_videoUriOverrideDoesNotChangeSettings_test()
{
    VideoManager testManager;
    VideoSettings *const videoSettings = SettingsManager::instance()->videoSettings();
    const QVariant videoSource = videoSettings->videoSource()->rawValue();
    const QVariant rtspUrl = videoSettings->rtspUrl()->rawValue();

    testManager.setVideoUriOverride(true, QStringLiteral("rtsp://192.0.2.1:8556/live"));
    QVERIFY(testManager.hasVideo());
    QVERIFY(testManager.isStreamSource());

    testManager.setVideoUriOverride(true, QString());
    QVERIFY(!testManager.hasVideo());
    QVERIFY(testManager.isStreamSource());

    testManager.setVideoUriOverride(false, QString());
    QCOMPARE(videoSettings->videoSource()->rawValue(), videoSource);
    QCOMPARE(videoSettings->rtspUrl()->rawValue(), rtspUrl);
}

void VideoManagerTest::_videoOutputQmlTypeAvailableInUnitTestMode_test()
{
    static constexpr auto envName = "QGC_TEST_ENABLE_GSTREAMER";
    TestFixtures::EnvVarFixture envBackup(envName);
    qunsetenv(envName);

    VideoManager testManager;
    QQmlEngine engine;
    QQmlComponent component(&engine);

    component.setData(R"QML(
import QtQuick
import QtMultimedia

VideoOutput {
    width: 32
    height: 24
}
)QML", QUrl(QStringLiteral("qrc:/VideoManagerTest.qml")));

    QObject *item = component.create();
    QVERIFY2(component.errors().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(item != nullptr);
    QVERIFY(qobject_cast<QQuickItem *>(item) != nullptr);

    delete item;
}

UT_REGISTER_TEST(VideoManagerTest, TestLabel::Unit)
