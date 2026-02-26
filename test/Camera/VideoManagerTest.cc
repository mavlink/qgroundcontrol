#include "VideoManagerTest.h"

#include <QtCore/QScopeGuard>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include "VideoManager.h"

void VideoManagerTest::_gstQt6VideoItemQmlTypeAvailableInUnitTestMode_test()
{
    static constexpr auto envName = "QGC_TEST_ENABLE_GSTREAMER";
    const bool wasSet = qEnvironmentVariableIsSet(envName);
    const QByteArray previousValue = qgetenv(envName);
    const auto restoreEnv = qScopeGuard([wasSet, previousValue]() {
        if (wasSet) {
            qputenv(envName, previousValue);
        } else {
            qunsetenv(envName);
        }
    });

    qunsetenv(envName);

    VideoManager testManager;
    QQmlEngine engine;
    QQmlComponent component(&engine);

    component.setData(R"QML(
import QtQuick
import org.freedesktop.gstreamer.Qt6GLVideoItem 1.0

GstGLQt6VideoItem {
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

