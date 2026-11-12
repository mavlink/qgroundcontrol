#include "GPSReceiverSettingsTest.h"

#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtPositioning/QGeoCoordinate>
#include <QtQml/QJSValue>
#include <QtQuick/QQuickItem>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "QGCPalette.h"
#include "Qml/Support/GPSPanelFixtures.h"
#include "QmlTesting/QmlFixtures.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"
#include "UnitsSettings.h"

using namespace GPSTest;

namespace {
/// A receiver session that follows the settings; scripted workers stand in for the receiver hardware. Without
/// @a serialLinks the receiver has the serial side of a build without serial links.
struct SettingsReceiver
{
    SettingsReceiver(RTKSettings* settings, bool serialLinks)
        : scripted(nullptr, dependencies(serialLinks))
    {
        GPSSettingsBindings::bindReceiver(settings, &gps);
    }

    ScriptedReceiverWorker* worker() const { return workers.current(); }

    // Scripted workers open no link, so a recorded session names a file here and never creates it.
    static QString recordingDirectory() { return QStringLiteral("/test/recordings"); }

    GPSReceiver::Dependencies dependencies([[maybe_unused]] bool serialLinks)
    {
        GPSReceiver::Dependencies dependencies{.serial = withoutSerialLinks, .recordingDirectory = recordingDirectory};
#ifndef QGC_NO_SERIAL_LINK
        if (serialLinks) {
            dependencies.serial = serial.side();
        }
#endif
        return dependencies;
    }

#ifndef QGC_NO_SERIAL_LINK
    // Every device the tests select; the panel's own port list decides which ones it shows as available.
    TestSerialPorts serial{{serialPort(QStringLiteral("/test/receiver")), serialPort(QStringLiteral("/test/other")),
                            serialPort(QStringLiteral("/test/missing"))}};
#endif
    GPSTest::ScriptedGPSReceiver scripted;
    GPSReceiver& gps = scripted.receiver;
    ScriptedReceiverWorkerFactory& workers = scripted.workers;
};

/// A serial device picker entry, labelled with its path unless @a label is given.
QVariant portEntries(const QString& path, const QString& label = {})
{
    return QVariant::fromValue(QList<GPSSerialPortEntry>{{.value = path, .label = label.isEmpty() ? path : label}});
}

/// Serial device picker entries labelled with their paths.
QVariant portEntries(const QStringList& paths)
{
    QList<GPSSerialPortEntry> entries;
    for (const QString& path : paths) {
        entries.append({.value = path, .label = path});
    }
    return QVariant::fromValue(entries);
}

/// The receiver settings panel over saved settings for a manufacturer, a scripted receiver that follows them, and the
/// receiver status, which has Connect, as the settings page and the indicator's drop-down show them together.
struct Panel
{
    explicit Panel(int manufacturer, bool serialLinks = true)
        : settings(manufacturer)
        , receiver(settings.settings, serialLinks)
    {}

    /// Creates the panel and the status; false on failure, with the reason in the engine's lastError().
    [[nodiscard]] bool create()
    {
        item = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverSettings.qml")),
                             {{QStringLiteral("receiver"), QVariant::fromValue(&receiver.gps)},
                              {QStringLiteral("settings"), QVariant::fromValue(settings.settings)},
                              {QStringLiteral("autoConnectFact"), QVariant::fromValue(settings.autoConnect)},
                              {QStringLiteral("serialPorts"), portEntries(QStringLiteral("/test/receiver"))},
                              {QStringLiteral("serialBaudRates"), QVariant::fromValue(QList<int>{115200, 230400})}});
        if (!item) {
            return false;
        }
        status = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                               {{QStringLiteral("receiver"), QVariant::fromValue(&receiver.gps)},
                                {QStringLiteral("facts"), QVariant::fromValue(receiver.gps.facts())},
                                {QStringLiteral("showWhenDisconnected"), true}});
        return status != nullptr;
    }

    /// A control of the panel, else of the status.
    template <typename T = QQuickItem>
    T* find(const char* name) const
    {
        const QString objectName = QString::fromLatin1(name);
        T* const control = item->findChild<T*>(objectName);
        return control ? control : status->findChild<T*>(objectName);
    }

    RTKSettingsFixture settings;
    SettingsReceiver receiver;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> item;
    std::unique_ptr<QObject> status;
};

/// Whether @a label, a text item, shows @a mark whole. A text wider than its item is cut on the right: the device
/// picker elides its button's there, and its popup clips an entry's.
bool showsWhole(const QQuickItem* label, const QString& mark)
{
    const QString text = label->property("text").toString();
    const qsizetype at = text.indexOf(mark);
    if (at < 0) {
        return false;
    }
    if (label->implicitWidth() <= label->width()) {
        return true;
    }
    const QFontMetricsF metrics(label->property("font").value<QFont>());
    return metrics.horizontalAdvance(text.left(at + mark.size()) + QChar(0x2026)) <= label->width();
}

/// The receiver status over a scripted base of @a family, connected and ready, whose status Facts publish at once.
/// The base surveys its position for 180 s to 2 m where the family surveys, and averages it otherwise.
struct BaseStatus
{
    explicit BaseStatus(GPSType family, const std::optional<GPSBaseStationConfig>& base = {})
    {
        TestFixtures::SettingsFixture& saved = units;
        UnitsSettings* const unitsSettings = SettingsManager::instance()->unitsSettings();
        saved.setFactValue(unitsSettings->horizontalDistanceUnits(), UnitsSettings::HorizontalDistanceUnitsMeters);
        saved.setFactValue(unitsSettings->verticalDistanceUnits(), UnitsSettings::VerticalDistanceUnitsMeters);
        configuration = receiverConfiguration(gpsReceiverManufacturerForType(family));
        if (base) {
            configuration.base = *base;
        } else if (!gpsReceiverCapabilities(family).surveyIn) {
            configuration.base = averagingBase(std::chrono::seconds{60});
        }
        harness.receiver.facts()->setLiveUpdates(true);
    }

    /// Connects the base and creates its status; false on failure, with the reason in the engine's lastError().
    [[nodiscard]] bool create()
    {
        if (!connectOverTcp(harness.receiver, configuration)) {
            return false;
        }
        harness.workers.current()->ready();
        status = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                               {{QStringLiteral("receiver"), QVariant::fromValue(&harness.receiver)},
                                {QStringLiteral("facts"), QVariant::fromValue(harness.receiver.facts())}});
        return status != nullptr;
    }

    QObject* find(const char* name) const { return status->findChild<QObject*>(QString::fromLatin1(name)); }

    /// What a status row shows as its value: the text of a row that is shown, else an empty one.
    QString value(const char* name) const
    {
        const QObject* const row = find(name);
        if (!row || !row->property("visible").toBool()) {
            return {};
        }
        const QObject* const factLabel = row->property("factLabel").value<QObject*>();
        return (factLabel ? factLabel->property("text") : row->property("labelText")).toString().trimmed();
    }

    void report(const GPSBaseStatus& baseStatus) const { harness.workers.current()->baseStatus(baseStatus); }

    TestFixtures::SettingsFixture units;
    GPSReceiver::Configuration configuration;
    ScriptedGPSReceiver harness;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> status;
};

QStringList siteNames(const RTKSettings* settings)
{
    QStringList names;
    for (const RTKBaseSite& site : settings->baseSites()) {
        names.append(site.name);
    }
    return names;
}

/// Saves sites until the list is full; false when one is refused.
[[nodiscard]] bool fillBaseSites(RTKSettings* settings)
{
    for (qsizetype i = settings->baseSites().size(); i < RTKSettings::MaxBaseSites; ++i) {
        if (!settings->saveBaseSite({.name = QStringLiteral("Site %1").arg(i), .latitude = 1., .longitude = 2.})) {
            return false;
        }
    }
    return true;
}

/// Enters @a text in the text field of the labelled field @a field as typing it and leaving the field does; false
/// for a field that is hidden, cannot be edited or has no single text field.
[[nodiscard]] bool enterText(QQuickItem* field, const QString& text)
{
    const QList<QQuickItem*> inputs =
        TestFixtures::findItems(field, [](QQuickItem* item) { return item->inherits("QQuickTextInput"); });
    if (inputs.size() != 1) {
        return false;
    }
    QQuickItem* const input = inputs.constFirst();
    return input->isVisible() && input->isEnabled() && input->setProperty("text", text) &&
           QMetaObject::invokeMethod(input, "editingFinished");
}
}  // namespace

void GPSReceiverSettingsTest::_surveySaveWorkflow()
{
    // How each family maps the saved base settings is GPSSettingsBindingsTest's; this is the panel's workflow.
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* connect = panel.find("rtkConnectButton");
    auto* fixed = panel.find("rtkFixedMode");
    auto* manufacturerControl = panel.find("rtkManufacturer");
    QVERIFY(connect && fixed && manufacturerControl);
    ScriptedReceiverWorkerFactory& workers = panel.receiver.workers;
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(panel.receiver.gps.hasReceiver());
    QCOMPARE(workers.count(), 1);
    QVERIFY(!fixed->isEnabled());
    QVERIFY(!manufacturerControl->isEnabled());

    // The application's receiver completes a survey; GPSManager keeps its position for a fixed-position start.
    RTKSettings* const settings = panel.settings.settings;
    AppReceiver base;
    QVERIFY(base.connect());
    base.survey(0.75);
    QVERIFY(GPSManager::instance()->saveCurrentBasePosition());
    QCOMPARE(settings->fixedBasePositionLatitude()->rawValue().toDouble(), kSurveyedPosition.latitudeDegrees);
    QCOMPARE(settings->fixedBasePositionLongitude()->rawValue().toDouble(), kSurveyedPosition.longitudeDegrees);
    QCOMPARE(settings->fixedBasePositionAltitude()->rawValue().toFloat(), kSurveyedPosition.altitudeMeters);
    QCOMPARE(settings->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.75);
    QCOMPARE(settings->baseMode()->rawValue().toInt(), 0);

    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(!panel.receiver.gps.hasReceiver());
    workers.finishRetired();
    QVERIFY(fixed->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(fixed, "click"));
    QCOMPARE(settings->baseMode()->rawValue().toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(workers.count(), 2);
    // The saved position starts the fixed base.
    const GPSBaseStationConfig::Mode savedPosition =
        GPSBaseStationConfig::Fixed{.position = kSurveyedPosition, .accuracyMeters = 0.75f};
    QVERIFY(workers.current()->capturedConfig().base.mode == savedPosition);
}

void GPSReceiverSettingsTest::_surveyCompletePrompt()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    AppReceiver base;
    QVERIFY(base.connect());
    QCOMPARE(GPSManager::instance()->receiver()->activeBaseMode(), int(BaseModeDefinition::Mode::BaseSurveyIn));
    TestFixtures::QmlEngine engine;
    // The status shows the application's receiver, whose position GPSManager saves.
    auto status = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")));
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* prompt = status->findChild<QObject*>(QStringLiteral("rtkSurveyCompletePrompt"));
    auto* save = status->findChild<QObject*>(QStringLiteral("rtkSurveySaveButton"));
    auto* accuracy = status->findChild<QObject*>(QStringLiteral("rtkSurveyAccuracy"));
    QVERIFY(prompt && save && accuracy);
    // Still surveying: nothing to keep yet, and the accuracy is the survey's so far.
    base.survey(0.75, true);
    QVERIFY(!prompt->property("visible").toBool());
    QCOMPARE(accuracy->property("label").toString(),
             QCoreApplication::translate("GPSReceiverSurveyProgress", "Current Accuracy"));

    // A position without a known accuracy cannot start a fixed base.
    base.survey(std::nullopt);
    QVERIFY(!prompt->property("visible").toBool());
    QVERIFY(!GPSManager::instance()->saveCurrentBasePosition());
    QCOMPARE(settings.settings->fixedBasePositionLatitude()->rawValue().toDouble(), 0.0);
    QCOMPARE(settings.settings->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.0);

    base.survey(0.75);
    QVERIFY(prompt->property("visible").toBool());
    QVERIFY(save->property("visible").toBool());
    QCOMPARE(accuracy->property("label").toString(),
             QCoreApplication::translate("GPSReceiverSurveyProgress", "Accuracy"));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QCOMPARE(settings.settings->fixedBasePositionLatitude()->rawValue().toDouble(), kSurveyedPosition.latitudeDegrees);
    QCOMPARE(settings.settings->fixedBasePositionAccuracy()->rawValue().toDouble(), 0.75);
    QVERIFY(!save->property("visible").toBool());
    QVERIFY(prompt->property("text").toString().contains(QStringLiteral("saved")));
    // Without a site name only the fixed base position is kept.
    QVERIFY(settings.settings->baseSites().isEmpty());

    // Another view of the receiver knows the position is saved; a new survey offers saving its result again.
    auto other = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")));
    QVERIFY2(other, qPrintable(engine.lastError()));
    auto* otherSave = other->findChild<QObject*>(QStringLiteral("rtkSurveySaveButton"));
    QVERIFY(otherSave);
    QVERIFY(!otherSave->property("visible").toBool());
    base.survey(0.75, true);
    base.survey(0.75);
    QVERIFY(save->property("visible").toBool());
    QVERIFY(otherSave->property("visible").toBool());
}

void GPSReceiverSettingsTest::_surveyProgress_data()
{
    QTest::addColumn<GPSType>("family");
    // What the receiver reports of a running survey besides its state.
    QTest::addColumn<bool>("reportsTime");
    QTest::addColumn<bool>("reportsAccuracy");
    // What the status shows of it: the label and value of the time row and whether it has a bar, then the value of
    // the accuracy row and whether that has one. An empty label or value is a row that is not shown.
    QTest::addColumn<QString>("timeLabel");
    QTest::addColumn<QString>("time");
    QTest::addColumn<bool>("timeBar");
    QTest::addColumn<QString>("accuracy");
    QTest::addColumn<bool>("accuracyBar");
    const auto row = [](GPSType family, bool reportsTime, bool reportsAccuracy, const char* timeLabel, const char* time,
                        bool timeBar, const char* accuracy, bool accuracyBar) {
        QTest::newRow(qPrintable(gpsReceiverName(family)))
            << family << reportsTime << reportsAccuracy << QString::fromLatin1(timeLabel) << QString::fromLatin1(time)
            << timeBar << QString::fromLatin1(accuracy) << accuracyBar;
    };
    // A u-blox receiver reports the survey's time and accuracy, which have to reach the configured minimum and limit.
    row(GPSType::UBlox, true, true, "Duration", "95 of 180 s", true, "2.50 of 2.00 m", true);
    // A Quectel receiver reports the observations it accepted, of those configured. Its limit filters each
    // observation, so the accuracy of the survey has none to reach.
    row(GPSType::Quectel, true, true, "Accepted observation time", "95 of 180 s", true, "2.50 m", false);
    // A Trimble receiver averages for the configured time and reports only when it is done: the time is the
    // connection's own count, which gets no bar.
    row(GPSType::Trimble, true, false, "Averaging", "95 of 180 s", false, "", false);
    // Septentrio and Femtomes receivers average for as long as they decide to.
    row(GPSType::Septentrio, true, false, "Averaging", "95 s", false, "", false);
    row(GPSType::Femto, true, false, "Averaging", "95 s", false, "", false);
    // A Unicore receiver averages without any report of how far it is: only the status line says that it does.
    row(GPSType::Unicore, false, false, "", "", false, "", false);
}

void GPSReceiverSettingsTest::_surveyProgress()
{
    QFETCH(GPSType, family);
    QFETCH(bool, reportsTime);
    QFETCH(bool, reportsAccuracy);
    QFETCH(QString, timeLabel);
    QFETCH(QString, time);
    QFETCH(bool, timeBar);
    QFETCH(QString, accuracy);
    QFETCH(bool, accuracyBar);
    BaseStatus base(family);
    QVERIFY2(base.create(), qPrintable(base.engine.lastError()));
    auto* timeRow = base.find("rtkSurveyDuration");
    auto* timeProgress = base.find("rtkSurveyDurationProgress");
    auto* accuracyRow = base.find("rtkSurveyAccuracy");
    auto* accuracyProgress = base.find("rtkSurveyAccuracyProgress");
    auto* panel = qobject_cast<QQuickItem*>(base.status.get());
    QVERIFY(panel && timeRow && timeProgress && accuracyRow && accuracyProgress);
    const qreal width = panel->implicitWidth();
    const auto translated = [](const QString& text) {
        return QCoreApplication::translate("GPSReceiverSurveyProgress", qPrintable(text));
    };
    const auto shown = [](const QObject* item) { return item->property("visible").toBool(); };

    GPSBaseStatus survey{.state = GPSBaseState::Active};
    if (reportsTime) {
        survey.duration = std::chrono::seconds{95};
    }
    if (reportsAccuracy) {
        survey.position = kSurveyedPosition;
        survey.meanAccuracyMeters = 2.5;
    }
    base.report(survey);
    QCOMPARE(base.value("rtkSurveyDuration"), time);
    if (!time.isEmpty()) {
        QCOMPARE(timeRow->property("label").toString(), translated(timeLabel));
    }
    QCOMPARE(shown(timeProgress), timeBar);
    if (timeBar) {
        QCOMPARE(timeProgress->property("value").toDouble(), 95. / 180.);
    }
    QCOMPARE(base.value("rtkSurveyAccuracy"), accuracy);
    if (!accuracy.isEmpty()) {
        QCOMPARE(accuracyRow->property("label").toString(), translated(QStringLiteral("Current Accuracy")));
    }
    QCOMPARE(shown(accuracyProgress), accuracyBar);
    // A bar is as wide as the panel is, and does not widen it.
    QCOMPARE(panel->implicitWidth(), width);
    if (accuracyBar) {
        // The bar fills as the accuracy comes down to the limit.
        QCOMPARE(accuracyProgress->property("value").toDouble(), 2. / 2.5);
        survey.meanAccuracyMeters = 1.6;
        base.report(survey);
        QCOMPARE(accuracyProgress->property("value").toDouble(), 1.);
    }

    // The finished survey keeps its time and accuracy, with nothing left to reach.
    survey.state = GPSBaseState::Valid;
    survey.position = kSurveyedPosition;
    if (reportsTime) {
        survey.duration = std::chrono::seconds{181};
    }
    if (reportsAccuracy) {
        survey.meanAccuracyMeters = 1.9;
    }
    base.report(survey);
    QCOMPARE(base.value("rtkSurveyDuration"), reportsTime ? QStringLiteral("181 s") : QString());
    QCOMPARE(base.value("rtkSurveyAccuracy"), reportsAccuracy ? QStringLiteral("1.90 m") : QString());
    if (reportsAccuracy) {
        QCOMPARE(accuracyRow->property("label").toString(), translated(QStringLiteral("Accuracy")));
    }
    QVERIFY(!shown(timeProgress) && !shown(accuracyProgress));
}

void GPSReceiverSettingsTest::_basePositionShownOnceValid_data()
{
    QTest::addColumn<GPSType>("family");
    QTest::addColumn<GPSBaseStationConfig>("base");
    QTest::addColumn<GPSEllipsoidPosition>("position");
    QTest::addColumn<QString>("height");
    const GPSBaseStationConfig survey = surveyInBase(2., std::chrono::seconds{180});
    QTest::newRow("surveyed") << GPSType::UBlox << survey << kSurveyedPosition << QStringLiteral("512.25 m");
    // A Trimble receiver reports the position it averaged without its height.
    QTest::newRow("without-height") << GPSType::Trimble << survey
                                    << GPSEllipsoidPosition{.latitudeDegrees = kSurveyedPosition.latitudeDegrees,
                                                            .longitudeDegrees = kSurveyedPosition.longitudeDegrees}
                                    << QString();
    QTest::newRow("averaged") << GPSType::Unicore << averagingBase(std::chrono::seconds{60}) << kSurveyedPosition
                              << QStringLiteral("512.25 m");
    // A fixed base shows the position it was given, once the receiver uses it.
    QTest::newRow("fixed") << GPSType::UBlox
                           << fixedBaseAt(kSurveyedPosition.latitudeDegrees, kSurveyedPosition.longitudeDegrees,
                                          kSurveyedPosition.altitudeMeters, 1)
                           << kSurveyedPosition << QStringLiteral("512.25 m");
}

void GPSReceiverSettingsTest::_basePositionShownOnceValid()
{
    QFETCH(GPSType, family);
    QFETCH(GPSBaseStationConfig, base);
    QFETCH(GPSEllipsoidPosition, position);
    QFETCH(QString, height);
    BaseStatus status(family, base);
    QVERIFY2(status.create(), qPrintable(status.engine.lastError()));
    QVERIFY(status.find("rtkBaseLatitude") && status.find("rtkBaseLongitude") && status.find("rtkBaseHeight"));
    const auto shownPosition = [&status] {
        return QStringList{status.value("rtkBaseLatitude"), status.value("rtkBaseLongitude"),
                           status.value("rtkBaseHeight")};
    };
    const QStringList none{QString(), QString(), QString()};
    QCOMPARE(shownPosition(), none);

    // What a receiver reports while it determines its position, such as the mean of a running survey, is not the
    // base position yet.
    status.report({.state = GPSBaseState::Active, .position = position});
    QCOMPARE(shownPosition(), none);
    status.report({.state = GPSBaseState::Valid, .position = position});
    QCOMPARE(shownPosition(), (QStringList{QStringLiteral("47.123456789"), QStringLiteral("8.987654321"), height}));
    // A base that lost its position shows none.
    status.report({});
    QCOMPARE(shownPosition(), none);
}

void GPSReceiverSettingsTest::_surveySaveNeedsApplicationReceiver()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    receiver.facts()->setLiveUpdates(true);
    QVERIFY(connectOverTcp(receiver));
    harness.workers.current()->ready();
    TestFixtures::QmlEngine engine;
    auto status = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                                {{QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
                                 {QStringLiteral("facts"), QVariant::fromValue(receiver.facts())}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* prompt = status->findChild<QObject*>(QStringLiteral("rtkSurveyCompletePrompt"));
    auto* save = status->findChild<QObject*>(QStringLiteral("rtkSurveySaveButton"));
    auto* accuracy = status->findChild<QObject*>(QStringLiteral("rtkSurveyAccuracy"));
    QVERIFY(prompt && save && accuracy);

    // GPSManager saves the position of the application's receiver, so a panel that shows another receiver's finished
    // survey offers no save.
    harness.workers.current()->baseStatus(
        {.state = GPSBaseState::Valid, .position = kSurveyedPosition, .meanAccuracyMeters = 0.75});
    QVERIFY(receiver.facts()->canSaveCurrentBasePosition());
    QVERIFY(accuracy->property("visible").toBool());
    QVERIFY(!prompt->property("visible").toBool());
    QVERIFY(!save->property("visible").toBool());
}

void GPSReceiverSettingsTest::_surveySavedAsSite()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    RTKSettings* const rtk = settings.settings;
    settings.saved.setFactValue(rtk->savedBaseSites(), QString());
    AppReceiver base;
    QVERIFY(base.connect());
    TestFixtures::QmlEngine engine;
    auto status = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")));
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* name = status->findChild<QObject*>(QStringLiteral("rtkSurveySiteName"));
    auto* save = status->findChild<QObject*>(QStringLiteral("rtkSurveySaveButton"));
    auto* failed = status->findChild<QObject*>(QStringLiteral("rtkSurveySaveFailed"));
    QVERIFY(name && save && failed);
    QCOMPARE(name->property("maximumLength").toInt(), static_cast<int>(RTKSettings::MaxBaseSiteNameLength));
    QVERIFY(!name->property("visible").toBool());
    base.survey(0.75);
    QVERIFY(name->property("visible").toBool());
    QVERIFY(!failed->property("visible").toBool());

    // A full list cannot keep another site: nothing is saved, and the panel says why.
    QVERIFY(fillBaseSites(rtk));
    const QString site = QStringLiteral("North field");
    QVERIFY(name->setProperty("text", QStringLiteral(" North field ")));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QVERIFY(failed->property("visible").toBool());
    QVERIFY(save->property("visible").toBool());
    QVERIFY(!base.facts()->currentBasePositionSaved());
    QCOMPARE(rtk->fixedBasePositionLatitude()->rawValue().toDouble(), 0.0);
    QVERIFY(!siteNames(rtk).contains(site));

    // With room for it, the survey becomes the fixed base position and a site of that name.
    QVERIFY(rtk->removeBaseSite(QStringLiteral("Site 0")));
    QVERIFY(!failed->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QVERIFY(!save->property("visible").toBool());
    QCOMPARE(rtk->fixedBasePositionLatitude()->rawValue().toDouble(), kSurveyedPosition.latitudeDegrees);
    const RTKBaseSite surveyed{.name = site,
                               .latitude = kSurveyedPosition.latitudeDegrees,
                               .longitude = kSurveyedPosition.longitudeDegrees,
                               .altitude = kSurveyedPosition.altitudeMeters,
                               .accuracy = 0.75};
    QCOMPARE(rtk->baseSites().last(), surveyed);
}

void GPSReceiverSettingsTest::_baseSitePicker()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    RTKSettings* const settings = panel.settings.settings;
    TestFixtures::SettingsFixture& saved = panel.settings.saved;
    saved.setFactValue(settings->savedBaseSites(), QString());
    saved.setFactValue(settings->baseMode(), static_cast<int>(BaseModeDefinition::Mode::BaseFixed));
    saved.setFactValue(settings->fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 512.5);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 0.75);
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* picker = panel.find("rtkBaseSitePicker");
    auto* use = panel.find("rtkBaseSiteUse");
    auto* remove = panel.find("rtkBaseSiteDelete");
    auto* name = panel.find("rtkBaseSiteName");
    auto* save = panel.find("rtkBaseSiteSave");
    auto* failed = panel.find("rtkBaseSiteSaveFailed");
    QVERIFY(picker && use && remove && name && save && failed);
    // Without a saved site there is nothing to pick, and a site needs a name, which the settings limit.
    QVERIFY(!picker->isVisible());
    QVERIFY(name->isVisible());
    QVERIFY(!save->isEnabled());
    QCOMPARE(name->property("maximumLength").toInt(), static_cast<int>(RTKSettings::MaxBaseSiteNameLength));

    const QString north = QStringLiteral("North field");
    const QString south = QStringLiteral("South field");
    QVERIFY(name->setProperty("text", north));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    settings->fixedBasePositionLatitude()->setRawValue(-33.5);
    QVERIFY(name->setProperty("text", south));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QCOMPARE(siteNames(settings), (QStringList{north, south}));
    // The picker shows the site just saved, not the first one a new list selects.
    QVERIFY(picker->isVisible());
    QCOMPARE(picker->property("currentText").toString(), south);

    // Picking a site changes nothing until it is used, which makes it the fixed base position the receiver starts from.
    QVERIFY(picker->setProperty("currentIndex", 0));
    QCOMPARE(settings->fixedBasePositionLatitude()->rawValue().toDouble(), -33.5);
    QVERIFY(QMetaObject::invokeMethod(use, "clicked"));
    QCOMPARE(settings->fixedBasePositionLatitude()->rawValue().toDouble(), 47.5);
    QVERIFY(panel.receiver.gps.connectReceiver());
    const GPSBaseStationConfig::Mode northPosition = GPSBaseStationConfig::Fixed{
        .position = {.latitudeDegrees = 47.5, .longitudeDegrees = 8.25, .altitudeMeters = 512.5},
        .accuracyMeters = 0.75f};
    QVERIFY(panel.receiver.worker()->capturedConfig().base.mode == northPosition);
    // A connected receiver keeps its position, which can still be saved as a site.
    QVERIFY(!use->isEnabled());
    QVERIFY(save->isEnabled() && remove->isEnabled());
    panel.receiver.gps.disconnectReceiver();

    // A full list refuses another name and says so until there is room.
    QVERIFY(fillBaseSites(settings));
    QVERIFY(name->setProperty("text", QStringLiteral("One too many")));
    QVERIFY(QMetaObject::invokeMethod(save, "clicked"));
    QVERIFY(failed->isVisible());
    QCOMPARE(settings->baseSites().size(), RTKSettings::MaxBaseSites);
    QVERIFY(picker->setProperty("currentIndex", 1));
    QVERIFY(QMetaObject::invokeMethod(remove, "clicked"));
    QVERIFY(!failed->isVisible());
    QVERIFY(!siteNames(settings).contains(south));
    QCOMPARE(siteNames(settings).first(), north);
}

void GPSReceiverSettingsTest::_fixedPositionAsEcef()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    RTKSettings* const settings = panel.settings.settings;
    TestFixtures::SettingsFixture& saved = panel.settings.saved;
    saved.setFactValue(settings->baseMode(), static_cast<int>(BaseModeDefinition::Mode::BaseFixed));
    saved.setFactValue(settings->fixedBasePositionLatitude(), 45);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 45);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 0);
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* format = panel.find("rtkFixedBaseFormat");
    auto* latitude = panel.find("rtkFixedBaseLatitude");
    auto* longitude = panel.find("rtkFixedBaseLongitude");
    auto* altitude = panel.find("rtkFixedBaseAltitude");
    auto* x = panel.find("rtkFixedBaseEcefX");
    auto* y = panel.find("rtkFixedBaseEcefY");
    auto* z = panel.find("rtkFixedBaseEcefZ");
    auto* unsaved = panel.find("rtkFixedBaseEcefUnsaved");
    QVERIFY(format && latitude && longitude && altitude && x && y && z && unsaved);
    // The form shows the one saved position either way, as the format setting says.
    QVERIFY(format->isVisible());
    QVERIFY(latitude->isVisible() && longitude->isVisible() && altitude->isVisible());
    QVERIFY(!x->isVisible() && !y->isVisible() && !z->isVisible());
    QVERIFY(QMetaObject::invokeMethod(format, "activated", Q_ARG(int, 1)));
    QCOMPARE(settings->fixedBasePositionFormat()->rawValue().toInt(), int(RTKSettings::Ecef));
    QVERIFY(!latitude->isVisible() && !longitude->isVisible() && !altitude->isVisible());
    QVERIFY(x->isVisible() && y->isVisible() && z->isVisible());
    QVERIFY(!unsaved->isVisible());
    const auto shown = [](QQuickItem* field) {
        const auto inputs =
            TestFixtures::findItems(field, [](QQuickItem* item) { return item->inherits("QQuickTextInput"); });
        return inputs.size() == 1 ? inputs.constFirst()->property("text").toString() : QString();
    };
    QCOMPARE(shown(x), QStringLiteral("3194419.1451"));
    QCOMPARE(shown(z), QStringLiteral("4487348.4089"));

    // 46 degrees north, 7 degrees east and 1500 m. The first coordinate alone is no position, and says so.
    QVERIFY(enterText(x, QStringLiteral("441022.1434")));
    QVERIFY(unsaved->isVisible());
    QCOMPARE(settings->fixedBasePositionLatitude()->rawValue().toDouble(), 45.);
    QVERIFY(enterText(x, QStringLiteral("4406271.7614")));
    QVERIFY(enterText(y, QStringLiteral("541022.1434")));
    QVERIFY(enterText(z, QStringLiteral("4566326.5505")));
    QVERIFY(!unsaved->isVisible());
    QCOMPARE(shown(x), QStringLiteral("4406271.7614"));
    QVERIFY(panel.receiver.gps.connectReceiver());
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&panel.receiver.worker()->capturedConfig().base.mode);
    QVERIFY(fixed);
    QVERIFY(std::abs(fixed->position.latitudeDegrees - 46) < 1e-9);
    QVERIFY(std::abs(fixed->position.longitudeDegrees - 7) < 1e-9);
    QVERIFY(std::abs(fixed->position.altitudeMeters - 1500) < 1e-4);
    // A connected receiver keeps its position.
    QVERIFY(!enterText(x, QStringLiteral("4406272.7614")));
    panel.receiver.gps.disconnectReceiver();

    // The other format shows what was entered, and a site that is used shows in this one.
    settings->fixedBasePositionFormat()->setRawValue(RTKSettings::Geodetic);
    QVERIFY(latitude->isVisible() && !x->isVisible());
    QCOMPARE(shown(latitude), QStringLiteral("46.000000000"));
    settings->fixedBasePositionFormat()->setRawValue(RTKSettings::Ecef);
    QVERIFY(settings->saveBaseSite({.name = QStringLiteral("Equator"), .latitude = 0., .longitude = 90.}));
    QVERIFY(settings->useBaseSite(QStringLiteral("Equator")));
    QCOMPARE(shown(y), QStringLiteral("6378137.0000"));
}

void GPSReceiverSettingsTest::_statusListsConstellations()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    // The panel appears with the receiver, which an item without a parent cannot do.
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    auto status = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                                {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                                 {QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
                                 {QStringLiteral("facts"), QVariant::fromValue(receiver.facts())}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* item = qobject_cast<QQuickItem*>(status.get());
    auto* row = status->findChild<QObject*>(QStringLiteral("rtkConstellations"));
    QVERIFY(item && row);
    QVERIFY(connectOverTcp(receiver));
    ScriptedReceiverWorker* const worker = harness.workers.current();
    worker->ready();
    // A receiver that reports no satellites per constellation has no row.
    QVERIFY(!row->property("visible").toBool());

    GPSSatelliteReport report;
    report.timestampUs = 1;
    report.inView = 17;
    report.addConstellation({GPSConstellation::GPS, 10, 8});
    report.addConstellation({GPSConstellation::Galileo, 7, std::nullopt});
    worker->satellites(report);
    QVERIFY(row->property("visible").toBool());
    // Used/in view, with a dash for a used count the receiver does not report.
    QCOMPARE(row->property("labelText").toString(), QStringLiteral("GPS\u00A08/10, Galileo\u00A0–/7"));
    const qreal width = item->implicitWidth();

    // More constellations wrap inside the row instead of widening the panel.
    for (const GPSConstellation constellation :
         {GPSConstellation::BeiDou, GPSConstellation::QZSS, GPSConstellation::SBAS, GPSConstellation::NavIC}) {
        report.addConstellation({constellation, 12, 11});
    }
    worker->satellites(report);
    QVERIFY(row->property("labelText").toString().endsWith(QStringLiteral("NavIC\u00A011/12")));
    QCOMPARE(item->implicitWidth(), width);

    receiver.disconnectReceiver();
    QVERIFY(!row->property("visible").toBool());
}

void GPSReceiverSettingsTest::_connectionRowFollowsReceiver()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* connect = panel.find("rtkConnectButton");
    auto* device = panel.find("rtkSerialDevice");
    auto* row = panel.find<QObject>("rtkReceiverStatus");
    auto* problem = panel.find<QObject>("rtkErrorMessage");
    QVERIFY(connect && device && row && problem);
    GPSReceiver& receiver = panel.receiver.gps;
    const QGCPalette palette;
    // The row's dot, line and button, and the receiver's problem below it.
    const auto verifyRow = [&](const QColor& color, const QString& text, const char* button) {
        QCOMPARE(row->property("statusColor").value<QColor>(), color);
        QCOMPARE(row->property("statusText").toString(), text);
        QCOMPARE(connect->property("text").toString(), QCoreApplication::translate("GPSReceiverConnectionRow", button));
        QVERIFY(connect->isEnabled());
        QCOMPARE(problem->property("text").toString(), receiver.errorMessage());
        QCOMPARE(problem->property("visible").toBool(), !receiver.errorMessage().isEmpty());
    };
    const QString disconnected = row->property("statusText").toString();
    QVERIFY(!disconnected.isEmpty());
    verifyRow(palette.colorGrey(), disconnected, "Connect");

    // Only Connect automatically reconnects a lost receiver.
    panel.settings.autoConnect->setRawValue(true);
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    // A connection being opened can be stopped.
    QCOMPARE(receiver.state(), GPSReceiver::State::Connecting);
    verifyRow(palette.colorOrange(), receiver.statusText(), "Disconnect");
    panel.receiver.worker()->ready();
    verifyRow(palette.colorGreen(), receiver.statusText(), "Disconnect");
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    panel.receiver.worker()->fail(GPSConnectionError::DeviceError);
    verifyExpectedLogMessage();
    // The row says what the receiver is doing and offers to stop the reconnecting, with the reason below it, while
    // the form stays locked.
    QCOMPARE(receiver.state(), GPSReceiver::State::Reconnecting);
    QVERIFY(!receiver.errorMessage().isEmpty());
    verifyRow(palette.colorOrange(), receiver.statusText(), "Cancel reconnect");
    QVERIFY(!device->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QCOMPARE(receiver.state(), GPSReceiver::State::Disconnected);
    QVERIFY(receiver.errorMessage().isEmpty());
    verifyRow(palette.colorGrey(), disconnected, "Connect");
    QVERIFY(device->isEnabled());
    QCOMPARE(panel.receiver.workers.count(), 1);
    panel.receiver.workers.finishRetired();

    // A connection that fails leaves its reason below the row, which offers to connect again.
    panel.settings.autoConnect->setRawValue(false);
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("Failed to open")));
    panel.receiver.worker()->fail(GPSConnectionError::OpenFailed);
    verifyExpectedLogMessage();
    QCOMPARE(receiver.state(), GPSReceiver::State::Disconnected);
    QVERIFY(!receiver.errorMessage().isEmpty());
    verifyRow(palette.colorRed(), disconnected, "Connect");
}

void GPSReceiverSettingsTest::_tcpConnectionFields_data()
{
    QTest::addColumn<bool>("serialLinks");
    QTest::newRow("build") << true;
    // The form of a build without serial links, on any build.
    QTest::newRow("no-serial-links") << false;
}

void GPSReceiverSettingsTest::_tcpConnectionFields()
{
    QFETCH(bool, serialLinks);
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox), serialLinks);
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* host = panel.find("rtkTcpHost");
    auto* port = panel.find("rtkTcpPort");
    auto* device = panel.find("rtkSerialDevice");
    auto* connectionType = panel.find("rtkConnectionType");
    auto* tcpOnly = panel.find("rtkTcpOnly");
    auto* consent = panel.find<QObject>("rtkPersistentChangesCheckBox");
    QVERIFY(host && port && device && connectionType && tcpOnly && consent);
    RTKSettings* const settings = panel.settings.settings;
    GPSReceiver& receiver = panel.receiver.gps;
    QVERIFY(connectionType->isVisible());
    const bool serial = receiver.serialSupported();
    QVERIFY(serialLinks || !serial);
    // Without serial links the saved serial selection shows the TCP address it connects, and says why.
    QCOMPARE(settings->connectionType()->rawValue().toInt(), int(RTKSettings::Serial));
    QCOMPARE(device->isVisible(), serial);
    QCOMPARE(host->isVisible() && port->isVisible(), !serial);
    QCOMPARE(tcpOnly->isVisible(), !serial);
    settings->connectionType()->setRawValue(RTKSettings::Tcp);
    QVERIFY(host->isVisible() && port->isVisible());
    QVERIFY(!device->isVisible());
    QVERIFY(!tcpOnly->isVisible());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY(consent->property("visible").toBool());
    // Which settings withdraw the permission is GPSReceiverTest's; the checkbox follows the receiver.
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(consent->property("checked").toBool());
    settings->tcpHost()->setRawValue(QStringLiteral("bridge"));
    QVERIFY(!consent->property("checked").toBool());
    settings->tcpPort()->setRawValue(2101);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("bridge:2101"));
    QVERIFY(!host->isEnabled() && !port->isEnabled());
}

void GPSReceiverSettingsTest::_roleSelectsFields()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* role = panel.find("rtkReceiverRole");
    auto* manufacturer = panel.find("rtkManufacturer");
    auto* survey = panel.find("rtkSurveyMode");
    auto* udpPort = panel.find("rtkUdpPort");
    auto* udpExplanation = panel.find("rtkUdpExplanation");
    auto* device = panel.find("rtkSerialDevice");
    auto* autoConnect = panel.find("rtkAutoConnect");
    auto* connect = panel.find("rtkConnectButton");
    QVERIFY(role && manufacturer && survey && udpPort && udpExplanation && device && autoConnect && connect);
    RTKSettings* const settings = panel.settings.settings;
    GPSReceiver& receiver = panel.receiver.gps;
    QVERIFY(role->isVisible() && manufacturer->isVisible() && survey->isVisible());
    QVERIFY(autoConnect->isVisible());
    QVERIFY(connect->isEnabled());

    // A receiver QGroundControl does not configure hides every base setting.
    settings->receiverRole()->setRawValue(RTKSettings::Passive);
    settings->forwardReceiverRtcm()->setRawValue(false);
    QVERIFY(!manufacturer->isVisible());
    QVERIFY(!survey->isVisible());
    QCOMPARE(device->isVisible(), receiver.serialSupported());
    QVERIFY(autoConnect->isVisible());
    QVERIFY(connect->isEnabled());

    settings->connectionType()->setRawValue(RTKSettings::Udp);
    QVERIFY(udpPort->isVisible() && udpExplanation->isVisible());
    QVERIFY(!device->isVisible());
    QVERIFY(autoConnect->isVisible());
    QVERIFY(connect->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QVERIFY(receiver.hasReceiver());
    QCOMPARE(panel.receiver.worker()->type(), GPSType::Passive);
    QCOMPARE(receiver.activeRole(), RTKSettings::Passive);
    QVERIFY(!receiver.forwardingCorrections());
    receiver.disconnectReceiver();

    // UDP cannot carry base configuration.
    settings->receiverRole()->setRawValue(RTKSettings::ConfiguredBase);
    QVERIFY(udpPort->isVisible());
    QVERIFY(!connect->isEnabled());
    QVERIFY(udpExplanation->property("text").toString().contains(QStringLiteral("serial or TCP")));
    settings->connectionType()->setRawValue(RTKSettings::Serial);
    QVERIFY(!udpPort->isVisible());
    QVERIFY(connect->isEnabled());
}

void GPSReceiverSettingsTest::_automaticManufacturer()
{
    Panel panel(GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* manufacturer = panel.find<QObject>("rtkManufacturer");
    auto* explanation = panel.find("rtkAutomaticExplanation");
    auto* connect = panel.find<QObject>("rtkConnectButton");
    QVERIFY(manufacturer && explanation && connect);
    RTKSettings* const settings = panel.settings.settings;
    GPSReceiver& receiver = panel.receiver.gps;
    QCOMPARE(manufacturer->property("currentText").toString(), QStringLiteral("Detect automatically"));
    QVERIFY(explanation->isVisible());
    QVERIFY(explanation->property("text").toString().contains(QStringLiteral("identification queries")));
    QVERIFY(connect->property("enabled").toBool());
    // Until a receiver is identified, the base modes of every family are offered.
    settings->baseMode()->setRawValue(2);
    QVERIFY(connect->property("enabled").toBool());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY(!explanation->isVisible());
    QVERIFY(!connect->property("enabled").toBool());
    settings->baseReceiverManufacturer()->setRawValue(GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY(QMetaObject::invokeMethod(connect, "clicked"));
    QVERIFY(receiver.hasReceiver());
    QCOMPARE(panel.receiver.worker()->type(), GPSType::Automatic);

    receiver.facts()->setLiveUpdates(true);
    auto* text = panel.find<QObject>("rtkReceiverStatus");
    auto* detected = panel.find("rtkDetectedReceiver");
    QVERIFY(text && detected);
    QCOMPARE(text->property("statusText").toString(), QStringLiteral("Identifying receiver..."));
    QVERIFY(!detected->isVisible());
    // The status line follows what the detection tries.
    panel.receiver.worker()->looksFor(GPSType::Automatic, 38400);
    QCOMPARE(text->property("statusText").toString(), panel.receiver.gps.statusText());
    QVERIFY(text->property("statusText").toString().contains(QStringLiteral("38400")) ==
            panel.receiver.gps.serialSupported());
    panel.receiver.worker()->detected(GPSType::Unicore);
    QVERIFY(detected->isVisible());
    QCOMPARE(detected->property("labelText").toString(), QStringLiteral("Unicore"));
    QCOMPARE(text->property("statusText").toString(), QStringLiteral("Connecting to receiver..."));
    panel.receiver.worker()->ready();
    // The status names the base mode once the receiver reports that it determines its position.
    panel.receiver.worker()->baseStatus({.state = GPSBaseState::Active});
    QVERIFY(text->property("statusText").toString().contains(QStringLiteral("averaging")));
    QVERIFY(detected->isVisible());
    receiver.disconnectReceiver();
    QVERIFY(!detected->isVisible());
}

void GPSReceiverSettingsTest::_automaticOffersQuectelConsent()
{
    Panel panel(GPS_AUTOMATIC_MANUFACTURER);
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* consent = panel.find("rtkPersistentChangesCheckBox");
    auto* consentWarning = panel.find("rtkPersistentConsentWarning");
    auto* restartNote = panel.find("rtkPersistentConfigurationWarning");
    auto* surveyNote = panel.find("rtkSurveySavesPosition");
    auto* connect = panel.find<QObject>("rtkConnectButton");
    QVERIFY(consent && consentWarning && restartNote && surveyNote && connect);
    // A factory LG290P needs this consent, so Automatic offers it and words it for an identified Quectel receiver.
    for (auto* item : {consent, consentWarning, restartNote, surveyNote}) {
        QVERIFY2(item->isVisible(), qPrintable(item->objectName()));
        QVERIFY2(item->property("text").toString().contains(QStringLiteral("if a Quectel receiver is identified"),
                                                            Qt::CaseInsensitive),
                 qPrintable(item->property("text").toString()));
    }
    QVERIFY(consent->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(panel.receiver.workers.count(), 1);
    QCOMPARE(panel.receiver.worker()->type(), GPSType::Automatic);
    QVERIFY(panel.receiver.worker()->capturedConfig().allowPersistentChanges);
    QVERIFY(!consent->property("checked").toBool());
    panel.receiver.gps.disconnectReceiver();

    RTKSettings* const settings = panel.settings.settings;
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY(!consent->isVisible());
    QVERIFY(!restartNote->isVisible());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY(consent->isVisible());
    QCOMPARE(consent->property("text").toString(), QStringLiteral("Allow flash save and restart"));
}

void GPSReceiverSettingsTest::_compactCorrectionsToggle()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* toggle = panel.find("rtkCompactRtcm");
    QVERIFY(toggle);
    RTKSettings* const settings = panel.settings.settings;
    QVERIFY(toggle->isVisible());
    QVERIFY(toggle->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(settings->compactRtcmCorrections()->rawValue().toBool());
    // Which families send compact corrections is the receiver presentation's; the toggle follows it.
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Septentrio));
    QVERIFY(!toggle->isVisible());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY(toggle->isVisible());
    QVERIFY(panel.receiver.gps.connectReceiver());
    QVERIFY(panel.receiver.worker()->capturedConfig().base.compactObservations);
    QVERIFY(!toggle->isEnabled());
}

void GPSReceiverSettingsTest::_recordingSwitch()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* toggle = panel.find("rtkRecordReceiverData");
    auto* file = panel.find("rtkRecordingFile");
    QVERIFY(toggle && file);
    GPSReceiver& receiver = panel.receiver.gps;
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(!file->isVisible());

    // The session that already runs is not recorded, and the note says when recording starts.
    QVERIFY(receiver.connectReceiver());
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(receiver.recordSessions());
    QVERIFY(file->isVisible());
    QCOMPARE(file->property("text").toString(),
             QCoreApplication::translate("GPSReceiverRecording", "Recording starts with the next connection."));

    receiver.disconnectReceiver();
    // The retired worker holds the serial port until it finishes.
    panel.receiver.workers.finishRetired();
    QVERIFY(receiver.connectReceiver());
    const QString recording = receiver.recordingFile();
    QVERIFY(recording.startsWith(SettingsReceiver::recordingDirectory()));
    QCOMPARE(file->property("text").toString(),
             QCoreApplication::translate("GPSReceiverRecording", "Recording file: %1").arg(recording));

    // The switch follows the receiver, and the session that started recorded stays so until it ends.
    receiver.setRecordSessions(false);
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(file->property("text").toString().endsWith(recording));
    receiver.disconnectReceiver();
    QVERIFY(!file->isVisible());
}

void GPSReceiverSettingsTest::_consentIsOneUse()
{
    Panel panel(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* consent = panel.find<QObject>("rtkPersistentChangesCheckBox");
    auto* connect = panel.find<QObject>("rtkConnectButton");
    QVERIFY(consent && connect);
    SettingsReceiver& receiver = panel.receiver;
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(receiver.workers.count(), 1);
    QVERIFY(receiver.worker()->capturedConfig().allowPersistentChanges);
    QVERIFY(!consent->property("checked").toBool());
    const auto failToOpen = [this, &receiver]() {
        expectLogMessage("GPS.Receiver.GPSReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Failed to open")));
        receiver.worker()->fail(GPSConnectionError::OpenFailed);
        verifyExpectedLogMessage();
        receiver.workers.finishRetired();
    };
    // A failed attempt uses up its consent.
    failToOpen();
    QVERIFY(!receiver.gps.hasReceiver());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(receiver.workers.count(), 2);
    QVERIFY(!receiver.worker()->capturedConfig().allowPersistentChanges);
    failToOpen();
    QVERIFY(!receiver.gps.hasReceiver());

    // Which settings withdraw the permission is GPSReceiverTest's; the checkbox follows the receiver.
    RTKSettings* const settings = panel.settings.settings;
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(consent->property("checked").toBool());
    settings->serialDevice()->setRawValue(QStringLiteral("/test/other"));
    QVERIFY(!consent->property("checked").toBool());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Unicore));
    QVERIFY(!consent->property("visible").toBool());
    settings->baseReceiverManufacturer()->setRawValue(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(receiver.gps.connectReceiver());
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(!consent->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(!receiver.gps.hasReceiver());
    QVERIFY(!consent->property("checked").toBool());
    // Leaving the panel withdraws a permission given for the configuration it showed.
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(receiver.gps.persistentChangesAllowed());
    panel.item.reset();
    QVERIFY(!receiver.gps.persistentChangesAllowed());
}

void GPSReceiverSettingsTest::_warningWidth_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<bool>("longText");
    for (const int width : {320, 1200}) {
        QTest::addRow("width-%d", width) << width << false;
        QTest::addRow("translated-width-%d", width) << width << true;
    }
}

void GPSReceiverSettingsTest::_warningWidth()
{
    QFETCH(int, width);
    QFETCH(bool, longText);
    TestFixtures::QmlWindowFixture window(width, 800);
    Panel panel(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* item = qobject_cast<QQuickItem*>(panel.item.get());
    QVERIFY(item);
    item->setParentItem(window.contentItem());
    item->setWidth(width);
    QVERIFY(window.show());
    for (const auto* name : {"rtkPersistentConfigurationWarning", "rtkPersistentConsentWarning"}) {
        auto* warning = panel.find(name);
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

void GPSReceiverSettingsTest::_serialSelectionTracksFacts()
{
#ifdef QGC_NO_SERIAL_LINK
    QSKIP("Serial selection requires serial link support");
#endif
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* device = panel.find<QObject>("rtkSerialDevice");
    auto* baud = panel.find<QObject>("rtkSerialBaudRate");
    auto* custom = panel.find<QObject>("rtkCustomBaudRate");
    QVERIFY(device && baud && custom);
    RTKSettings* const settings = panel.settings.settings;
    GPSReceiver& receiver = panel.receiver.gps;
    // The picker shows the labels of the receiver's serial port entries, which GPSReceiverTest::_serialPortEntries
    // covers, and stores their paths.
    QCOMPARE(device->property("currentText").toString(), QStringLiteral("/test/receiver"));
    QCOMPARE(baud->property("currentText").toString(), QStringLiteral("115200"));
    settings->serialDevice()->setRawValue(QStringLiteral("/test/missing"));
    QVERIFY(device->property("currentText").toString().contains(QStringLiteral("/test/missing")));
    QVERIFY(device->property("currentText").toString().contains(QStringLiteral("unavailable")));
    const QString label = QStringLiteral("Receiver – /test/missing");
    QVERIFY(panel.item->setProperty("serialPorts", portEntries(QStringLiteral("/test/missing"), label)));
    QCOMPARE(device->property("currentText").toString(), label);
    // With Connect automatically on, a configured base can leave the device to USB discovery.
    const QString anyReceiver = QCoreApplication::translate("GPSReceiverConnection", "Any RTK receiver on USB");
    panel.settings.autoConnect->setRawValue(true);
    QVERIFY(QMetaObject::invokeMethod(device, "activated", Q_ARG(int, 0)));
    QVERIFY(settings->serialDevice()->rawValue().toString().isEmpty());
    QCOMPARE(device->property("currentText").toString(), anyReceiver);
    panel.settings.autoConnect->setRawValue(false);
    QVERIFY(QMetaObject::invokeMethod(device, "activated", Q_ARG(int, 0)));
    QCOMPARE(settings->serialDevice()->rawValue().toString(), QStringLiteral("/test/missing"));
    settings->serialBaudRate()->setRawValue(123457);
    QVERIFY(custom->property("visible").toBool());
    QCOMPARE(custom->property("text").toString(), QStringLiteral("123457"));
    settings->serialBaudRate()->setRawValue(230400);
    QVERIFY(!custom->property("visible").toBool());
    QCOMPARE(baud->property("currentText").toString(), QStringLiteral("230400"));
    QVERIFY(QMetaObject::invokeMethod(baud, "activated", Q_ARG(int, 3)));
    QVERIFY(custom->property("visible").toBool());
    QVERIFY(custom->setProperty("text", QStringLiteral("250000")));
    QVERIFY(QMetaObject::invokeMethod(custom, "_onEditingFinished"));
    QCOMPARE(settings->serialBaudRate()->rawValue().toInt(), 250000);
    QVERIFY(QMetaObject::invokeMethod(baud, "activated", Q_ARG(int, 0)));
    QCOMPARE(settings->serialBaudRate()->rawValue().toInt(), 0);
    QCOMPARE(baud->property("currentText").toString(), QCoreApplication::translate("GPSReceiverSerialPort", "Auto"));
    // A passive receiver's rate is found by listening, so Auto stays selected for that role.
    settings->receiverRole()->setRawValue(RTKSettings::Passive);
    QVERIFY(!custom->property("visible").toBool());
    QCOMPARE(baud->property("currentText").toString(), QCoreApplication::translate("GPSReceiverSerialPort", "Auto"));
    settings->receiverRole()->setRawValue(RTKSettings::ConfiguredBase);
    settings->serialBaudRate()->setRawValue(230400);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(receiver.activeEndpoint(), QStringLiteral("/test/missing"));
    QCOMPARE(panel.receiver.worker()->capturedConfig().baudRate, 230400u);
    QVERIFY(!device->property("enabled").toBool());
    QVERIFY(!baud->property("enabled").toBool());
    QVERIFY(!custom->property("enabled").toBool());
}

void GPSReceiverSettingsTest::_serialSelectionSurvivesChoiceChanges()
{
#ifdef QGC_NO_SERIAL_LINK
    QSKIP("Serial selection requires serial link support");
#endif
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    auto* device = panel.find<QObject>("rtkSerialDevice");
    auto* baud = panel.find<QObject>("rtkSerialBaudRate");
    QVERIFY(device && baud);
    RTKSettings* const settings = panel.settings.settings;
    const QString receiver = QStringLiteral("/test/receiver");
    const QString other = QStringLiteral("/test/other");
    const QString plugged = QStringLiteral("/test/plugged");
    const auto selection = [](const QObject* combo) {
        return QStringLiteral("%1: %2")
            .arg(combo->property("currentIndex").toInt())
            .arg(combo->property("currentText").toString());
    };
    // A combo box selects its first entry when its choices are replaced, so neither selection is the first one.
    QVERIFY(panel.item->setProperty("serialPorts", portEntries({receiver, other})));
    settings->serialDevice()->setRawValue(other);
    settings->serialBaudRate()->setRawValue(230400);
    QCOMPARE(selection(device), QStringLiteral("1: /test/other"));
    QCOMPARE(selection(baud), QStringLiteral("2: 230400"));

    // Plugging in a port lists it; unplugging the selected one keeps it, as unavailable.
    QVERIFY(panel.item->setProperty("serialPorts", portEntries({receiver, other, plugged})));
    QCOMPARE(selection(device), QStringLiteral("1: /test/other"));
    QVERIFY(panel.item->setProperty("serialPorts", portEntries({receiver, plugged})));
    QCOMPARE(selection(device), QStringLiteral("2: (unavailable) /test/other"));
    // Connect automatically lists USB discovery above the ports.
    panel.settings.autoConnect->setRawValue(true);
    QCOMPARE(selection(device), QStringLiteral("3: (unavailable) /test/other"));

    // A receiver may list other rates.
    QVERIFY(panel.item->setProperty("serialBaudRates", QVariant::fromValue(QList<int>{57600, 115200, 230400})));
    QCOMPARE(selection(baud), QStringLiteral("3: 230400"));
    QCOMPARE(settings->serialDevice()->rawValue().toString(), other);
    QCOMPARE(settings->serialBaudRate()->rawValue().toInt(), 230400);
}

void GPSReceiverSettingsTest::_serialDeviceMarkOnNarrowPanel_data()
{
    QTest::addColumn<QString>("mark");
    // A device another connection holds, one that stays closed where only one can be open at a time, and a saved
    // device that is not listed.
    for (const auto* mark : {"(in use)", "(another is in use)", "(unavailable)"}) {
        QTest::newRow(mark) << QString::fromLatin1(mark);
    }
}

void GPSReceiverSettingsTest::_serialDeviceMarkOnNarrowPanel()
{
#ifdef QGC_NO_SERIAL_LINK
    QSKIP("Serial selection requires serial link support");
#else
    QFETCH(QString, mark);
    constexpr int width = 320;
    TestFixtures::QmlWindowFixture window(width, 800);
    Panel panel(gpsReceiverManufacturerForType(GPSType::UBlox));
    QVERIFY2(panel.create(), qPrintable(panel.engine.lastError()));
    // A device whose name and path are longer than the panel is wide.
    const QString device = QStringLiteral("/dev/serial/by-id/usb-u-blox_AG_-_www.u-blox.com_u-blox_GNSS_receiver-if00");
    const QString other = QStringLiteral("/test/other");
    SerialPortManager::Port port = serialPort(device);
    port.description = QStringLiteral("u-blox GNSS receiver");
    TestSerialPorts& serial = panel.receiver.serial;
    SerialPortManager::ReservationPtr vehicleLink;
    if (mark == QStringLiteral("(unavailable)")) {
        serial.plug({serialPort(other)});
    } else {
        serial.plug({port, serialPort(other)});
        const bool singlePort = mark == QStringLiteral("(another is in use)");
        serial.manager.setSinglePortOnly(singlePort);
        vehicleLink = serial.manager.reservePort(singlePort ? other : device);
        QVERIFY(vehicleLink);
    }
    GPSReceiver& receiver = panel.receiver.gps;
    receiver.tick();
    panel.settings.settings->serialDevice()->setRawValue(device);
    QVERIFY(panel.item->setProperty("serialPorts", QVariant::fromValue(receiver.serialPortEntries())));
    auto* item = qobject_cast<QQuickItem*>(panel.item.get());
    QVERIFY(item);
    item->setParentItem(window.contentItem());
    item->setWidth(width);
    QVERIFY(window.show());
    auto* picker = panel.find("rtkSerialDevice");
    QVERIFY(picker);
    QVERIFY2(picker->property("currentText").toString().contains(mark),
             qPrintable(picker->property("currentText").toString()));

    // The picker's button shows the selected device, and its popup lists the devices as wide as the button.
    auto* button = picker->property("contentItem").value<QQuickItem*>();
    QVERIFY(button);
    QTRY_VERIFY_WITH_TIMEOUT(button->width() > 0 && picker->width() <= width, TestTimeout::mediumMs());
    QVERIFY(button->implicitWidth() > button->width());
    QVERIFY2(showsWhole(button, mark), qPrintable(button->property("text").toString()));
    auto* popup = picker->property("popup").value<QObject*>();
    QVERIFY(popup);
    QVERIFY(QMetaObject::invokeMethod(popup, "open"));
    QTRY_VERIFY_WITH_TIMEOUT(popup->property("opened").toBool(), TestTimeout::mediumMs());
    const auto entries =
        TestFixtures::findItems(popup->property("contentItem").value<QQuickItem*>(), [&mark](QQuickItem* entry) {
            return entry->inherits("QQuickText") && entry->property("text").toString().contains(mark);
        });
    QCOMPARE(entries.size(), 1);
    QVERIFY(entries.first()->implicitWidth() > entries.first()->width());
    QVERIFY2(showsWhole(entries.first(), mark), qPrintable(entries.first()->property("text").toString()));
#endif
}

void GPSReceiverSettingsTest::_horizontalAccuracyLabel()
{
    TestFixtures::SettingsFixture saved;
    Fact* const units = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    saved.setFactValue(units, UnitsSettings::HorizontalDistanceUnitsMeters);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> panel =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GcsPositionStatus.qml")),
                      {{QStringLiteral("gcsPosition"), QVariant::fromValue(QGeoCoordinate(47.0, 8.0))},
                       {QStringLiteral("horizontalAccuracy"), 5.1}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* accuracy = panel->findChild<QObject*>(QStringLiteral("gcsHorizontalAccuracy"));
    QVERIFY(accuracy);
    QVERIFY(accuracy->property("visible").toBool());
    QCOMPARE(accuracy->property("label").toString(), QStringLiteral("Horizontal accuracy"));
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("5.1 m"));
    QVERIFY(panel->setProperty("horizontalAccuracy", 0));
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("0.0 m"));
    QVERIFY(panel->setProperty("horizontalAccuracy", 5.1));
    // The accuracy follows the units setting while it is shown.
    units->setRawValue(UnitsSettings::HorizontalDistanceUnitsFeet);
    QCOMPARE(accuracy->property("labelText").toString(), QStringLiteral("16.7 ft"));
    // An unknown accuracy is not shown.
    QVERIFY(panel->setProperty("horizontalAccuracy", qInf()));
    QVERIFY(!accuracy->property("visible").toBool());
}

void GPSReceiverSettingsTest::_settingsPageListsGroups_data()
{
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<BaseModeDefinition::Mode>("baseMode");
    QTest::addColumn<bool>("baseStation");
    QTest::addColumn<bool>("baseSites");
    const int ublox = gpsReceiverManufacturerForType(GPSType::UBlox);
    using Mode = BaseModeDefinition::Mode;
    QTest::newRow("survey-in base") << int(RTKSettings::ConfiguredBase) << ublox << Mode::BaseSurveyIn << true << false;
    QTest::newRow("fixed base") << int(RTKSettings::ConfiguredBase) << ublox << Mode::BaseFixed << true << true;
    // A passive receiver has no base settings, whatever base mode is saved.
    QTest::newRow("passive") << int(RTKSettings::Passive) << ublox << Mode::BaseFixed << false << false;
    // A manufacturer no receiver family has offers no base mode: the base station group only says that the saved one
    // is not supported.
    const int unknownManufacturer = 9999;
    QTest::newRow("unknown manufacturer")
        << int(RTKSettings::ConfiguredBase) << unknownManufacturer << Mode::BaseFixed << true << false;
}

void GPSReceiverSettingsTest::_settingsPageListsGroups()
{
    QFETCH(int, role);
    QFETCH(int, manufacturer);
    QFETCH(BaseModeDefinition::Mode, baseMode);
    QFETCH(bool, baseStation);
    QFETCH(bool, baseSites);
    // The settings page shows the groups of the form one by one, so that its sidebar and search reach each of them.
    // Its definition states when the two base groups are shown apart from the groups themselves, so the sidebar, the
    // page and the groups have to agree.
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    settings.saved.setFactValue(settings.settings->receiverRole(), role);
    settings.saved.setFactValue(settings.settings->baseReceiverManufacturer(), manufacturer);
    settings.saved.setFactValue(settings.settings->baseMode(), static_cast<int>(baseMode));
    // The page shows the application's receiver, which follows the saved settings only outside tests.
    GPSReceiver* const receiver = GPSManager::instance()->receiver();
    const GPSReceiver::Configuration unbound = receiver->configuration();
    const auto restore = qScopeGuard([receiver, unbound]() { receiver->setConfiguration(unbound); });
    GPSReceiver::Configuration selected = unbound;
    selected.receiverRole = static_cast<RTKSettings::ReceiverRole>(role);
    selected.baseReceiverManufacturer = manufacturer;
    selected.autoConnect = false;
    receiver->setConfiguration(selected);
    TestFixtures::QmlEngine engine;
    auto page = engine.create(QByteArrayLiteral(R"(
        import QtQuick
        import QGroundControl
        import QGroundControl.AppSettings

        Item {
            // The page's scroll area takes its palette from the window that shows the page.
            QGCPalette { id: qgcPal }
            GNSSReceiverSettings {}
        }
    )"));
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto sidebar = engine.create(QByteArrayLiteral(R"(
        import QGroundControl.AppSettings

        SettingsPagesModel {
            /// The sections the settings sidebar lists under the GNSS Receiver page.
            function receiverSections() {
                for (let i = 0; i < count; i++) {
                    if (get(i).nameKey === "GNSS Receiver") {
                        return get(i).sections().filter(section => section.visible).map(section => section.name)
                    }
                }
                return []
            }
        }
    )"));
    QVERIFY2(sidebar, qPrintable(engine.lastError()));
    QVariant listed;
    QVERIFY(QMetaObject::invokeMethod(sidebar.get(), "receiverSections", Q_RETURN_ARG(QVariant, listed)));
    QStringList sections{QStringLiteral("GNSS Receiver Status"), QStringLiteral("GNSS Receiver")};
    if (baseStation) {
        sections.append(QStringLiteral("RTK Base Station"));
    }
    if (baseSites) {
        sections.append(QStringLiteral("Saved Base Sites"));
    }
    sections.append(
        {QStringLiteral("Connection Options"), QStringLiteral("Position Receiver"), QStringLiteral("GCS Position")});
    QCOMPARE(listed.value<QJSValue>().toVariant().toStringList(), sections);

    // The page's group of a control: the child of the page layout that holds it.
    const auto group = [&page](const char* control) -> QQuickItem* {
        auto* item = page->findChild<QQuickItem*>(QString::fromLatin1(control));
        while (item && item->parentItem() && item->parentItem()->objectName() != QLatin1String("settingsPageLayout")) {
            item = item->parentItem();
        }
        return item && item->parentItem() ? item : nullptr;
    };
    auto* status = group("rtkConnectButton");
    auto* connection = group("rtkReceiverRole");
    auto* baseStationGroup = group("rtkFixedMode");
    auto* baseSitesGroup = group("rtkBaseSiteName");
    auto* options = group("rtkAutoConnect");
    QVERIFY(status && connection && baseStationGroup && baseSitesGroup && options);
    // Connect is in the status group, which the page always shows.
    QCOMPARE(status, group("rtkReceiverStatus"));
    QVERIFY(status->isVisible() && connection->isVisible() && options->isVisible());
    // The group of the remaining options is headed as the sidebar lists it, not after the button that left it.
    const auto headed = TestFixtures::findItems(
        options, [](QQuickItem* item) { return !item->property("heading").toString().isEmpty(); });
    QCOMPARE(headed.size(), 1);
    QCOMPARE(headed.first()->property("heading").toString(), QStringLiteral("Connection Options"));
    QCOMPARE(baseStationGroup->isVisible(), baseStation);
    QCOMPARE(baseSitesGroup->isVisible(), baseSites);
    for (const auto& [file, shown] :
         {std::pair{"GPSReceiverBaseStation.qml", baseStation}, std::pair{"GPSReceiverBaseSites.qml", baseSites}}) {
        auto part = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/") + QString::fromLatin1(file)));
        QVERIFY2(part, qPrintable(engine.lastError()));
        QVERIFY2(part->property("visible").toBool() == shown, file);
    }
}

UT_REGISTER_TEST(GPSReceiverSettingsTest, TestLabel::Unit)
