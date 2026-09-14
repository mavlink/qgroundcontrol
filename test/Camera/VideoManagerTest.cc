#include "VideoManagerTest.h"

#include <QtCore/QScopedPointer>
#include <QtCore/QTemporaryDir>
#include <QtCore/QVariant>
#include <QtGui/QImageReader>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include "Fixtures/RAIIFixtures.h"
#include "VideoManager.h"

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

void VideoManagerTest::_saveImageFromQml_test()
{
#ifdef Q_OS_ANDROID
    QSKIP("Gallery publication requires a device test; do not create real Gallery items in unit tests.");
#else
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString fileName = tempDir.filePath(QStringLiteral("snapshot.jpg"));
    QImage frame(32, 24, QImage::Format_RGB32);
    frame.fill(Qt::red);

    VideoManager testManager;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("testVideoManager"), &testManager);
    engine.rootContext()->setContextProperty(QStringLiteral("testFileName"), fileName);
    engine.rootContext()->setContextProperty(QStringLiteral("testFrame"), QVariant::fromValue(frame));
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQml

QtObject {
    property bool saved: testVideoManager.saveImage(testFileName, testFrame)
}
)QML",
                      QUrl(QStringLiteral("qrc:/VideoManagerSaveImageTest.qml")));

    const QScopedPointer<QObject> result(component.create());
    QVERIFY2(component.errors().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(result);
    QVERIFY(result->property("saved").toBool());

    QImageReader reader(fileName);
    QCOMPARE(reader.format(), QByteArray("jpeg"));
    const QImage savedFrame = reader.read();
    QVERIFY2(!savedFrame.isNull(), qPrintable(reader.errorString()));
    QCOMPARE(savedFrame.size(), frame.size());
#endif
}

void VideoManagerTest::_saveImageRejectsEmptyInput_test()
{
    VideoManager testManager;
    QVERIFY(!testManager.saveImage(QStringLiteral("unused.jpg"), QImage()));
    QVERIFY(!testManager.saveImage(QString(), QImage(1, 1, QImage::Format_RGB32)));
}

UT_REGISTER_TEST(VideoManagerTest, TestLabel::Unit)
