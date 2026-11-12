#include "SerialSettingsTest.h"

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlError>

#include "SerialLink.h"

namespace {

/// The serial settings page of a link configuration, over a stand-in for the link manager's port inventory.
struct SerialSettingsPage
{
    /// Leaves page null, and error telling why, if the page cannot be made.
    void create(SerialConfiguration& config)
    {
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
        inventory.reset(inventoryComponent.create());
        if (!inventory) {
            error = inventoryComponent.errorString();
            return;
        }
        engine.rootContext()->setContextProperty(QStringLiteral("subEditConfig"), &config);
        for (const auto* name : {"_rowSpacing", "_colSpacing", "_secondColumnWidth"}) {
            engine.rootContext()->setContextProperty(QString::fromLatin1(name), 100);
        }
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/SerialSettings.qml")));
        (void) QTest::qWaitFor([&component] { return !component.isLoading(); }, TestTimeout::mediumMs());
        if (component.isReady()) {
            page.reset(component.createWithInitialProperties(
                {{QStringLiteral("linkManager"), QVariant::fromValue(inventory.get())}}));
        }
        if (!page) {
            error = component.errorString();
        }
    }

    QQmlEngine engine;
    std::unique_ptr<QObject> inventory;
    std::unique_ptr<QObject> page;
    QString error;
};

}  // namespace

void SerialSettingsTest::_hotplugKeepsPortIdentity()
{
    SerialConfiguration config(QStringLiteral("Settings test"));
    SerialSettingsPage settings;
    settings.create(config);
    QVERIFY2(settings.page, qPrintable(settings.error));
    QObject* const inventory = settings.inventory.get();
    QObject* const page = settings.page.get();
    auto* combo = page->findChild<QObject*>(QStringLiteral("serialPortCombo"));
    QVERIFY(combo);
    QVERIFY(!combo->property("enabled").toBool());
    const auto update = [&](const QStringList& paths, const QStringList& labels) {
        inventory->setProperty("serialPorts", paths);
        inventory->setProperty("serialPortStrings", labels);
        return QMetaObject::invokeMethod(inventory, "commPortsChanged");
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

void SerialSettingsTest::_losingItsConfigurationIsQuiet()
{
    SerialConfiguration config(QStringLiteral("Settings test"));
    QStringList warnings;
    SerialSettingsPage settings;
    (void) connect(&settings.engine, &QQmlEngine::warnings, this, [&warnings](const QList<QQmlError>& errors) {
        for (const QQmlError& error : errors) {
            warnings.append(error.toString());
        }
    });
    settings.create(config);
    QVERIFY2(settings.page, qPrintable(settings.error));

    // The link editor gives up the configuration it edits before its page is gone.
    settings.engine.rootContext()->setContextProperty(QStringLiteral("subEditConfig"),
                                                      QVariant::fromValue<QObject*>(nullptr));
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
}

UT_REGISTER_TEST(SerialSettingsTest, TestLabel::Unit, TestLabel::Comms)
