#include "GPSReceiverSettingsTest.h"

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QThread>
#include <QtPositioning/QGeoCoordinate>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtQml/QQmlPropertyMap>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include "AutoConnectSettings.h"
#include "ColoredSvgImageProvider.h"
#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSRtk.h"
#include "GPSTransport.h"
#include "MAVLinkLib.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "development/mavlink_msg_gnss_integrity.h"

/// The receiver state the GPS toolbar indicator reads, without a receiver session.
class IndicatorReceiver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GPSRTKFactGroup* facts READ facts CONSTANT)
    Q_PROPERTY(int activeRole MEMBER activeRole CONSTANT)

public:
    IndicatorReceiver() { factGroup.setLiveUpdates(true); }

    GPSRTKFactGroup* facts() { return &factGroup; }

    GPSRTKFactGroup factGroup;
    int activeRole = GPSRtk::PositionOnly;
};

/// Records the QML command boundary without opening a device or configuring hardware.
class ReceiverSettingsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasReceiver READ hasReceiver NOTIFY receiverChanged)
    Q_PROPERTY(bool reconnecting READ reconnecting NOTIFY receiverChanged)
    Q_PROPERTY(bool serialSupported READ serialSupported CONSTANT)

public:
    explicit ReceiverSettingsController(RTKSettings* settings)
        : _settings(settings)
    {}

    bool hasReceiver() const { return _hasReceiver; }

    bool reconnecting() const { return _reconnecting; }

    void setReconnecting(bool reconnecting)
    {
        _reconnecting = reconnecting;
        emit receiverChanged();
    }

    bool serialSupported() const { return true; }

    void setConnected(bool connected)
    {
        _hasReceiver = connected;
        emit receiverChanged();
    }

    Q_INVOKABLE GPSReceiverPresentation capabilitiesFor(int role, int manufacturer) const
    {
        return gpsReceiverPresentation(role == GPSRtk::ConfiguredBase ? manufacturer
                                                                      : GPSRtk::manufacturerForType(GPSType::passive));
    }

    Q_INVOKABLE bool connectConfiguredGPS(bool allowPersistentChanges = false)
    {
        permissions.append(allowPersistentChanges);
        selectedType = _settings->receiverRole()->rawValue().toInt() == GPSRtk::ConfiguredBase
                           ? GPSRtk::typeForManufacturer(_settings->baseReceiverManufacturers()->rawValue().toInt())
                           : std::optional(GPSType::passive);
        selectedMode = _settings->useFixedBasePosition()->rawValue().toInt();
        fixedPosition = {};
        if (selectedMode == static_cast<int>(BaseModeDefinition::Mode::BaseFixed)) {
            fixedPosition.mode = GPSBaseStationConfig::Fixed{
                .position = {.latitudeDegrees = _settings->fixedBasePositionLatitude()->rawValue().toDouble(),
                             .longitudeDegrees = _settings->fixedBasePositionLongitude()->rawValue().toDouble(),
                             .altitudeMeters = _settings->fixedBasePositionAltitude()->rawValue().toFloat()},
                .accuracyMeters = _settings->fixedBasePositionAccuracy()->rawValue().toFloat()};
        }
        if (connectSucceeds) {
            setConnected(true);
        }
        return connectSucceeds;
    }

    Q_INVOKABLE void disconnectConfiguredGPS()
    {
        _reconnecting = false;
        setConnected(false);
    }

    bool connectSucceeds = true;
    QList<bool> permissions;
    std::optional<GPSType> selectedType;
    int selectedMode = -1;
    GPSBaseStationConfig fixedPosition;

signals:
    void receiverChanged();

private:
    RTKSettings* _settings;
    bool _hasReceiver = false;
    bool _reconnecting = false;
};

namespace {
struct SettingsFixture
{
    TestFixtures::SettingsFixture saved;
    RTKSettings* settings = SettingsManager::instance()->rtkSettings();
    Fact* autoConnect = SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS();

    explicit SettingsFixture(int manufacturer)
    {
        const bool passive = manufacturer == GPSRtk::manufacturerForType(GPSType::passive);
        saved.setFactValue(autoConnect, false);
        saved.setFactValue(settings->receiverRole(), passive ? GPSRtk::Passive : GPSRtk::ConfiguredBase);
        saved.setFactValue(settings->baseReceiverManufacturers(),
                           passive ? settings->baseReceiverManufacturers()->rawValue() : QVariant(manufacturer));
        saved.setFactValue(settings->udpPort(), settings->udpPort()->rawValue());
        saved.setFactValue(settings->connectOnStartup(), false);
        saved.setFactValue(settings->useFixedBasePosition(), 0);
        saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/receiver"));
        saved.setFactValue(settings->serialBaudRate(), 115200);
        saved.setFactValue(settings->fixedBasePositionLatitude(), 0);
        saved.setFactValue(settings->fixedBasePositionLongitude(), 0);
        saved.setFactValue(settings->fixedBasePositionAltitude(), 0);
        saved.setFactValue(settings->fixedBasePositionAccuracy(), 0);
        saved.setFactValue(settings->compactRtcmCorrections(), false);
        saved.setFactValue(settings->connectionType(), GPSRtk::Serial);
        saved.setFactValue(settings->tcpHost(), QString());
        saved.setFactValue(settings->tcpPort(), 0);
    }
};

QUrl sourceUrl(const QString& filename)
{
    return QUrl::fromLocalFile(
        QFileInfo(QString::fromUtf8(__FILE__)).dir().filePath(QStringLiteral("../../../src/Toolbar/") + filename));
}

void configureEngine(QQmlEngine& engine)
{
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    if (!engine.imageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId))) {
        engine.addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());
    }
}

void setSurvey(GPSRTKFactGroup& facts)
{
    facts.currentLatitude()->setRawValue(47.123456789);
    facts.currentLongitude()->setRawValue(8.987654321);
    facts.currentAltitude()->setRawValue(512.25f);
    facts.currentAccuracy()->setRawValue(0.75);
    facts.valid()->setRawValue(true);
    facts.connected()->setRawValue(true);
}

std::unique_ptr<QObject> createPanel(QQmlEngine& engine, ReceiverSettingsController& receiver,
                                     SettingsFixture& settings, GPSRTKFactGroup& facts, QString& error)
{
    configureEngine(engine);
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("../AppSettings/GPSReceiverSettings.qml")));
    if (!QTest::qWaitFor([&]() { return !component.isLoading(); }, TestTimeout::mediumMs())) {
        error = QStringLiteral("Receiver settings component did not finish loading");
        return {};
    }
    std::unique_ptr<QObject> result(component.createWithInitialProperties(
        {{QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
         {QStringLiteral("settings"), QVariant::fromValue(settings.settings)},
         {QStringLiteral("baseFacts"), QVariant::fromValue(&facts)},
         {QStringLiteral("autoConnectFact"), QVariant::fromValue(settings.autoConnect)},
         {QStringLiteral("serialPorts"), QStringList{QStringLiteral("/test/receiver")}},
         {QStringLiteral("serialBaudRates"), QStringList{QStringLiteral("115200"), QStringLiteral("230400")}}}));
    error = component.errorString();
    return result;
}

bool invokeBool(QObject* root, const QString& expression, bool& result)
{
    QQmlExpression command(qmlContext(root), root, expression);
    result = command.evaluate().toBool();
    return !command.hasError();
}
}  // namespace

void GPSReceiverSettingsTest::_surveySaveWorkflow_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::newRow("ublox") << 4;
    QTest::newRow("quectel") << 6;
}

void GPSReceiverSettingsTest::_surveySaveWorkflow()
{
    QFETCH(int, manufacturer);
    SettingsFixture settings(manufacturer);
    ReceiverSettingsController receiver(settings.settings);
    GPSRTKFactGroup facts;
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* connect = panel->findChild<QQuickItem*>(QStringLiteral("rtkConnectButton"));
    auto* save = panel->findChild<QQuickItem*>(QStringLiteral("rtkSaveBasePosition"));
    auto* fixed = panel->findChild<QQuickItem*>(QStringLiteral("rtkFixedMode"));
    auto* manufacturerControl = panel->findChild<QQuickItem*>(QStringLiteral("rtkManufacturer"));
    QVERIFY(connect && save && fixed && manufacturerControl);
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(receiver.hasReceiver());
    QCOMPARE(receiver.selectedType, GPSRtk::typeForManufacturer(manufacturer));
    QCOMPARE(receiver.selectedMode, static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn));
    QVERIFY(!fixed->isEnabled());
    QVERIFY(!manufacturerControl->isEnabled());
    QVERIFY(!save->isEnabled());
    setSurvey(facts);
    QVERIFY(save->isVisible());
    QVERIFY(save->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(save, "click"));
    QCOMPARE(settings.settings->fixedBasePositionLatitude()->rawValue(), facts.currentLatitude()->rawValue());
    QCOMPARE(settings.settings->fixedBasePositionLongitude()->rawValue(), facts.currentLongitude()->rawValue());
    QCOMPARE(settings.settings->fixedBasePositionAltitude()->rawValue(), facts.currentAltitude()->rawValue());
    QCOMPARE(settings.settings->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.75);
    QCOMPARE(settings.settings->useFixedBasePosition()->rawValue().toInt(), 0);
    QCOMPARE(receiver.permissions, QList<bool>{false});

    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(!receiver.hasReceiver());
    facts.valid()->setRawValue(false);
    facts.currentLatitude()->setRawValue(qQNaN());
    facts.currentAccuracy()->setRawValue(qQNaN());
    QVERIFY(!save->isEnabled());
    QVERIFY(fixed->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(fixed, "click"));
    QCOMPARE(settings.settings->useFixedBasePosition()->rawValue().toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(receiver.selectedType, GPSRtk::typeForManufacturer(manufacturer));
    QCOMPARE(receiver.selectedMode, static_cast<int>(BaseModeDefinition::Mode::BaseFixed));
    QVERIFY(std::holds_alternative<GPSBaseStationConfig::Fixed>(receiver.fixedPosition.mode));
    const auto& fixedPosition = std::get<GPSBaseStationConfig::Fixed>(receiver.fixedPosition.mode);
    QCOMPARE(fixedPosition.position.latitudeDegrees, 47.123456789);
    QCOMPARE(fixedPosition.position.longitudeDegrees, 8.987654321);
    QCOMPARE(fixedPosition.position.altitudeMeters, 512.25f);
    QCOMPARE(fixedPosition.accuracyMeters, 0.75f);
    QCOMPARE(gpsValidateReceiverConfig(*receiver.selectedType, {.base = receiver.fixedPosition}),
             GPSReceiverConfigError::None);
}

void GPSReceiverSettingsTest::_unavailablePositionCannotBeSaved_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<QString>("field");
    QTest::addColumn<double>("value");
    QTest::newRow("unicore-unknown-accuracy") << 5 << QStringLiteral("currentAccuracy") << qQNaN();
    QTest::newRow("quectel-unknown-accuracy") << 6 << QStringLiteral("currentAccuracy") << qQNaN();
    QTest::newRow("negative-accuracy") << 4 << QStringLiteral("currentAccuracy") << -1.0;
    QTest::newRow("invalid-latitude") << 4 << QStringLiteral("currentLatitude") << 91.0;
    QTest::newRow("infinite-altitude") << 4 << QStringLiteral("currentAltitude") << qInf();
}

void GPSReceiverSettingsTest::_unavailablePositionCannotBeSaved()
{
    QFETCH(int, manufacturer);
    QFETCH(QString, field);
    QFETCH(double, value);
    SettingsFixture settings(manufacturer);
    ReceiverSettingsController receiver(settings.settings);
    GPSRTKFactGroup facts;
    setSurvey(facts);
    facts.property(field.toUtf8().constData()).value<Fact*>()->setRawValue(value);
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* save = panel->findChild<QQuickItem*>(QStringLiteral("rtkSaveBasePosition"));
    QVERIFY(save && !save->isEnabled());
    bool result = true;
    QVERIFY(invokeBool(panel.get(), QStringLiteral("saveCurrentBasePosition()"), result));
    QVERIFY(!result);
    QCOMPARE(settings.settings->fixedBasePositionLatitude()->rawValue().toDouble(), 0.0);
    QCOMPARE(settings.settings->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.0);
}

void GPSReceiverSettingsTest::_rtkBaseMapMarker()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), settings->baseReceiverManufacturers()->rawValue());
    saved.setFactValue(settings->useFixedBasePosition(), static_cast<int>(BaseModeDefinition::Mode::BaseFixed));
    saved.setFactValue(settings->fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 0.5);
    auto* receiver = GPSManager::instance()->gpsRtk();
    const auto retire = qScopeGuard([receiver]() { receiver->disconnectGPS(); });
    QQmlEngine engine;
    configureEngine(engine);
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("../FlightMap/MapItems/RTKBaseMapItem.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QQmlPropertyMap> map(QQmlPropertyMap::create());
    map->insert(QStringLiteral("isSatelliteMap"), false);
    std::unique_ptr<QObject> marker(
        component.createWithInitialProperties({{QStringLiteral("map"), QVariant::fromValue(map.get())}}));
    QVERIFY2(marker, qPrintable(component.errorString()));
    // Map items stay hidden outside a map, so check the bound position that drives visibility.
    QVERIFY(!marker->property("coordinate").value<QGeoCoordinate>().isValid());
    QVERIFY(receiver->connectReceiver(GPSType::ublox, [](const std::atomic_bool& stop) {
        while (!stop.load()) {
            QThread::msleep(1);
        }
        return std::unique_ptr<GPSTransport>{};
    }));
    QCOMPARE(marker->property("coordinate").value<QGeoCoordinate>(), QGeoCoordinate(47.5, 8.25));
    QVERIFY(marker->property("_final").toBool());
    QVERIFY(marker->findChild<QObject*>(QStringLiteral("rtkBaseMarker")));
    receiver->disconnectGPS();
    QVERIFY(!marker->property("coordinate").value<QGeoCoordinate>().isValid());
}

void GPSReceiverSettingsTest::_reconnectingOffersDisconnect()
{
    SettingsFixture settings(4);
    ReceiverSettingsController receiver(settings.settings);
    GPSRTKFactGroup facts;
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* connect = panel->findChild<QQuickItem*>(QStringLiteral("rtkConnectButton"));
    auto* device = panel->findChild<QQuickItem*>(QStringLiteral("rtkSerialDevice"));
    QVERIFY(connect && device);
    receiver.setReconnecting(true);
    QCOMPARE(connect->property("text").toString(), QCoreApplication::translate("GPSReceiverSettings", "Disconnect"));
    QVERIFY(connect->isEnabled());
    QVERIFY(!device->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QVERIFY(!receiver.reconnecting());
    QVERIFY(device->isEnabled());
    QCOMPARE(connect->property("text").toString(), QCoreApplication::translate("GPSReceiverSettings", "Connect"));
    QVERIFY(receiver.permissions.isEmpty());
}

void GPSReceiverSettingsTest::_tcpConnectionFields()
{
    SettingsFixture settings(4);
    ReceiverSettingsController receiver(settings.settings);
    GPSRTKFactGroup facts;
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* host = panel->findChild<QQuickItem*>(QStringLiteral("rtkTcpHost"));
    auto* port = panel->findChild<QQuickItem*>(QStringLiteral("rtkTcpPort"));
    auto* device = panel->findChild<QQuickItem*>(QStringLiteral("rtkSerialDevice"));
    auto* connectionType = panel->findChild<QQuickItem*>(QStringLiteral("rtkConnectionType"));
    auto* consent = panel->findChild<QObject*>(QStringLiteral("rtkPersistentChangesCheckBox"));
    QVERIFY(host && port && device && connectionType && consent);
    QVERIFY(connectionType->isVisible());
    QVERIFY(device->isVisible());
    QVERIFY(!host->isVisible() && !port->isVisible());
    settings.settings->connectionType()->setRawValue(GPSRtk::Tcp);
    QVERIFY(host->isVisible() && port->isVisible());
    QVERIFY(!device->isVisible());
    settings.settings->baseReceiverManufacturers()->setRawValue(6);
    QVERIFY(consent->property("visible").toBool());
    for (Fact* fact : {settings.settings->tcpHost(), settings.settings->tcpPort()}) {
        QVERIFY(QMetaObject::invokeMethod(consent, "click"));
        QVERIFY(consent->property("checked").toBool());
        fact->setRawValue(fact == settings.settings->tcpPort() ? QVariant(2101) : QVariant(QStringLiteral("bridge")));
        QVERIFY(!consent->property("checked").toBool());
    }
    receiver.setConnected(true);
    QVERIFY(!host->isEnabled() && !port->isEnabled());
}

void GPSReceiverSettingsTest::_roleSelectsFields()
{
    SettingsFixture settings(4);
    ReceiverSettingsController receiver(settings.settings);
    GPSRTKFactGroup facts;
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* role = panel->findChild<QQuickItem*>(QStringLiteral("receiverRole"));
    auto* manufacturer = panel->findChild<QQuickItem*>(QStringLiteral("rtkManufacturer"));
    auto* survey = panel->findChild<QQuickItem*>(QStringLiteral("rtkSurveyMode"));
    auto* udpPort = panel->findChild<QQuickItem*>(QStringLiteral("rtkUdpPort"));
    auto* udpExplanation = panel->findChild<QQuickItem*>(QStringLiteral("rtkUdpExplanation"));
    auto* device = panel->findChild<QQuickItem*>(QStringLiteral("rtkSerialDevice"));
    auto* startup = panel->findChild<QQuickItem*>(QStringLiteral("rtkConnectOnStartup"));
    auto* connect = panel->findChild<QQuickItem*>(QStringLiteral("rtkConnectButton"));
    QVERIFY(role && manufacturer && survey && udpPort && udpExplanation && device && startup && connect);
    QVERIFY(role->isVisible() && manufacturer->isVisible() && survey->isVisible());
    QVERIFY(startup->isVisible());
    QVERIFY(connect->isEnabled());

    // A receiver QGroundControl does not configure hides every base setting.
    settings.settings->receiverRole()->setRawValue(GPSRtk::PositionOnly);
    QVERIFY(!manufacturer->isVisible());
    QVERIFY(!survey->isVisible());
    QVERIFY(device->isVisible());
    QVERIFY(connect->isEnabled());

    settings.settings->connectionType()->setRawValue(GPSRtk::Udp);
    QVERIFY(udpPort->isVisible() && udpExplanation->isVisible());
    QVERIFY(!device->isVisible());
    QVERIFY(connect->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QCOMPARE(receiver.selectedType, std::optional(GPSType::passive));
    receiver.setConnected(false);

    // UDP cannot carry base configuration.
    settings.settings->receiverRole()->setRawValue(GPSRtk::ConfiguredBase);
    QVERIFY(udpPort->isVisible());
    QVERIFY(!connect->isEnabled());
    QVERIFY(udpExplanation->property("text").toString().contains(QStringLiteral("serial or TCP")));
    settings.settings->connectionType()->setRawValue(GPSRtk::Serial);
    QVERIFY(!udpPort->isVisible());
    QVERIFY(connect->isEnabled());
}

void GPSReceiverSettingsTest::_compactCorrectionsToggle()
{
    SettingsFixture settings(4);
    ReceiverSettingsController receiver(settings.settings);
    GPSRTKFactGroup facts;
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* toggle = panel->findChild<QQuickItem*>(QStringLiteral("rtkCompactRtcm"));
    QVERIFY(toggle);
    QVERIFY(toggle->isVisible());
    QVERIFY(toggle->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(settings.settings->compactRtcmCorrections()->rawValue().toBool());
    for (const int manufacturer : {2, 5, 6, 7}) {
        settings.settings->baseReceiverManufacturers()->setRawValue(manufacturer);
        QVERIFY2(!toggle->isVisible(), qPrintable(QString::number(manufacturer)));
    }
    settings.settings->baseReceiverManufacturers()->setRawValue(4);
    QVERIFY(toggle->isVisible());
    receiver.setConnected(true);
    QVERIFY(!toggle->isEnabled());
}

void GPSReceiverSettingsTest::_consentIsOneUse()
{
    SettingsFixture settings(6);
    ReceiverSettingsController receiver(settings.settings);
    receiver.connectSucceeds = false;
    GPSRTKFactGroup facts;
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* consent = panel->findChild<QObject*>(QStringLiteral("rtkPersistentChangesCheckBox"));
    auto* connect = panel->findChild<QObject*>(QStringLiteral("rtkConnectButton"));
    QVERIFY(consent && connect);
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(receiver.permissions, QList<bool>{true});
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(receiver.permissions, (QList<bool>{true, false}));

    for (const auto& [fact, value] :
         QList<QPair<Fact*, QVariant>>{{settings.settings->serialDevice(), QStringLiteral("/test/other")},
                                       {settings.settings->serialBaudRate(), 230400},
                                       {settings.settings->useFixedBasePosition(), 1},
                                       {settings.settings->baseReceiverManufacturers(), 5}}) {
        QVERIFY(QMetaObject::invokeMethod(consent, "click"));
        QVERIFY(consent->property("checked").toBool());
        fact->setRawValue(value);
        QVERIFY(!consent->property("checked").toBool());
    }
    QVERIFY(!consent->property("visible").toBool());
    settings.settings->baseReceiverManufacturers()->setRawValue(6);
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    receiver.setConnected(true);
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(!consent->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    panel.reset();
    panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    QVERIFY(!panel->findChild<QObject*>(QStringLiteral("rtkPersistentChangesCheckBox"))->property("checked").toBool());
}

void GPSReceiverSettingsTest::_warningWidth_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<bool>("longText");
    for (const int width : {320, 480, 1200}) {
        QTest::newRow(qPrintable(QStringLiteral("width-%1").arg(width))) << width << false;
        QTest::newRow(qPrintable(QStringLiteral("translated-width-%1").arg(width))) << width << true;
    }
}

void GPSReceiverSettingsTest::_warningWidth()
{
    QFETCH(int, width);
    QFETCH(bool, longText);
    SettingsFixture settings(6);
    ReceiverSettingsController receiver(settings.settings);
    GPSRTKFactGroup facts;
    QQuickWindow window;
    window.resize(width, 800);
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* item = qobject_cast<QQuickItem*>(panel.get());
    QVERIFY(item);
    item->setParentItem(window.contentItem());
    item->setWidth(width);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    for (const auto* name : {"rtkPersistentConfigurationWarning", "rtkPersistentConsentWarning"}) {
        auto* warning = panel->findChild<QQuickItem*>(QString::fromLatin1(name));
        QVERIFY(warning);
        if (longText) {
            const QString translated = QStringLiteral(
                "Empfänger-Konfiguration und dauerhafte Speicherung benötigen Ihre ausdrückliche "
                "Zustimmung. Änderungen können trotz fehlgeschlagener Verbindung gespeichert bleiben. ");
            QVERIFY(warning->setProperty("text", translated.repeated(4)));
        }
        // Text changes its content height before the enclosing layout's next polish.
        QTRY_VERIFY_WITH_TIMEOUT(warning->width() > 0 && warning->property("lineCount").toInt() > 1 &&
                                     warning->height() >= warning->property("contentHeight").toDouble(),
                                 TestTimeout::mediumMs());
        const auto bounds = warning->mapRectToItem(item, warning->boundingRect());
        QVERIFY2(bounds.left() >= 0 && bounds.right() <= item->width() + 1,
                 qPrintable(QStringLiteral("%1: text bounds %2..%3, panel width %4")
                                .arg(QString::fromLatin1(name))
                                .arg(bounds.left())
                                .arg(bounds.right())
                                .arg(item->width())));
        QVERIFY(item->implicitWidth() < 1200);
    }
}

void GPSReceiverSettingsTest::_pageWidth_data()
{
    QTest::addColumn<int>("width");
    QTest::newRow("mobile") << 320;
    QTest::newRow("narrow-desktop") << 640;
    QTest::newRow("desktop") << 1200;
}

void GPSReceiverSettingsTest::_pageWidth()
{
    QFETCH(int, width);
    SettingsFixture settings(6);
    QQuickWindow window;
    window.resize(width, 800);
    QQmlEngine engine;
    configureEngine(engine);
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("GPSIndicatorPage.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> page(component.createWithInitialProperties(
        {{QStringLiteral("availableWidth"), width}, {QStringLiteral("expanded"), true}}));
    QVERIFY2(page, qPrintable(component.errorString()));
    auto* item = qobject_cast<QQuickItem*>(page.get());
    QVERIFY(item);
    item->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    auto* panel = page->findChild<QQuickItem*>(QStringLiteral("gpsReceiverSettings"));
    QVERIFY(panel);
    QTRY_VERIFY_WITH_TIMEOUT(panel->width() > 0, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(item->width() <= width + 1, TestTimeout::mediumMs());
    const auto bounds = panel->mapRectToItem(item, panel->boundingRect());
    QVERIFY(bounds.right() <= width + 1);
}

void GPSReceiverSettingsTest::_disconnectedPage_data()
{
    _pageWidth_data();
}

void GPSReceiverSettingsTest::_disconnectedPage()
{
    QFETCH(int, width);
    SettingsFixture settings(4);
    auto* receiver = GPSManager::instance()->gpsRtk();
    QVERIFY(!receiver->hasReceiver());
    auto* facts = receiver->gpsRtkFactGroup();
    settings.saved.setFactValue(facts->connected(), false);
    settings.saved.setFactValue(facts->active(), false);
    settings.saved.setFactValue(facts->numSatellites(), 12);
    settings.saved.setFactValue(facts->numSatellitesUsed(), 7);

    QQuickWindow window;
    window.resize(width, 800);
    QQmlEngine engine;
    configureEngine(engine);
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("GPSIndicatorPage.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> page(component.createWithInitialProperties(
        {{QStringLiteral("availableWidth"), width},
         {QStringLiteral("activeVehicle"), QVariant::fromValue(static_cast<QObject*>(nullptr))}}));
    QVERIFY2(page, qPrintable(component.errorString()));
    auto* item = qobject_cast<QQuickItem*>(page.get());
    QVERIFY(item);
    item->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    QVERIFY(!page->property("expanded").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(item->height() > 0 && item->implicitHeight() > 0, TestTimeout::shortMs());
    QVERIFY(item->width() <= width + 1);
    QVERIFY(page->property("_showExpand").toBool());

    auto* status = page->findChild<QQuickItem*>(QStringLiteral("rtkReceiverStatus"));
    auto* satellites = page->findChild<QQuickItem*>(QStringLiteral("rtkSatellitesInView"));
    auto* usage = page->findChild<QQuickItem*>(QStringLiteral("rtkSatellitesUsed"));
    QVERIFY(status && satellites && usage);
    QVERIFY(status->isVisible());
    const QString disconnectedText = status->property("text").toString();
    QVERIFY(!disconnectedText.isEmpty());
    QVERIFY(!satellites->isVisible());
    QVERIFY(!usage->isVisible());
    QVERIFY(status->mapRectToItem(item, status->boundingRect()).right() <= width + 1);

    facts->connected()->setRawValue(true);
    // FactGroup publishes at 1 Hz; allow that interval plus event-loop scheduling.
    QTRY_VERIFY_WITH_TIMEOUT(satellites->isVisible() && usage->isVisible(), TestTimeout::mediumMs());
    QVERIFY(status->property("text").toString() != disconnectedText);
    facts->connected()->setRawValue(false);
    QTRY_VERIFY_WITH_TIMEOUT(status->isVisible() && !satellites->isVisible() && !usage->isVisible(),
                             TestTimeout::mediumMs());
    QCOMPARE(status->property("text").toString(), disconnectedText);
    QVERIFY(item->height() > 0);

    QVERIFY(page->setProperty("expanded", true));
    QTRY_VERIFY_WITH_TIMEOUT(page->findChild<QQuickItem*>(QStringLiteral("gpsReceiverSettings")),
                             TestTimeout::shortMs());
    auto* connect = page->findChild<QQuickItem*>(QStringLiteral("rtkConnectButton"));
    QVERIFY(connect);
    // TCP connections are available even without serial support.
    QVERIFY(connect->isVisible());
    QVERIFY(connect->isEnabled());
    QVERIFY(item->width() <= width + 1);
}

void GPSReceiverSettingsTest::_serialSelectionTracksFacts()
{
    SettingsFixture settings(4);
    ReceiverSettingsController receiver(settings.settings);
    GPSRTKFactGroup facts;
    QQmlEngine engine;
    QString error;
    auto panel = createPanel(engine, receiver, settings, facts, error);
    QVERIFY2(panel, qPrintable(error));
    auto* device = panel->findChild<QObject*>(QStringLiteral("rtkSerialDevice"));
    auto* baud = panel->findChild<QObject*>(QStringLiteral("rtkSerialBaudRate"));
    auto* custom = panel->findChild<QObject*>(QStringLiteral("rtkCustomBaudRate"));
    QVERIFY(device && baud && custom);
    QCOMPARE(device->property("currentText").toString(), QStringLiteral("/test/receiver"));
    QCOMPARE(baud->property("currentText").toString(), QStringLiteral("115200"));
    settings.settings->serialDevice()->setRawValue(QStringLiteral("/test/missing"));
    QVERIFY(device->property("currentText").toString().contains(QStringLiteral("/test/missing")));
    QVERIFY(device->property("currentText").toString().contains(QStringLiteral("unavailable")));
    QVERIFY(panel->setProperty("serialPorts", QStringList{QStringLiteral("/test/missing")}));
    QCOMPARE(device->property("currentText").toString(), QStringLiteral("/test/missing"));
    settings.settings->serialBaudRate()->setRawValue(123457);
    QVERIFY(baud->property("isCustomBaud").toBool());
    QCOMPARE(custom->property("text").toString(), QStringLiteral("123457"));
    settings.settings->serialBaudRate()->setRawValue(230400);
    QVERIFY(!baud->property("isCustomBaud").toBool());
    QCOMPARE(baud->property("currentText").toString(), QStringLiteral("230400"));
    QVERIFY(QMetaObject::invokeMethod(baud, "activated", Q_ARG(int, 3)));
    QVERIFY(baud->property("isCustomBaud").toBool());
    QVERIFY(custom->setProperty("text", QStringLiteral("250000")));
    QVERIFY(QMetaObject::invokeMethod(custom, "_onEditingFinished"));
    QCOMPARE(settings.settings->serialBaudRate()->rawValue().toInt(), 250000);
    QVERIFY(QMetaObject::invokeMethod(baud, "activated", Q_ARG(int, 0)));
    QCOMPARE(settings.settings->serialBaudRate()->rawValue().toInt(), 0);
    QCOMPARE(baud->property("currentText").toString(), QCoreApplication::translate("GPSReceiverSerialPort", "Auto"));
    // Passive roles need the receiver's existing rate, so Auto is not offered.
    settings.settings->receiverRole()->setRawValue(GPSRtk::Passive);
    QVERIFY(baud->property("isCustomBaud").toBool());
    settings.settings->receiverRole()->setRawValue(GPSRtk::ConfiguredBase);
    settings.settings->baseReceiverManufacturers()->setRawValue(4);
    settings.settings->serialBaudRate()->setRawValue(230400);
    receiver.setConnected(true);
    QVERIFY(!device->property("enabled").toBool());
    QVERIFY(!baud->property("enabled").toBool());
    QVERIFY(!custom->property("enabled").toBool());
}

void GPSReceiverSettingsTest::_resilienceUnknownStates_data()
{
    QTest::addColumn<int>("spoofing");
    QTest::addColumn<int>("jamming");
    for (const int spoofing : {0, 1, 2, 3, 255}) {
        for (const int jamming : {0, 1, 2, 3, 255}) {
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(spoofing).arg(jamming))) << spoofing << jamming;
        }
    }
}

void GPSReceiverSettingsTest::_resilienceUnknownStates()
{
    QFETCH(int, spoofing);
    QFETCH(int, jamming);
    Fact spoofingFact(0, QStringLiteral("spoofing"), FactMetaData::valueTypeUint8);
    Fact jammingFact(0, QStringLiteral("jamming"), FactMetaData::valueTypeUint8);
    Fact authenticationFact(0, QStringLiteral("authentication"), FactMetaData::valueTypeUint8);
    spoofingFact.setRawValue(spoofing);
    jammingFact.setRawValue(jamming);
    authenticationFact.setRawValue(255);
    std::unique_ptr<QQmlPropertyMap> aggregate(QQmlPropertyMap::create());
    aggregate->insert(QStringLiteral("spoofingState"), QVariant::fromValue(&spoofingFact));
    aggregate->insert(QStringLiteral("jammingState"), QVariant::fromValue(&jammingFact));
    aggregate->insert(QStringLiteral("authenticationState"), QVariant::fromValue(&authenticationFact));
    QQuickWindow window;
    QQmlEngine engine;
    configureEngine(engine);
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("GPSIndicator.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> indicator(component.createWithInitialProperties(
        {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
         {QStringLiteral("_activeVehicle"), QVariant::fromValue(static_cast<QObject*>(nullptr))},
         {QStringLiteral("_gpsAggregate"), QVariant::fromValue(aggregate.get())}}));
    QVERIFY2(indicator, qPrintable(component.errorString()));
    const int expected = qMax(spoofing == 255 ? 0 : spoofing, jamming == 255 ? 0 : jamming);
    QCOMPARE(indicator->property("_interferenceState").toInt(), expected);
    auto* icon = indicator->findChild<QObject*>(QStringLiteral("gpsInterferenceIcon"));
    QVERIFY(icon);
    QCOMPARE(icon->property("visible").toBool(), expected > 0);
    QVERIFY(indicator->setProperty("_gpsAggregate", QVariant::fromValue(static_cast<QObject*>(nullptr))));
    QCOMPARE(indicator->property("_interferenceState").toInt(), 0);
    QVERIFY(!icon->property("visible").toBool());
}

void GPSReceiverSettingsTest::_horizontalAccuracyLabel()
{
    QQmlEngine engine;
    configureEngine(engine);
    const QUrl url =
        QUrl::fromLocalFile(QFileInfo(QString::fromUtf8(__FILE__))
                                .dir()
                                .filePath(QStringLiteral("../../../src/AppSettings/GcsPositionStatus.qml")));
    QQmlComponent component(&engine, url);
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> panel(
        component.createWithInitialProperties({{QStringLiteral("_horizontalAccuracy"), 5.1}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* accuracy = panel->findChild<QObject*>(QStringLiteral("gcsHorizontalAccuracy"));
    QVERIFY(accuracy);
    QCOMPARE(accuracy->property("label").toString(), QStringLiteral("Horizontal accuracy"));
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("5.1 m"));
    QVERIFY(panel->setProperty("_horizontalAccuracy", 0));
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("0.0 m"));
    QVERIFY(panel->setProperty("_horizontalAccuracy", qInf()));
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("N/A"));
}

void GPSReceiverSettingsTest::_vehicleAccuracyFacts()
{
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps = qobject_cast<VehicleGPSFactGroup*>(vehicle.gpsFactGroup());
    QVERIFY(gps);
    gps->setLiveUpdates(true);
    // The page shows vehicle GPS status only once the vehicle reports GPS telemetry.
    mavlink_gps_raw_int_t raw{};
    raw.fix_type = GPS_FIX_TYPE_3D_FIX;
    raw.eph = UINT16_MAX;
    raw.epv = UINT16_MAX;
    raw.cog = UINT16_MAX;
    raw.satellites_visible = 10;
    mavlink_message_t message{};
    mavlink_msg_gps_raw_int_encode(1, 1, &message, &raw);
    gps->handleMessage(&vehicle, message);
    QVERIFY(gps->telemetryAvailable());
    gps->hdop()->setRawValue(0.8);
    gps->vdop()->setRawValue(1.2);
    gps->horizontalAccuracy()->setRawValue(2.5);
    gps->verticalAccuracy()->setRawValue(4.5);
    QQuickWindow window;
    window.resize(640, 800);
    QQmlEngine engine;
    configureEngine(engine);
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("GPSIndicatorPage.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> page(
        component.createWithInitialProperties({{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                                               {QStringLiteral("activeVehicle"), QVariant::fromValue(&vehicle)},
                                               {QStringLiteral("availableWidth"), 640}}));
    QVERIFY2(page, qPrintable(component.errorString()));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    auto* hdop = page->findChild<QObject*>(QStringLiteral("vehicleGpsHdop"));
    auto* vdop = page->findChild<QObject*>(QStringLiteral("vehicleGpsVdop"));
    auto* horizontal = page->findChild<QObject*>(QStringLiteral("vehicleGpsHorizontalAccuracy"));
    auto* vertical = page->findChild<QObject*>(QStringLiteral("vehicleGpsVerticalAccuracy"));
    QVERIFY(hdop && vdop && horizontal && vertical);
    QCOMPARE(hdop->property("labelText").toString(), gps->hdop()->cookedValueString());
    QCOMPARE(vdop->property("labelText").toString(), gps->vdop()->cookedValueString());
    const auto accuracyText = [](Fact* fact) {
        return fact->cookedValueString() + QLatin1Char(' ') + fact->cookedUnits();
    };
    QCOMPARE(horizontal->property("labelText").toString(), accuracyText(gps->horizontalAccuracy()));
    QCOMPARE(vertical->property("labelText").toString(), accuracyText(gps->verticalAccuracy()));
    QVERIFY(horizontal->property("visible").toBool());
    QVERIFY(vertical->property("visible").toBool());
    gps->horizontalAccuracy()->setRawValue(qQNaN());
    QTRY_VERIFY_WITH_TIMEOUT(!horizontal->property("visible").toBool(), TestTimeout::shortMs());
    QCOMPARE(hdop->property("labelText").toString(), gps->hdop()->cookedValueString());
    gps->hdop()->setRawValue(qQNaN());
    gps->vdop()->setRawValue(qQNaN());
    gps->horizontalAccuracy()->setRawValue(5.0);
    gps->verticalAccuracy()->setRawValue(10.0);
    QTRY_VERIFY_WITH_TIMEOUT(horizontal->property("visible").toBool(), TestTimeout::shortMs());
    QCOMPARE(horizontal->property("labelText").toString(), accuracyText(gps->horizontalAccuracy()));
    QCOMPARE(vertical->property("labelText").toString(), accuracyText(gps->verticalAccuracy()));
    QCOMPARE(hdop->property("labelText").toString(), gps->hdop()->cookedValueString());
    QCOMPARE(vdop->property("labelText").toString(), gps->vdop()->cookedValueString());
    QVERIFY(page->setProperty("activeVehicle", QVariant::fromValue(static_cast<Vehicle*>(nullptr))));
    QVERIFY(!horizontal->property("visible").toBool());
    QVERIFY(!vertical->property("visible").toBool());
}

void GPSReceiverSettingsTest::_indicatorShowsReceiverWithoutVehicleGps_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("fixType");
    QTest::addColumn<bool>("surveying");
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("detail");
    QTest::newRow("position-only") << int(GPSRtk::PositionOnly) << 3 << false << "GNSS" << "3D";
    QTest::newRow("passive-float") << int(GPSRtk::Passive) << 5 << false << "RTK" << "Float";
    QTest::newRow("base-surveying") << int(GPSRtk::ConfiguredBase) << 0 << true << "RTK" << "Survey";
    QTest::newRow("base-fixed") << int(GPSRtk::ConfiguredBase) << 0 << false << "RTK" << "Base";
}

void GPSReceiverSettingsTest::_indicatorShowsReceiverWithoutVehicleGps()
{
    QFETCH(int, role);
    QFETCH(int, fixType);
    QFETCH(bool, surveying);
    QFETCH(QString, label);
    QFETCH(QString, detail);
    IndicatorReceiver receiver;
    receiver.activeRole = role;
    auto& facts = receiver.factGroup;
    facts.fixType()->setRawValue(fixType);
    facts.numSatellitesUsed()->setRawValue(9);
    facts.active()->setRawValue(surveying);
    QQuickWindow window;
    QQmlEngine engine;
    configureEngine(engine);
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("GPSIndicator.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> indicator(component.createWithInitialProperties(
        {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
         {QStringLiteral("_activeVehicle"), QVariant::fromValue(static_cast<QObject*>(nullptr))},
         {QStringLiteral("_receiver"), QVariant::fromValue(&receiver)}}));
    QVERIFY2(indicator, qPrintable(component.errorString()));
    QVERIFY(indicator->property("showIndicator").toBool());
    auto* rtkLabel = indicator->findChild<QObject*>(QStringLiteral("gpsCorrectionsLabel"));
    auto* satellites = indicator->findChild<QObject*>(QStringLiteral("gpsSatelliteCount"));
    auto* detailLabel = indicator->findChild<QObject*>(QStringLiteral("gpsDetail"));
    QVERIFY(rtkLabel && satellites && detailLabel);
    QVERIFY(!rtkLabel->property("visible").toBool());
    QVERIFY(!satellites->property("visible").toBool());

    facts.connected()->setRawValue(true);
    QVERIFY(rtkLabel->property("visible").toBool());
    QCOMPARE(rtkLabel->property("text").toString(), label);
    QVERIFY(satellites->property("visible").toBool());
    QCOMPARE(satellites->property("text").toString(), QStringLiteral("9"));
    QCOMPARE(detailLabel->property("text").toString(), detail);
    // Only receivers that supply corrections expect them; none arrive here, so the label warns.
    QCOMPARE(indicator->property("_correctionsExpected").toBool(), role != GPSRtk::PositionOnly);
    QVERIFY(!indicator->property("_correctionsFresh").toBool());
}

void GPSReceiverSettingsTest::_resiliencePageGroups()
{
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps1 = qobject_cast<VehicleGPSFactGroup*>(vehicle.gpsFactGroup());
    auto* gps2 = qobject_cast<VehicleGPSFactGroup*>(vehicle.gps2FactGroup());
    QVERIFY(gps1 && gps2);
    for (FactGroup* group : {vehicle.gpsFactGroup(), vehicle.gps2FactGroup(), vehicle.gpsAggregateFactGroup()}) {
        group->setLiveUpdates(true);
    }
    const auto integrity = [&vehicle](VehicleGPSFactGroup* gps, uint8_t id, uint8_t spoofing, uint8_t jamming) {
        mavlink_gnss_integrity_t report{};
        report.id = id;
        report.spoofing_state = spoofing;
        report.jamming_state = jamming;
        report.authentication_state = 0;
        mavlink_message_t message{};
        mavlink_msg_gnss_integrity_encode(1, 1, &message, &report);
        gps->handleMessage(&vehicle, message);
    };
    QQuickWindow window;
    window.resize(640, 800);
    QQmlEngine engine;
    configureEngine(engine);
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("GPSIndicatorPage.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> page(
        component.createWithInitialProperties({{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                                               {QStringLiteral("activeVehicle"), QVariant::fromValue(&vehicle)},
                                               {QStringLiteral("availableWidth"), 640}}));
    QVERIFY2(page, qPrintable(component.errorString()));
    auto* summary = page->findChild<QObject*>(QStringLiteral("gpsResilienceStatus"));
    auto* first = page->findChild<QObject*>(QStringLiteral("gps1Resilience"));
    auto* second = page->findChild<QObject*>(QStringLiteral("gps2Resilience"));
    QVERIFY(summary && first && second);
    QVERIFY(!summary->property("visible").toBool());

    // With one receiver reporting, its details would repeat the summary.
    integrity(gps1, 0, 1, 3);
    QTRY_VERIFY_WITH_TIMEOUT(summary->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(first->property("reported").toBool());
    QVERIFY(!first->property("visible").toBool());
    QVERIFY(!second->property("visible").toBool());

    integrity(gps2, 1, 2, 1);
    QTRY_VERIFY_WITH_TIMEOUT(second->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(first->property("visible").toBool());
    QVERIFY(summary->property("visible").toBool());
}

UT_REGISTER_TEST(GPSReceiverSettingsTest, TestLabel::Unit)

#include "GPSReceiverSettingsTest.moc"
