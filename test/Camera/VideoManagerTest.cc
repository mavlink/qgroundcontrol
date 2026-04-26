#include "VideoManagerTest.h"

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include "VideoManager.h"

void VideoManagerTest::_videoOutputQmlTypeAvailable_test()
{
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
)QML",
                      QUrl(QStringLiteral("qrc:/VideoManagerTest.qml")));

    QObject* item = component.create();
    QVERIFY2(component.errors().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(item != nullptr);
    QVERIFY(qobject_cast<QQuickItem*>(item) != nullptr);

    delete item;
}

UT_REGISTER_TEST(VideoManagerTest, TestLabel::Unit)
