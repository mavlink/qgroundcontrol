#include "GPSDiagnosticsUITest.h"

#include <QtCore/QScopeGuard>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include <memory>

#include "ColoredSvgImageProvider.h"
#include "Fact.h"
#include "GPSPositionSettings.h"
#include "QGCFileDialogController.h"
#include "SettingsManager.h"

namespace {
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root || root->objectName() == name) {
        return root;
    }
    for (auto* child : root->childItems()) {
        if (auto* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

std::unique_ptr<QObject> mock(QQmlEngine& engine, const QByteArray& source)
{
    QQmlComponent component(&engine);
    component.setData(source, QUrl());
    return std::unique_ptr<QObject>(component.create());
}

void configureEngine(QQmlEngine& engine)
{
    engine.addImageProvider(QStringLiteral("coloredsvg"), new ColoredSvgImageProvider);
    engine.addImportPath(QStringLiteral("qrc:/qml"));
}
}  // namespace

void GPSDiagnosticsUITest::_positionSelection()
{
    auto* settings = SettingsManager::instance()->gpsPositionSettings();
    const QVariant originalMode = settings->sourceMode()->rawValue();
    const auto restoreMode =
        qScopeGuard([settings, originalMode]() { settings->sourceMode()->setRawValue(originalMode); });
    settings->sourceMode()->setRawValue(0);
    QQmlEngine engine;
    configureEngine(engine);
    auto position = mock(engine, R"(
        import QtQml
        QtObject {
            property string selectedSourceName: "NMEA"
            property string selectionReason: "NMEA source selected"
            property string sourceStatusText: "Waiting for NMEA fix"
        }
    )");
    QVERIFY(position);
    QQmlComponent component(&engine);
    component.loadFromModule("QGroundControl.AppSettings", "PositionSourceSettings");
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> panel(component.createWithInitialProperties(
        {{QStringLiteral("settings"), QVariant::fromValue(settings)},
         {QStringLiteral("positionManager"), QVariant::fromValue(position.get())}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* root = qobject_cast<QQuickItem*>(panel.get());
    auto* control = findItem(root, QStringLiteral("gpsPositionSourceMode"));
    QVERIFY(control);
    auto* combo = control->property("comboBox").value<QObject*>();
    QVERIFY(combo);
    QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, 3)));
    QCOMPARE(settings->sourceMode()->rawValue().toInt(), 3);
    auto* status = findItem(root, QStringLiteral("gpsPositionSourceStatus"));
    QVERIFY(status);
    QCOMPARE(status->property("text").toString(), QStringLiteral("Waiting for NMEA fix"));
    position->setProperty("sourceStatusText", QStringLiteral("NMEA fix active"));
    QCOMPARE(status->property("text").toString(), QStringLiteral("NMEA fix active"));
}

void GPSDiagnosticsUITest::_recordingAndExport()
{
    QQmlEngine engine;
    configureEngine(engine);
    auto controller = mock(engine, R"(
        import QtQml
        QtObject {
            property bool recording: false
            property bool hasRecording: false
            property int eventCount: 2
            property int bytesRecorded: 100
            property bool limitReached: false
            property string errorString: ""
            property string lastExportPath: ""
            property int exports: 0
            function start() { recording = true; return true }
            function stop() { recording = false }
            function exportRecording(file) { lastExportPath = file.toString(); exports++; return true }
        }
    )");
    QVERIFY(controller);
    QQmlComponent component(&engine);
    component.loadFromModule("QGroundControl.AppSettings", "GPSRecordingSettings");
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> panel(
        component.createWithInitialProperties({{QStringLiteral("controller"), QVariant::fromValue(controller.get())},
                                               {QStringLiteral("exportFolder"), QStringLiteral("/tmp")}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* root = qobject_cast<QQuickItem*>(panel.get());
    auto* toggle = findItem(root, QStringLiteral("gpsRecordingToggle"));
    auto* exportButton = findItem(root, QStringLiteral("gpsRecordingExport"));
    QVERIFY(toggle);
    QVERIFY(exportButton);
    QVERIFY(!exportButton->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(toggle, "clicked"));
    QVERIFY(controller->property("recording").toBool());
    controller->setProperty("hasRecording", true);
    QVERIFY(!exportButton->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(toggle, "clicked"));
    QVERIFY(exportButton->isEnabled());
    QGCFileDialogController::setTestNextFileForAccept(QStringLiteral("/tmp/gps capture.json"));
    QVERIFY(QMetaObject::invokeMethod(exportButton, "clicked"));
    QCOMPARE(controller->property("exports").toInt(), 1);
    QCOMPARE(QUrl(controller->property("lastExportPath").toString()).toLocalFile(),
             QStringLiteral("/tmp/gps capture.json"));
    QVERIFY(!QGCFileDialogController::testHookArmed());
}

void GPSDiagnosticsUITest::_observationDetails()
{
    QQmlEngine engine;
    configureEngine(engine);
    auto satellites = mock(engine, R"(
        import QtQuick
        ListModel {
            property bool fresh: true
            property string sourceId: "Receiver"
            ListElement { satelliteId: 3; constellation: "GPS"; used: true; elevation: 45; signalStrength: 30; azimuth: 120 }
        }
    )");
    auto nmea = mock(engine, R"(
        import QtQuick
        ListModel { property bool fresh: false; property string sourceId: "NMEA" }
    )");
    auto relative = mock(engine, R"(
        import QtQml
        QtObject { property bool fresh: false; property string sourceId: "Receiver" }
    )");
    QVERIFY(satellites);
    QVERIFY(nmea);
    QVERIFY(relative);
    QQmlComponent component(&engine);
    component.loadFromModule("QGroundControl.AppSettings", "ReceiverObservationDiagnostics");
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> panel(component.createWithInitialProperties(
        {{QStringLiteral("satelliteModel"), QVariant::fromValue(satellites.get())},
         {QStringLiteral("nmeaSatelliteModel"), QVariant::fromValue(nmea.get())},
         {QStringLiteral("relativePosition"), QVariant::fromValue(relative.get())}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* root = qobject_cast<QQuickItem*>(panel.get());
    QVERIFY(!findItem(root, QStringLiteral("gpsSatelliteDetails")));
    auto* details = findItem(root, QStringLiteral("gpsObservationDetailsToggle"));
    QVERIFY(details);
    details->setProperty("checked", true);
    QTRY_VERIFY_WITH_TIMEOUT(findItem(root, QStringLiteral("gpsSatelliteDetails")), TestTimeout::mediumMs());
    auto* selector = findItem(root, QStringLiteral("gpsSatelliteSource"));
    auto* summary = findItem(root, QStringLiteral("gpsSatelliteSummary"));
    QVERIFY(selector);
    QVERIFY(summary);
    QVERIFY(summary->property("text").toString().contains(QStringLiteral("Receiver")));
    selector->setProperty("currentIndex", 1);
    QVERIFY(summary->property("text").toString().contains(QStringLiteral("No fresh")));
    nmea->setProperty("fresh", true);
    QVERIFY(summary->property("text").toString().contains(QStringLiteral("NMEA")));
    details->setProperty("checked", false);
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(root, QStringLiteral("gpsSatelliteDetails")), TestTimeout::mediumMs());
}

void GPSDiagnosticsUITest::_configurationReadback()
{
    QQmlEngine engine;
    configureEngine(engine);
    const QVariantList report{QVariantMap{{"key", "dynamicModel"},
                                          {"label", "Dynamic model"},
                                          {"requestedValue", 2},
                                          {"requestState", 1},
                                          {"readbackState", 0},
                                          {"detail", ""}},
                              QVariantMap{{"key", "outputRateHz"},
                                          {"label", "Output rate"},
                                          {"units", "Hz"},
                                          {"requestedValue", 5},
                                          {"requestState", 1},
                                          {"readbackState", 1},
                                          {"reportedValue", 1},
                                          {"comparisonApplicable", true},
                                          {"matchesRequested", false},
                                          {"detail", ""}}};
    QQmlComponent component(&engine);
    component.loadFromModule("QGroundControl.AppSettings", "ReceiverConfigurationStatus");
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> panel(component.createWithInitialProperties(
        {{QStringLiteral("report"), report}, {QStringLiteral("reportActive"), true}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* root = qobject_cast<QQuickItem*>(panel.get());
    auto* toggle = findItem(root, QStringLiteral("gpsConfigurationReportToggle"));
    QVERIFY(toggle);
    toggle->setProperty("checked", true);
    QTRY_VERIFY_WITH_TIMEOUT(findItem(root, QStringLiteral("gpsConfigurationReadback_dynamicModel")),
                             TestTimeout::mediumMs());
    auto* unverified = findItem(root, QStringLiteral("gpsConfigurationReadback_dynamicModel"));
    auto* mismatch = findItem(root, QStringLiteral("gpsConfigurationReadback_outputRateHz"));
    QVERIFY(mismatch);
    QVERIFY(unverified->property("text").toString().contains(QStringLiteral("unavailable")));
    QVERIFY(mismatch->property("text").toString().contains(QStringLiteral("differs")));
    panel->setProperty("reportActive", false);
    auto* summary = findItem(root, QStringLiteral("gpsConfigurationReportSummary"));
    QVERIFY(summary);
    QVERIFY(summary->property("text").toString().contains(QStringLiteral("Previous connection")));
}

UT_REGISTER_TEST(GPSDiagnosticsUITest, TestLabel::Unit)
