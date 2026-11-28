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
                property var serialPortLinks: []
                property var serialBaudRates: ["57600"]
                property var portsWithoutFlowControl: []
                property var portLinks: ({})
                signal commPortsChanged()
                signal commPortStringsChanged()
                function serialPortHasFlowControl(name) { return portsWithoutFlowControl.indexOf(name) < 0 }
                function serialPortLinkTo(name) { return portLinks[name] || "" }
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

void SerialSettingsTest::_portWithoutFlowControlGetsNone()
{
    SerialConfiguration config(QStringLiteral("Settings test"));
    config.setFlowControl(QSerialPort::HardwareControl);
    SerialSettingsPage settings;
    settings.create(config);
    QVERIFY2(settings.page, qPrintable(settings.error));
    QObject* const inventory = settings.inventory.get();
    auto* box = settings.page->findChild<QObject*>(QStringLiteral("flowControlCheckBox"));
    QVERIFY(box);

    // A radio on an adapter with RTS and CTS, and a flight controller whose USB port has no such lines.
    const QString radio = QStringLiteral("/test/radio");
    const QString board = QStringLiteral("/test/board");
    inventory->setProperty("portsWithoutFlowControl", QStringList{board});
    inventory->setProperty("serialPorts", QStringList{radio, board});
    inventory->setProperty("serialPortStrings", QStringList{QStringLiteral("Radio"), QStringLiteral("Board")});
    QVERIFY(QMetaObject::invokeMethod(inventory, "commPortsChanged"));
    QCOMPARE(config.portName(), radio);
    QVERIFY(box->property("enabled").toBool());
    QVERIFY(box->property("checked").toBool());
    QCOMPARE(config.flowControl(), QSerialPort::HardwareControl);

    // The port would refuse to open with a flow control that it does not have.
    config.setPortName(board);
    QVERIFY(!box->property("enabled").toBool());
    QVERIFY(!box->property("checked").toBool());
    QCOMPARE(config.flowControl(), QSerialPort::NoFlowControl);

    // The user may turn it on again for a port that has it.
    config.setPortName(radio);
    QVERIFY(box->property("enabled").toBool());
    QCOMPARE(config.flowControl(), QSerialPort::NoFlowControl);
    QVERIFY(box->setProperty("checked", true));
    QCOMPARE(config.flowControl(), QSerialPort::HardwareControl);

    // What a port can do is known once its device is attached, which may be while the page is open.
    inventory->setProperty("portsWithoutFlowControl", QStringList{radio, board});
    QVERIFY(QMetaObject::invokeMethod(inventory, "commPortsChanged"));
    QVERIFY(!box->property("enabled").toBool());
    QCOMPARE(config.flowControl(), QSerialPort::NoFlowControl);
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

void SerialSettingsTest::_portCanBeTypedOrChosenAsALink()
{
    SerialConfiguration config(QStringLiteral("Settings test"));
    SerialSettingsPage settings;
    settings.create(config);
    QVERIFY2(settings.page, qPrintable(settings.error));
    QObject* const inventory = settings.inventory.get();
    QObject* const page = settings.page.get();
    auto* combo = page->findChild<QObject*>(QStringLiteral("serialPortCombo"));
    QVERIFY(combo);
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    QVERIFY(combo->property("editable").toBool());

    const QString port = QStringLiteral("/dev/ttyACM4");
    const QString link = QStringLiteral("/dev/serial/by-id/usb-ArduPilot_Pixhawk6X_0123-if00");
    inventory->setProperty("serialPorts", QStringList{port});
    inventory->setProperty("serialPortStrings", QStringList{QStringLiteral("ttyACM4")});
    inventory->setProperty("serialPortLinks", QStringList{link});
    inventory->setProperty("portLinks", QVariantMap{{port, link}});
    QVERIFY(QMetaObject::invokeMethod(inventory, "commPortsChanged"));

    // Links to ports are offered first, as the paths that a link keeps, and a new link takes the first port by one
    QCOMPARE(combo->property("count").toInt(), 2);
    QCOMPARE(config.portName(), link);
    QCOMPARE(combo->property("currentIndex").toInt(), 0);
    QCOMPARE(combo->property("editText").toString(), link);

    // The port itself can still be chosen
    QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, 1)));
    QCOMPARE(config.portName(), port);
    QCOMPARE(combo->property("editText").toString(), QStringLiteral("ttyACM4"));

    // A typed path is kept, and then listed
    const QString alias = QStringLiteral("/dev/pixhawk");
    QVERIFY(combo->setProperty("editText", QStringLiteral(" %1 ").arg(alias)));
    QVERIFY(QMetaObject::invokeMethod(combo, "accepted"));
    QCOMPARE(config.portName(), alias);
    QCOMPARE(combo->property("currentText").toString(), alias);
    QCOMPARE(combo->property("count").toInt(), 3);

    // The name of a listed port, typed, is that port
    QVERIFY(combo->setProperty("editText", QStringLiteral("ttyACM4")));
    QVERIFY(QMetaObject::invokeMethod(combo, "accepted"));
    QCOMPARE(config.portName(), port);

    // Ok takes a path still being typed
    const QString byPath = QStringLiteral("/dev/serial/by-path/pci-0000:00:14.0-usb-0:2:1.0");
    QVERIFY(combo->setProperty("editText", byPath));
    QVERIFY(QMetaObject::invokeMethod(page, "saveSettings"));
    QCOMPARE(config.portName(), byPath);

    // Empty text changes nothing
    QVERIFY(combo->setProperty("editText", QString()));
    QVERIFY(QMetaObject::invokeMethod(page, "saveSettings"));
    QCOMPARE(config.portName(), byPath);
#else
    // Ports are chosen from the list only, which has no links
    QVERIFY(!combo->property("editable").toBool());
    inventory->setProperty("serialPorts", QStringList{QStringLiteral("COM3")});
    inventory->setProperty("serialPortStrings", QStringList{QStringLiteral("COM3")});
    inventory->setProperty("serialPortLinks", QStringList{QStringLiteral("/dev/serial/by-id/x")});
    QVERIFY(QMetaObject::invokeMethod(inventory, "commPortsChanged"));
    QCOMPARE(combo->property("count").toInt(), 1);
#endif
}

UT_REGISTER_TEST(SerialSettingsTest, TestLabel::Unit, TestLabel::Comms)
