#include "MissionItemVisualLoaderTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QMetaEnum>
#include <QtCore/QPointer>
#include <QtCore/QTemporaryDir>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlPropertyMap>
#include <QtQuick/QQuickItem>

#include "QmlObjectListModel.h"
#include "QmlTesting/QmlFixtures.h"

namespace {

/// Writes a QML file that no engine has loaded yet, so its first load is asynchronous.
QUrl writeQmlFile(const QTemporaryDir& dir, const QString& name, const QByteArray& qml)
{
    QFile file(dir.filePath(name));
    if (!file.open(QFile::WriteOnly) || (file.write(qml) != qml.size())) {
        return {};
    }
    return QUrl::fromLocalFile(file.fileName());
}

}  // namespace

void MissionItemVisualLoaderTest::_itemRemovedWhileItsVisualLoads_data()
{
    // An item that shows the visual of each mission item in its visualItems property
    QTest::addColumn<QByteArray>("qml");
    // The mission item's property that names its visual
    QTest::addColumn<QString>("visualProperty");

    QTest::newRow("map") << QByteArray(R"(
        import QtQuick
        import QGroundControl.PlanView

        Item {
            id: root

            property var visualItems

            Repeater {
                model: root.visualItems

                MissionItemMapVisual { }
            }
        }
    )") << QStringLiteral("mapVisualQML");
    QTest::newRow("GeoMap") << QByteArray(R"(
        import QtQuick
        import QGroundControl.GeoMap

        GeoMapMissionItems {
            id: root

            property var visualItems

            missionController: QtObject {
                readonly property var visualItems: root.visualItems
            }
        }
    )") << QStringLiteral("geoMapVisualQML");
}

void MissionItemVisualLoaderTest::_itemRemovedWhileItsVisualLoads()
{
    QFETCH(QByteArray, qml);
    QFETCH(QString, visualProperty);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QUrl visualUrl = writeQmlFile(dir, QStringLiteral("Visual.qml"), "import QtQuick\nItem { }\n");
    const QUrl laterUrl = writeQmlFile(dir, QStringLiteral("Later.qml"), "import QtQml\nQtObject { }\n");
    QVERIFY(visualUrl.isValid() && laterUrl.isValid());

    const std::unique_ptr<QQmlPropertyMap> missionItem(QQmlPropertyMap::create());
    missionItem->insert(visualProperty, visualUrl.toString());
    QmlObjectListModel visualItems;

    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> root =
        engine.create(qml, {{QStringLiteral("visualItems"), QVariant::fromValue(&visualItems)}});
    QVERIFY2(root, qPrintable(engine.lastError()));

    visualItems.append(missionItem.get());
    const QList<QQuickItem*> loaders = TestFixtures::findItems(
        qobject_cast<QQuickItem*>(root.get()), [](QQuickItem* item) { return item->inherits("QQuickLoader"); });
    QCOMPARE(loaders.size(), 1);
    const QPointer<QQuickItem> loader = loaders.constFirst();
    const QMetaObject* const loaderType = loader->metaObject();
    const QMetaEnum loaderStatus = loaderType->enumerator(loaderType->indexOfEnumerator("Status"));
    QCOMPARE(loader->property("status").toInt(), loaderStatus.keyToValue("Loading"));

    // As when a mission is replaced while the first one's visuals still load: the item's Loader is released with its
    // source loading, and deleted later.
    visualItems.clear();

    // The file loader works in order, so the visual has finished loading once a file requested after it has.
    // processEvents() called from a test function does not delete the released Loader, as QTest's waits would.
    QQmlComponent later(&engine, laterUrl, QQmlComponent::Asynchronous);
    QElapsedTimer timer;
    timer.start();
    while (later.isLoading() && (timer.elapsed() < TestTimeout::mediumMs())) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    QVERIFY(later.isReady());
    // A Loader that still creates its item here warns that its context is invalid, which the log check of every
    // test fails on.
    QVERIFY(loader);

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!loader);
}

UT_REGISTER_TEST(MissionItemVisualLoaderTest, TestLabel::Unit, TestLabel::MissionManager)
