#include "GPSReceiverSettingsTest.h"

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include "AutoConnectSettings.h"
#include "ColoredSvgImageProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSRTKFactGroup.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSRtk.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

/// Records the QML command boundary without opening a device or configuring hardware.
class ReceiverSettingsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasReceiver READ hasReceiver NOTIFY receiverChanged)
    Q_PROPERTY(bool serialSupported READ serialSupported CONSTANT)

public:
    explicit ReceiverSettingsController(RTKSettings* settings)
        : _settings(settings)
    {}

    bool hasReceiver() const { return _hasReceiver; }

    bool serialSupported() const { return true; }

    void setConnected(bool connected)
    {
        _hasReceiver = connected;
        emit receiverChanged();
    }

    Q_INVOKABLE QVariantMap capabilitiesForManufacturer(int manufacturer) const
    {
        return gpsReceiverPresentation(manufacturer);
    }

    Q_INVOKABLE bool connectConfiguredGPS(bool allowPersistentChanges = false)
    {
        permissions.append(allowPersistentChanges);
        selectedType = GPSRtk::typeForManufacturer(_settings->baseReceiverManufacturers()->rawValue().toInt());
        selectedMode = _settings->useFixedBasePosition()->rawValue().toInt();
        fixedPosition = {
            .useFixedBase = selectedMode == static_cast<int>(BaseModeDefinition::Mode::BaseFixed),
            .fixedPosition = {.latitudeDegrees = _settings->fixedBasePositionLatitude()->rawValue().toDouble(),
                              .longitudeDegrees = _settings->fixedBasePositionLongitude()->rawValue().toDouble(),
                              .altitudeMeters = _settings->fixedBasePositionAltitude()->rawValue().toFloat()},
            .fixedBaseAccuracyMeters = _settings->fixedBasePositionAccuracy()->rawValue().toFloat()};
        if (connectSucceeds) {
            setConnected(true);
        }
        return connectSucceeds;
    }

    Q_INVOKABLE void disconnectConfiguredGPS() { setConnected(false); }

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
};

namespace {
struct SettingsFixture
{
    TestFixtures::SettingsFixture saved;
    RTKSettings* settings = SettingsManager::instance()->rtkSettings();
    Fact* autoConnect = SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS();

    explicit SettingsFixture(int manufacturer)
    {
        saved.setFactValue(autoConnect, false);
        saved.setFactValue(settings->baseReceiverManufacturers(), manufacturer);
        saved.setFactValue(settings->useFixedBasePosition(), 0);
        saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/receiver"));
        saved.setFactValue(settings->serialBaudRate(), 115200);
        saved.setFactValue(settings->fixedBasePositionLatitude(), 0);
        saved.setFactValue(settings->fixedBasePositionLongitude(), 0);
        saved.setFactValue(settings->fixedBasePositionAltitude(), 0);
        saved.setFactValue(settings->fixedBasePositionAccuracy(), 0);
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
    QQmlComponent component(&engine, sourceUrl(QStringLiteral("GPSReceiverSettings.qml")));
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
    QVERIFY(receiver.fixedPosition.useFixedBase);
    QCOMPARE(receiver.fixedPosition.fixedPosition.latitudeDegrees, 47.123456789);
    QCOMPARE(receiver.fixedPosition.fixedPosition.longitudeDegrees, 8.987654321);
    QCOMPARE(receiver.fixedPosition.fixedPosition.altitudeMeters, 512.25f);
    QCOMPARE(receiver.fixedPosition.fixedBaseAccuracyMeters, 0.75f);
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
    QTRY_VERIFY_WITH_TIMEOUT(satellites->isVisible() && usage->isVisible(), TestTimeout::shortMs());
    QVERIFY(status->property("text").toString() != disconnectedText);
    facts->connected()->setRawValue(false);
    QTRY_VERIFY_WITH_TIMEOUT(status->isVisible() && !satellites->isVisible() && !usage->isVisible(),
                             TestTimeout::shortMs());
    QCOMPARE(status->property("text").toString(), disconnectedText);
    QVERIFY(item->height() > 0);

    QVERIFY(page->setProperty("expanded", true));
    QTRY_VERIFY_WITH_TIMEOUT(page->findChild<QQuickItem*>(QStringLiteral("gpsReceiverSettings")),
                             TestTimeout::shortMs());
    auto* connect = page->findChild<QQuickItem*>(QStringLiteral("rtkConnectButton"));
    QVERIFY(connect);
    QCOMPARE(connect->isVisible(), receiver->serialSupported());
    if (receiver->serialSupported()) {
        QVERIFY(connect->isEnabled());
    }
    QVERIFY(item->width() <= width + 1);
}

UT_REGISTER_TEST(GPSReceiverSettingsTest, TestLabel::Unit)

#include "GPSReceiverSettingsTest.moc"
