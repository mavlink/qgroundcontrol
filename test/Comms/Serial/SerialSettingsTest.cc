#include "SerialSettingsTest.h"

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include "SerialLink.h"

void SerialSettingsTest::_hotplugKeepsPortIdentity()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent inventoryComponent(&engine);
    inventoryComponent.setData(R"(
        import QtQml
        QtObject {
            property var serialPorts: []
            property var serialPortStrings: []
            property var serialBaudRates: ["57600"]
            signal commPortsChanged()
            signal commPortStringsChanged()
        }
    )",
                               QUrl());
    std::unique_ptr<QObject> inventory(inventoryComponent.create());
    QVERIFY2(inventory, qPrintable(inventoryComponent.errorString()));
    SerialConfiguration config(QStringLiteral("Settings test"));
    engine.rootContext()->setContextProperty(QStringLiteral("subEditConfig"), &config);
    for (const auto* name : {"_rowSpacing", "_colSpacing", "_secondColumnWidth"}) {
        engine.rootContext()->setContextProperty(QString::fromLatin1(name), 100);
    }
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/SerialSettings.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> page(
        component.createWithInitialProperties({{QStringLiteral("linkManager"), QVariant::fromValue(inventory.get())}}));
    QVERIFY2(page, qPrintable(component.errorString()));
    auto* combo = page->findChild<QObject*>(QStringLiteral("serialPortCombo"));
    QVERIFY(combo);
    QVERIFY(!combo->property("enabled").toBool());
    const auto update = [&](const QStringList& paths, const QStringList& labels) {
        inventory->setProperty("serialPorts", paths);
        inventory->setProperty("serialPortStrings", labels);
        return QMetaObject::invokeMethod(inventory.get(), "commPortsChanged");
    };
    QVERIFY(update({"/test/A", "/test/B"}, {"A", "B"}));
    QCOMPARE(config.portName(), QStringLiteral("/test/A"));
    QCOMPARE(combo->property("currentText").toString(), QStringLiteral("A"));
    QVERIFY(update({"/test/B", "/test/A"}, {"B", "A"}));
    QCOMPARE(combo->property("currentIndex").toInt(), 1);
    QCOMPARE(combo->property("currentText").toString(), QStringLiteral("A"));
    QVERIFY(update({"/test/B"}, {"B"}));
    QCOMPARE(config.portName(), QStringLiteral("/test/A"));
    QCOMPARE(combo->property("currentText").toString(), QStringLiteral("/test/A"));
    QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, 0)));
    QCOMPARE(config.portName(), QStringLiteral("/test/B"));
    QVERIFY(update({}, {}));
    QCOMPARE(config.portName(), QStringLiteral("/test/B"));
    QVERIFY(!combo->property("enabled").toBool());
}

UT_REGISTER_TEST(SerialSettingsTest, TestLabel::Unit, TestLabel::Comms)
