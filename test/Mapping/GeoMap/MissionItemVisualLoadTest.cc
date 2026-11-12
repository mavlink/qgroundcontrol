#include "MissionItemVisualLoadTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QMetaProperty>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include "QmlObjectListModel.h"

namespace {

constexpr const char* kFlightMapVisualsQml = R"(
import QtQuick
import QGroundControl.PlanView

Item {
    id: root

    property var visualItems

    Repeater {
        model: root.visualItems

        MissionItemMapVisual {
            map: root
        }
    }
}
)";

constexpr const char* kGeoMapVisualsQml = R"(
import QtQuick
import QGroundControl.GeoMap

GeoMapMissionItems {
    id: root

    property var visualItems

    missionController: QtObject {
        readonly property var visualItems: root.visualItems
    }
}
)";

// Has the properties that the maps set on a visual
constexpr const char* kVisualQml = R"(
import QtQuick

Item {
    property var map
    property var vehicle
    property bool interactive
    property var item
    property var scene
    property var surfaceModel
}
)";

constexpr int kItemCount = 3;

/// The loaders under \a item that are loading their source
int loadingLoaderCount(const QQuickItem* item)
{
    int count = 0;
    if (item->inherits("QQuickLoader")) {
        const QMetaProperty status = item->metaObject()->property(item->metaObject()->indexOfProperty("status"));
        if (status.read(item).toInt() == status.enumerator().keyToValue("Loading")) {
            count++;
        }
    }
    for (const QQuickItem* const child : item->childItems()) {
        count += loadingLoaderCount(child);
    }
    return count;
}

}  // namespace

void MissionItemVisualLoadTest::_itemRemovedWhileVisualLoads_data()
{
    QTest::addColumn<QByteArray>("visualsQml");

    QTest::newRow("FlightMap") << QByteArray(kFlightMapVisualsQml);
    QTest::newRow("GeoMap") << QByteArray(kGeoMapVisualsQml);
}

// A mission that is replaced while the visuals of its items load, as one is when a vehicle sends its own, leaves
// no visual behind and no warning. Qt deletes the views of removed items later and warns when a visual of one
// finishes loading before that.
void MissionItemVisualLoadTest::_itemRemovedWhileVisualLoads()
{
    QFETCH(QByteArray, visualsQml);

    // A file of its own, so that the engine has to load the visual: it loads a known one at once
    QTemporaryDir visualDir;
    QVERIFY(visualDir.isValid());
    const QString visualPath = visualDir.filePath(QStringLiteral("MissionItemVisualLoadTestVisual.qml"));
    QFile visualFile(visualPath);
    if (!visualFile.open(QIODevice::WriteOnly)) {
        QFAIL(qPrintable(QStringLiteral("Cannot write %1: %2").arg(visualPath, visualFile.errorString())));
    }
    QVERIFY(visualFile.write(kVisualQml) > 0);
    visualFile.close();
    const QUrl visualUrl = QUrl::fromLocalFile(visualPath);

    QmlObjectListModel visualItems;
    for (int i = 0; i < kItemCount; i++) {
        visualItems.append(new MissionItemVisualLoadTestItem(visualUrl.toString(), &visualItems));
    }

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(visualsQml, QUrl());
    const std::unique_ptr<QObject> root(
        component.createWithInitialProperties({{QStringLiteral("visualItems"), QVariant::fromValue(&visualItems)}}));
    QVERIFY2(root, qPrintable(component.errorString()));
    const QQuickItem* const rootItem = qobject_cast<QQuickItem*>(root.get());
    QVERIFY(rootItem);
    QCOMPARE(loadingLoaderCount(rootItem), kItemCount);

    // Done loading once the visuals are, whose loaders asked for the file first
    const QQmlComponent loadProbe(&engine, visualUrl, QQmlComponent::Asynchronous);

    visualItems.clear();

    // Not a QTRY wait: it delivers deferred deletions too, and the removed items' views must outlive the loading
    QElapsedTimer loadTimer;
    loadTimer.start();
    while (loadProbe.isLoading() && (loadTimer.elapsed() < TestTimeout::mediumMs())) {
        QCoreApplication::processEvents();
    }
    QCOMPARE(loadProbe.status(), QQmlComponent::Ready);
    QCoreApplication::processEvents();

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCOMPARE(loadingLoaderCount(rootItem), 0);
}

UT_REGISTER_TEST(MissionItemVisualLoadTest, TestLabel::Unit)
